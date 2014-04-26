/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: _ThreadStat.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 02 日
**
** 描        述: 线程状态修改.

** BUG:
2013.12.11  加入 _ThreadUnwaitStatus() 删除或者重启线程时, 如果正在等待其他线程改变状态, 则需要退出链表.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _ThreadWaitStatus
** 功能描述: 等待目标线程状态设置完毕 (进入调度器与内核并关中断状态被调用)
** 输　入  : ptcbCur       当前线程控制块
**           ptcbDest      目标线程控制块
**           uiStatusReq   请求改变的
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static VOID  _ThreadWaitStatus (PLW_CLASS_TCB  ptcbCur, PLW_CLASS_TCB  ptcbDest, UINT  uiStatusReq)
{
    PLW_CLASS_PCB  ppcb;

    if (__LW_THREAD_IS_READY(ptcbCur)) {
        ppcb = _GetPcb(ptcbCur);
        __DEL_FROM_READY_RING_NOLOCK(ptcbCur, ppcb);                    /*  从就绪表中删除              */
    }
    
    ptcbCur->TCB_ptcbWaitStatus     = ptcbDest;
    ptcbCur->TCB_uiStatusChangeReq  = uiStatusReq;                      /*  请求修改的状态              */
    ptcbCur->TCB_usStatus          |= LW_THREAD_STATUS_WSTAT;
    
    _List_Line_Add_Ahead(&ptcbCur->TCB_lineStatusPend,
                         &ptcbDest->TCB_plineStatusReqHeader);
}
/*********************************************************************************************************
** 函数名称: _ThreadUnwaitStatus
** 功能描述: 从等待目标线程状态设置链表中退出, 并激活其他任务等待我的状态改变.(进入内核并关中断状态被调用)
** 输　入  : ptcbCur       当前线程控制块
**           ptcbDest      目标线程控制块
**           uiStatusReq   请求改变的
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : 这里只是退出链表, 并不操作就续表.
*********************************************************************************************************/
VOID  _ThreadUnwaitStatus (PLW_CLASS_TCB  ptcb)
{
    PLW_CLASS_TCB  ptcbDest = ptcb->TCB_ptcbWaitStatus;
    PLW_CLASS_TCB  ptcbWait;
    PLW_CLASS_PCB  ppcb;

    if (ptcbDest) {
        _List_Line_Del(&ptcb->TCB_lineStatusPend, 
                       &ptcbDest->TCB_plineStatusReqHeader);
                       
        ptcb->TCB_ptcbWaitStatus    = LW_NULL;
        ptcb->TCB_uiStatusChangeReq = 0;
    }
    
    while (ptcb->TCB_plineStatusReqHeader) {                            /*  有其他线程请求              */
        ptcbWait = _LIST_ENTRY(ptcb->TCB_plineStatusReqHeader, 
                               LW_CLASS_TCB, 
                               TCB_lineStatusPend);
        ptcbWait->TCB_ptcbWaitStatus = LW_NULL;
        
        _List_Line_Del(&ptcbWait->TCB_lineStatusPend, 
                       &ptcb->TCB_plineStatusReqHeader);
        
        if (!__LW_THREAD_IS_READY(ptcbWait)) {                          /*  如果没有就绪, 取消 WSTAT    */
            ptcbWait->TCB_usStatus &= ~LW_THREAD_STATUS_WSTAT;
            if (__LW_THREAD_IS_READY(ptcbWait)) {
                ppcb = _GetPcb(ptcbWait);
                ptcbWait->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT; /*  中断激活方式                */
                __ADD_TO_READY_RING(ptcbWait, ppcb);
            }
        }
    }
}
/*********************************************************************************************************
** 函数名称: _ThreadStatusChangeCur
** 功能描述: 改变当前线程状态
** 输　入  : ptcbCur       当前线程控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 
*********************************************************************************************************/
VOID  _ThreadStatusChangeCur (PLW_CLASS_TCB  ptcbCur)
{
    INTREG         iregInterLevel;
    PLW_CLASS_TCB  ptcb;
    PLW_CLASS_PCB  ppcb;

    if (__THREAD_LOCK_GET(ptcbCur) > 0ul) {                             /*  目前不可抢占                */
        return;
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();
    
    if (ptcbCur->TCB_plineStatusReqHeader) {                            /*  需要阻塞                    */
        if (__LW_THREAD_IS_READY(ptcbCur)) {
            ppcb = _GetPcb(ptcbCur);
            __DEL_FROM_READY_RING(ptcbCur, ppcb);                       /*  从就绪表中删除              */
        }
        
        do {
            ptcb = _LIST_ENTRY(ptcbCur->TCB_plineStatusReqHeader, LW_CLASS_TCB, TCB_lineStatusPend);
            
            switch (ptcb->TCB_uiStatusChangeReq) {
            
            case LW_TCB_REQ_SUSPEND:
                ptcbCur->TCB_ulSuspendNesting++;
                ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_SUSPEND;
                break;
                
            case LW_TCB_REQ_STOP:
                ptcbCur->TCB_ulStopNesting++;
                ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_STOP;
                break;
                
            case LW_TCB_REQ_WDEATH:
                ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_WDEATH;
                break;
            }
            
            ptcb->TCB_ptcbWaitStatus    = LW_NULL;
            ptcb->TCB_uiStatusChangeReq = 0;                            /*  请求修改的状态成功          */
            
            _List_Line_Del(&ptcb->TCB_lineStatusPend, 
                           &ptcbCur->TCB_plineStatusReqHeader);
            
            if (!__LW_THREAD_IS_READY(ptcb)) {                          /*  如果没有就绪, 取消 WSTAT    */
                ptcb->TCB_usStatus &= ~LW_THREAD_STATUS_WSTAT;
                if (__LW_THREAD_IS_READY(ptcb)) {
                    ppcb = _GetPcb(ptcb);
                    ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT; /*  中断激活方式                */
                    __ADD_TO_READY_RING(ptcb, ppcb);
                }
            }
        } while (ptcbCur->TCB_plineStatusReqHeader);
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** 函数名称: _ThreadStatusChange
** 功能描述: 改变线程状态 (进入内核状态被调用)
** 输　入  : ptcb          线程控制块
**           uiStatusReq   状态 (LW_TCB_REQ_SUSPEND / LW_TCB_REQ_WDEATH / LW_TCB_REQ_STOP)
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数已经预置的阻塞点, 最后一次退出内核时, 将阻塞等待目标线程状态改变完毕.
*********************************************************************************************************/
ULONG  _ThreadStatusChange (PLW_CLASS_TCB  ptcb, UINT  uiStatusReq)
{
    INTREG         iregInterLevel;
    PLW_CLASS_PCB  ppcb;
    PLW_CLASS_TCB  ptcbCur;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    LW_TCB_GET_CUR(ptcbCur);
    
#if LW_CFG_SMP_EN > 0
    if (ptcb == ptcbCur) {                                              /*  改变自己的状态              */
        goto    __change1;
    
    } else if (!__LW_THREAD_IS_READY(ptcb)) {                           /*  任务没有就绪                */
        goto    __change2;
    
    } else {                                                            /*  目标任务处于就绪状态        */
        if (LW_CPU_GET_CUR_NESTING()) {
            KN_INT_ENABLE(iregInterLevel);                              /*  打开中断                    */
            return  (ERROR_KERNEL_IN_ISR);                              /*  中断状态下无法完成此工作    */
        }
        
        LW_SPIN_LOCK_IGNIRQ(&_K_slScheduler);                           /*  锁调度器 spinlock           */
        if (!__LW_THREAD_IS_RUNNING(ptcb)) {                            /*  目标任务不是正在执行        */
            ppcb = _GetPcb(ptcb);
            __DEL_FROM_READY_RING_NOLOCK(ptcb, ppcb);                   /*  从就绪表中删除              */
            LW_SPIN_UNLOCK_IGNIRQ(&_K_slScheduler);                     /*  解锁调度器 spinlock         */
            goto    __change2;
        
        } else {                                                        /*  目标任务正在执行            */
            _SmpSendIpi(ptcb->TCB_ulCPUId, LW_IPI_STATUS_REQ, 0);       /*  发送核间中断通知改变状态    */
            _ThreadWaitStatus(ptcbCur, ptcb, uiStatusReq);              /*  设置等待对方完成状态        */
            LW_SPIN_UNLOCK_IGNIRQ(&_K_slScheduler);                     /*  解锁调度器 spinlock         */
        }
        
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        return  (ERROR_NONE);
    }
    
__change1:
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
    if (__LW_THREAD_IS_READY(ptcb)) {
        ppcb = _GetPcb(ptcb);
        __DEL_FROM_READY_RING(ptcb, ppcb);                              /*  从就绪表中删除              */
    }
    
#if LW_CFG_SMP_EN > 0
__change2:
#endif                                                                  /*  LW_CFG_SMP_EN               */
    
    ptcbCur->TCB_uiStatusChangeReq = 0;                                 /*  状态修改操作执行成功        */
    
    switch (uiStatusReq) {
    
    case LW_TCB_REQ_SUSPEND:
        ptcb->TCB_ulSuspendNesting++;
        ptcb->TCB_usStatus |= LW_THREAD_STATUS_SUSPEND;
        break;
        
    case LW_TCB_REQ_STOP:
        ptcb->TCB_ulStopNesting++;
        ptcb->TCB_usStatus |= LW_THREAD_STATUS_STOP;
        break;
        
    case LW_TCB_REQ_WDEATH:
        ptcb->TCB_usStatus |= LW_THREAD_STATUS_WDEATH;
        break;
    }
        
    KN_INT_ENABLE(iregInterLevel);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
