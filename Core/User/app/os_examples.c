#include "os_examples.h"
#include "cmsis_gcc.h"
#include "kernel.h"
#include "mutex.h"
#include "queue.h"
#include "sem.h"
#include "stdint.h"
#include <stdio.h>

/* 1=延时  2=信号量  3=消息队列  4=互斥锁  5=任务删除 */
#ifndef DEMO_SELECT
#define DEMO_SELECT 2
#endif

static void idle_task(void *arg) {
  (void)arg;
  while (1) {
    for (int i = 0; i < MAX_TASKS; ++i) {
      tcb_t *t = &tcb_pool[i];
      if (t != current_tcb && t->state == TASK_DYING) {
        t->sp = NULL;
        t->next = NULL;
        t->stack_base = 0;
        t->delay_ticks = 0;
        t->slice = 0;
        t->state = TASK_FREE;
        printf("idle: recycle TCB[%d]\r\n", i);
      }
    }
    __WFI();
  }
}

static void task_a(void *arg) {
  (void)arg;
  while (1) {
    printf("A\r\n");
    task_delay(500);
  }
}
static void task_b(void *arg) {
  (void)arg;
  while (1) {
    printf("B\r\n");
    task_delay(1000);
  }
}

static sem_t demo_sem;
static void sem_tx(void *arg) {
  (void)arg;
  while (1) {
    sem_give(&demo_sem);
    printf("sem: give\r\n");
    task_delay(1000);
  }
}
static void sem_rx(void *arg) {
  (void)arg;
  while (1) {
    sem_take(&demo_sem);
    printf("sem: take\r\n");
  }
}

static queue_t demo_queue;
static int demo_queue_buf[4];
static void queue_tx(void *arg) {
  (void)arg;
  int n = 0;
  while (1) {
    queue_send(&demo_queue, &n);
    printf("queue: send %d\r\n", n++);
    task_delay(500);
  }
}
static void queue_rx(void *arg) {
  (void)arg;
  int n;
  while (1) {
    queue_recv(&demo_queue, &n);
    printf("queue: recv %d\r\n", n);
  }
}

static mutex_t demo_mutex;
static void mutex_task(void *arg) {
  (void)arg;
  while (1) {
    mutex_take(&demo_mutex);
    printf("mutex: locked\r\n");
    task_delay(100);
    mutex_give(&demo_mutex);
    task_delay(300);
  }
}
static void delete_task(void *arg) {
  (void)arg;
  printf("delete: exit\r\n");
  task_delete(NULL);
  while (1) {
  }
}

void os_examples_init(void) {
  task_create(idle_task, NULL, 0);
#if DEMO_SELECT == 1
  task_create(task_a, NULL, 2);
  task_create(task_b, NULL, 2);
#elif DEMO_SELECT == 2
  sem_init(&demo_sem, 0);
  task_create(sem_tx, NULL, 2);
  task_create(sem_rx, NULL, 3);
#elif DEMO_SELECT == 3
  queue_init(&demo_queue, demo_queue_buf, sizeof(demo_queue_buf[0]), 4);
  task_create(queue_tx, NULL, 2);
  task_create(queue_rx, NULL, 3);
#elif DEMO_SELECT == 4
  mutex_init(&demo_mutex);
  task_create(mutex_task, NULL, 2);
  task_create(mutex_task, NULL, 2);
#elif DEMO_SELECT == 5
  task_create(delete_task, NULL, 3);
#endif
}
