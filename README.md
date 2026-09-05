# TinyRTOS

面向 STM32F407VGT6、使用 GCC/CMake 构建的轻量级抢占式 RTOS 学习项目。

## 功能

- 静态 TCB 池和任务栈，不使用 `malloc`
- 基于优先级的调度、PendSV 上下文切换
- SysTick 任务延时和时间片轮转
- 临界区、信号量、消息队列、递归互斥锁
- 互斥锁优先级继承
- 任务自删除和 idle 回收 TCB
- idle 任务使用 `__WFI()` 等待中断

## 目录

```text
Core/User/app/
  os_examples.c/.h   可切换的功能示例
Core/User/rtos/
  kernel.c/.h        内核、调度、任务创建和删除
  tcb.h              TCB 与任务状态
  switch.s           PendSV/SVC 上下文切换
  port.h             临界区接口
  sem.c/.h           信号量
  queue.c/.h         消息队列
  mutex.c/.h         互斥锁
```

## 选择示例

编辑 `Core/User/app/os_examples.c`：

```c
#define DEMO_SELECT 2
```

| 值 | 示例 |
|---:|---|
| 1 | 任务延时和同优先级调度 |
| 2 | 信号量生产者/消费者 |
| 3 | 消息队列发送与接收 |
| 4 | 两个任务竞争互斥锁 |
| 5 | 任务删除与 idle 回收 TCB |

系统入口由 `Core/Src/main.c` 调用：

```c
os_examples_init();
os_start();
```

## 任务写法

任务入口必须是 `void (*)(void *)`：

```c
void my_task(void *arg)
{
    (void)arg;
    while (1) {
        /* 任务工作 */
        task_delay(1000);
    }
}
```

任务删除当前任务：

```c
task_delete(NULL);
```

任务先标记为 `TASK_DYING` 并切换出去，再由 idle 清空 TCB，使槽位恢复为 `TASK_FREE`。

## 注意事项

- `MAX_TASKS` 当前为 5，idle 任务也占用一个 TCB。
- 任务栈大小当前为 2048 字节，使用 `printf` 时注意栈空间。
- `task_delay()` 已经包含内核所需的临界区，不要在外层再次包裹临界区。
- 不要在临界区中调用 `HAL_Delay()`；它是依赖中断 tick 的忙等待。
- 任务函数不要直接返回，应持续运行或调用 `task_delete(NULL)`。
- 外部删除目前只安全支持 READY 任务；BLOCKED 任务还可能挂在等待队列中。

## 构建

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

Release：

```powershell
cmake --preset Release
cmake --build --preset Release
```

串口输出使用 USART1，具体波特率以 CubeMX 配置为准。

本说明只描述源码结构，不代表已经完成编译、烧录或真实硬件验证。
