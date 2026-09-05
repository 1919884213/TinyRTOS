#include "main_demo.h"
#include "cmsis_gcc.h"
#include "kernel.h"
#include "main.h"
#include "mutex.h"
#include "stm32f4xx.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/_types.h>
#include "gpio.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"

/* =====================================================================
 * 互斥量 + 优先级反转演示
 * ---------------------------------------------------------------------
 * 三个角色：
 *   taskL  低优先级(1)：持有互斥量，占用共享资源一小段时间
 *   taskM  中优先级(2)：CPU 密集，不停抢 CPU（它不需要锁）
 *   taskH  高优先级(3)：想要锁，但锁被 taskL 拿着
 *
 * 经典反转时序（把 CMakeLists 里 MUTEX_INHERIT 删掉再看）：
 *   ① taskL 先拿到锁，正忙着用共享资源
 *   ② taskM(2) 抢占 taskL(1) 疯狂跑 → taskL 被压住，没机会释放锁
 *   ③ taskH(3) 醒来要锁 → 锁被 taskL 持有 → taskH 阻塞
 *   ④ 现在就绪队列最高是 taskM(2) → CPU 一直在 taskM 手里
 *      → 明明 taskH 优先级最高，却被 taskM 拖住 → ★优先级反转★
 *
 * 打开 MUTEX_INHERIT（默认已开）：
 *   ③ taskH 阻塞的那一刻，会把持有者 taskL 提升到优先级 3
 *      → 调度器马上选 taskL 跑 → taskL 忙完释放锁 → taskH 立刻拿到锁
 *      → 反转被消除（这叫"优先级继承"）
 * ===================================================================== */
static mutex_t mtx;

/* 低优先级任务：持有共享资源（互斥量）一段时间 */
static void taskL(void *arg) {
  while (1) {
    mutex_take(&mtx);
    printf("[%5lu] L  拿到锁，占用共享资源...\r\n", (unsigned long)g_tick);
    /* 模拟用共享资源：忙等约 3ms（期间不让出 CPU，锁一直攥着） */
    uint32_t end = g_tick + 3;
    while (g_tick < end) { /* 空转，纯占用 CPU */ }
    printf("[%5lu] L  用完，释放锁\r\n", (unsigned long)g_tick);
    mutex_give(&mtx);
    task_delay(30); /* 让出，等下轮 */
  }
}

/* 中优先级任务：CPU 密集，不需要锁，专门"压着"低优先级任务 */
static void taskM(void *arg) {
  while (1) {
    printf("[%5lu] M  抢占 CPU（密集计算中）\r\n", (unsigned long)g_tick);
    for (volatile int i = 0; i < 200000; i++) { /* 空转烧 CPU */ }
    task_delay(1);
  }
}

/* 高优先级任务：最想要锁，但总被低优先级任务占着 */
static void taskH(void *arg) {
  while (1) {
    task_delay(10); /* 让 taskL 先抢到锁，反转才有戏 */
    printf("[%5lu] H  尝试拿锁...\r\n", (unsigned long)g_tick);
    mutex_take(&mtx);
    printf("[%5lu] H  拿到锁！\r\n", (unsigned long)g_tick);
    task_delay(5);
    mutex_give(&mtx);
  }
}


/* 空闲任务：最低优先级(0)，所有任务都阻塞时运行。
 * 为什么必须要有它？
 *   没有它：两个任务都 task_delay 时 ready_bitmap==0，os_schedule 直接 return
 *           保持 current_tcb → PendSV 把"阻塞中"的任务假唤醒继续跑，
 *           它的 delay_ticks 每次都被重置、永远到不了 0 → 调度全乱（本 bug
 * 现场） 有它  ：ready_bitmap 至少 bit0=1，os_schedule 永远选得到 idle任务，
 *           阻塞任务真正睡到 delay 归零才被 os_tick 唤醒 */
static void idle(void *arg) {
  while (1) {
    __WFI();
  }
}

void app_init(void) {
  mutex_init(&mtx);
  task_create(taskL, NULL, 1);
  task_create(taskM, NULL, 2);
  task_create(taskH, NULL, 3);
  task_create(idle, NULL, 0);  /* 最低优先级兜底，杜绝假唤醒 */
}