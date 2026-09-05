  .syntax unified
  .cpu    cortex-m4
  .thumb


  .global PendSV_Handler
  .thumb_func
  .type   PendSV_Handler, %function

PendSV_Handler:
  LDR R1,=current_tcb
  LDR R2,[R1]
  CBZ R2,PendSV_restore

  MRS R0,PSP
  STMDB R0!,{R4-R11}
  STR R0,[R2,#o]

 PendSV_restore
  PUSH {LR}
  BL   os_schedule
  POP   {LR}

  LDR R1,=current_tcb
  LDR R1,[R1]
  LDR R0,[R1,#0]
  LDMIA R0!,{R4-R11}
  MSR   PSP,R0   
  BX   LR

  .size   PendSV_Handler, .-PendSV_Handler

  global SVC_Handler
  .thumb_func
  .type   SVC_Handler, %function
SVC_Handler:
    LDR     R0, =current_tcb
    LDR     R0, [R0]            /* R0 = current_tcb（os_start 已预选） */
    LDR     R0, [R0, #0]        /* R0 = current_tcb->sp（首任务栈） */
    LDMIA   R0!, {R4-R11}       /* 弹首帧软件 8 字 */
    MSR     PSP, R0             /* PSP = 首帧硬件区 */
    BX      LR                  /* 异常返回：弹硬件 8 字 → 任务从入口开跑 */

    .size   SVC_Handler, .-SVC_Handler

    .end                        /* 汇编文件结束 */