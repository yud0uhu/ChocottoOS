// Attiny85 Timer0による実装
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#define TASK_ID_MAX 10
#define TASK_PRIORITY_NUMBER 10

typedef volatile uint8_t Priority;

void all_set_task(struct task_define *tasks);
void create_task(volatile unsigned char task_id, Priority priority, void (*task)(void));
void context_switch(void);
unsigned char get_top_ready_id(void);
void os_start(void);
struct TCB *readyqueue_deque(Priority priority);
void readyqueue_enque(struct TCB *p_tcb_);
void schedule(void);
void timer_create(void);
void task_reload(void);
void task_start(void);
void init_all(void);

struct task_define
{
    volatile unsigned char task_id_;
    Priority priority_;
    void (*task_handler_)(void);
};
struct task_define tasks[] = {};

enum TaskState
{
    RUNNING,
    READY,
    SUSPEND
};

typedef struct TCB
{
    volatile unsigned char task_id_;
    TaskState state_;
    Priority priority_;
    struct TCB *p_prev_;
    struct TCB *p_next_;
    void (*task_handler_)(void);
} tcb_t;

tcb_t TCB[TASK_ID_MAX];

tcb_t ready_que[TASK_PRIORITY_NUMBER];
tcb_t *current;

struct TCB *readyqueue_deque(Priority priority)
{
    struct TCB *p_ready_que_ = &ready_que[priority];

    struct TCB *p_tcb_;

    p_tcb_ = p_ready_que_->p_next_;

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
    tcb_t *p_ready_que_ = &ready_que[p_tcb_->priority_];

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
    uint8_t i;
    for (i = 0; i < TASK_ID_MAX; i++)
    {
        current = readyqueue_deque(i);
        if (current)
        {
            return;
        }
    }
}

void create_task(volatile unsigned char task_id, Priority priority, void (*task)(void))
{
    if (task_id >= TASK_ID_MAX)
        exit(1);

    current = &TCB[task_id];
    current->task_id_ = task_id;
    current->priority_ = priority;
    current->task_handler_ = task;
    current->state_ = READY;

    readyqueue_enque(current);

    return;
}

void context_switch(void)
{
    schedule();
    if (current->state_ == READY)
    {
        readyqueue_enque(current);
        current->state_ = RUNNING;
    }
    else
        task_reload();
    schedule();
}

unsigned char get_top_ready_id(void)
{
    Priority priority_;
    for (priority_ = 0; priority_ < TASK_PRIORITY_NUMBER; priority_++)
    {
        if (ready_que[priority_].p_next_ != 0)
        {
            return ready_que[priority_].p_next_->task_id_;
        }
    }
    return 0;
}

int intr_count = 0;
void os_start(void)

{
    if (get_top_ready_id() == 0)
    {
        task_reload();
    }
    current->task_id_ = get_top_ready_id();
    current->state_ = READY;
    context_switch();
    task_start();
    sei();
}

void task_start()
{
    if (current->task_id_ != 0)
    {
        (*current->task_handler_)();
    }
}

void init_all(void)
{
    memset(TCB, 0, sizeof(TCB));
    memset(current, 0, sizeof(current));
}

void task_reload()
{
    tasks[0] = {2, 2, LED1};
    tasks[1] = {3, 3, LED2};
    tasks[2] = {4, 4, LED3};
    all_set_task(tasks);
}

void timer_create(void)
{
    TCCR0A = 0x00;
    TCCR0B = 0x00;
    TCCR0B |= (1 << CS00) | (1 << CS02);
    TCCR0A |= (1 << WGM01);
    OCR0A = 161;
    TCNT0 = 0;
    TIMSK |= (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect)
{
    if (intr_count == 100) // 1 秒を取得するには、比較一致が 100 回発生する必要があるため、100 カウント待機
    {
        os_start();
        intr_count = 0;
    }
    else
        intr_count++; // 1秒を作るためのインクリメント
}

void LED1(void)
{
    digitalWrite(0, !digitalRead(0));
}

void LED2(void)
{
    digitalWrite(1, !digitalRead(1));
}

void LED3(void)
{
    digitalWrite(2, !digitalRead(2));
}

void all_set_task(struct task_define *tasks)
{
    for (int i = 0; i < sizeof(tasks) + 1; i++)
    {
        create_task(tasks[i].task_id_, tasks[i].priority_, tasks[i].task_handler_);
    }
}
void setup()
{
    cli();
    init_all();

    pinMode(0, OUTPUT);
    digitalWrite(0, LOW);
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);

    tasks[0] = {2, 2, LED1};
    tasks[1] = {3, 3, LED2};
    tasks[2] = {4, 4, LED3};

    all_set_task(tasks);

    timer_create();
    os_start();

    while (1)
        ; // 無限ループ（割込み待ち）
}

void loop()
{
}