// Atmega328P Timer1による実装
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#define TASK_ID_MAX 10
#define TASK_PRIORITY_NUMBER 10

unsigned char run_queue;
typedef unsigned int Priority;

void create_task(unsigned char task_id, Priority priority, void (*task)(void));
unsigned char get_top_ready_id(void);
void os_start(void);
struct TCB *readyqueue_deque(int priority);
void readyqueue_enque(struct TCB *p_tcb_);
void schedule(void);
void timer_create(void);
void task_start(unsigned char current_task_id);
void timer_init(void);
int timer_handler(void);

enum TaskState
{
    RUNNING,
    READY,
    SUSPEND
};

/* task control block */
typedef struct TCB
{
    unsigned char task_id_; /* タスクの識別子 0~255の整数値*/
    TaskState state_;       /* running, ready, suspendの3つの状態を持つ */
    Priority priority_;     /* タスクの優先度(0~9) */
    struct TCB *p_prev_;    /* 前のタスク p_next_*/
    struct TCB *p_next_;    /* 次のタスク p_prev_*/
    void (*task_handler_)(void);
} tcb_t;
tcb_t TCB[3];

// 実行可能状態のタスクを登録するリスト
tcb_t ready_que[TASK_PRIORITY_NUMBER];
tcb_t *current;
tcb_t *load_tcb;

struct TCB *readyqueue_deque(int priority)
{
    struct TCB *p_ready_que_ = &ready_que[priority];

    struct TCB *p_tcb_;

    p_tcb_ = p_ready_que_->p_next_;

    task_reload();

    if (p_tcb_)
    {
        p_ready_que_->p_next_ = p_tcb_->p_next_;
        if (p_ready_que_->p_next_ == 0)
        {
            p_ready_que_->p_prev_ = 0;
        }
        p_tcb_->p_next_ = 0;
    }

    return p_tcb_;
}

void readyqueue_enque(struct TCB *p_tcb_)
{
    struct TCB *p_ready_que_ = &ready_que[p_tcb_->priority_];

    p_tcb_->p_next_ = 0;

    if (p_ready_que_->p_prev_)
    {
        p_ready_que_->p_prev_->p_next_ = p_tcb_;
    }
    else
    {
        p_ready_que_->p_next_ = p_tcb_;
    }
    p_ready_que_->p_prev_ = p_tcb_;
}

void schedule(void)
{
    // タスクの初期化
    int i;
    for (i = 0; i < TASK_ID_MAX; i++)
    {
        current = readyqueue_deque(i);
        if (current)
            return;
    }
}

void create_task(unsigned char task_id, Priority priority, void (*task)(void))
{

    if (task_id >= TASK_ID_MAX)
        exit(1);
    if (&TCB[task_id] == 0)
    {
        os_start();
    }

    current = &TCB[task_id];
    load_tcb = current;

    current->task_id_ = task_id;
    current->priority_ = priority;
    current->task_handler_ = task;
    current->state_ = READY;

    readyqueue_enque(current);
}

void context_switch(void)
{
    // タスクのコンテキストを保存
    if (current->state_ == READY)
        readyqueue_enque(current);

    schedule();
}

unsigned char get_top_ready_id(void)
{
    struct TCB *p_tcb_;
    Priority priority_;
    for (priority_ = 0; priority_ < TASK_PRIORITY_NUMBER; priority_++)
    {
        //  優先順位順に連結されているTCBリストの末尾まで探索し
        if (ready_que[priority_].p_next_ != 0)
        {
            return ready_que[priority_].p_next_->task_id_;
        }
    }
    return 0;
}

unsigned char current_task_id;
void os_start(void)
{
    // シリアル通信の遅延が処理に作用しているため、printはそのままにする
    Serial.println("OS_START");
    current_task_id = get_top_ready_id();

    schedule();
    current[current_task_id] = TCB[current_task_id];
    current[current_task_id].state_ = RUNNING;

    context_switch();
    task_start(current_task_id);

    sei();
}

void task_reload()
{
    if (current_task_id == 0)
    {
        cli();
        // シリアル通信の遅延が処理に作用しているため、printはそのままにする
        Serial.println("TASK_RELOAD");
        current = load_tcb;
        current_task_id = current->task_id_;
        current_task_id = current_task_id + 1;
        return;
    }
}

void task_start(unsigned char current_id)
{
    if (current[current_task_id].task_id_ != 0)
    {
        (*current[current_task_id].task_handler_)();
    }
}

void timer_init(void)
{
    TCNT1 = 3036;
}

void timer_create(void)
{
    cli();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 3036;
    TCCR1B |= _BV(CS12);  // 256分周, CTCモード
    TIMSK1 |= _BV(TOIE1); // オーバーフロー割り込みを許可
}

ISR(TIMER1_OVF_vect)
{
    os_start();
}
void LED1(void)
{
    digitalWrite(2, !digitalRead(2));
}

void LED2(void)
{
    digitalWrite(3, !digitalRead(3));
}

void LED3(void)
{
    digitalWrite(4, !digitalRead(4));
}

void setup()
{
    Serial.begin(9600);

    memset(TCB, 0, sizeof(TCB));
    memset(ready_que, 0, sizeof(ready_que));

    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(4, OUTPUT);

    create_task(1, 6, LED1);
    create_task(2, 4, LED2);
    create_task(3, 5, LED3);
    timer_create();
    os_start();

    while (1)
        ; // 無限ループ（割込み待ち）
}

void loop()
{
}