# ChocottoOS 仕様書

## timer_create()

- タイマカウンターの作成

```
引数:void
戻り値:void
```

## all_set_task()

- 新規タスクをスレッドに登録する

```
引数:struct task define *tasks
戻り値:void
```

引数は `volatile unsigned char task*id*`, `Priority priority`, `void (*task*handler*)(void)` を引数に持つ構造体配列.

以下のように実行したいタスク関数を作成する

```c++
void LED(void)
{
    digitalWrite(1, !digitalRead(1));
}
```

作成したタスク関数に対し、

```c++
tasks[0] = {2, 2, LED)};
```

としてタスク ID とプライオリティをセットし、次のようにスレッドに登録する

```c++
all_set_task(tasks);
```

## タスク関数

- プライオリティは `0~10` の範囲で指定

```c++
void task_function(void)
{
// do something
}
```

## main 関数

- タイマを用いる場合、main 関数の最後に `while (1);`命令を明示する

```c++
int main(void)
{
init_all();

    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(4, OUTPUT);

    tasks[0] = {2, 2, LED1};
    tasks[1] = {3, 3, LED2};
    tasks[2] = {4, 4, LED3};

    all_set_task(tasks);

    timer_create();
    os_start();
    while (1)
        ; // 無限ループ(割込み待ち)

}
```
