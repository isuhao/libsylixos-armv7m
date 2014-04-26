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
** 文   件   名: _ThreadMiscLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 18 日
**
** 描        述: 线程改变优先级。

** BUG
2008.03.30  使用新的就绪环操作.
2012.07.04  合并 _ThreadDeleteWaitDeath() 到这里.
2012.08.24  加入 _ThreadUserGet() 函数.
2012.10.23  加入 _ThreadContinue() 函数.
2013.07.18  加入 _ThreadStop() 函数.
2013.08.27  加入内核事件监控器.
2013.12.02  将状态改变函数移动到 _ThreadStatus.c 文件中.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _ThreadUserGet
** 功能描述: 获取线程用户信息
** 输　入  : ulId          线程 ID
**           puid          uid
**           pgid          gid
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  _ThreadUserGet (LW_HANDLE  ulId, uid_t  *puid, gid_t  *pgid)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        return  (ERROR_THREAD_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    if (puid) {
        *puid = ptcb->TCB_uid;
    }
    if (pgid) {
        *pgid = ptcb->TCB_gid;
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _ThreadDeleteWaitDeath
** 功能描述: 将将要删除的线程进入僵死状态.
** 输　入  : ptcbDel         将要删除的线程控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : ptcbDel != current tcb
             这里等待任务自旋锁是为了确保将要被删除的任务没有在操作自己的协程.
*********************************************************************************************************/
VOID  _ThreadDeleteWaitDeath (PLW_CLASS_TCB  ptcbDel)
{
    LW_SPIN_LOCK(&ptcbDel->TCB_slLock);
    __KERNEL_ENTER();
    
    _ThreadStatusChange(ptcbDel, LW_TCB_REQ_WDEATH);
    
    LW_SPIN_UNLOCK(&ptcbDel->TCB_slLock);
    __KERNEL_EXIT();                                                    /*  等待对方状态转换完毕        */
}
/*********************************************************************************************************
** 函数名称: _ThreadSched
** 功能描述: 线程解锁后尝试进行调度
** 输　入  : ptcbCur       当前任务控制块
** 输　出  : 调度器返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _ThreadSched (PLW_CLASS_TCB  ptcbCur)
{
    LW_KERNEL_JOB_EXEC();                                               /*  尝试执行异步工作队列        */
    
    return  (_Sched());
}
/*********************************************************************************************************
** 函数名称: _ThreadStop
** 功能描述: 停止线程 (进入内核后被调用)
** 输　入  : ptcb          线程控制块
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : ptcb != current tcb
*********************************************************************************************************/
ULONG  _ThreadStop (PLW_CLASS_TCB  ptcb)
{
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_STOP, 
                      ptcb->TCB_ulId, LW_NULL);

    return  (_ThreadStatusChange(ptcb, LW_THREAD_STATUS_STOP));
}
/*********************************************************************************************************
** 函数名称: _ThreadContinue
** 功能描述: 被 _ThreadStop() 的线程继续执行 (进入内核后被调用)
** 输　入  : ptcb          线程控制块
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : ptcb != current tcb
*********************************************************************************************************/
ULONG  _ThreadContinue (PLW_CLASS_TCB  ptcb)
{
             INTREG         iregInterLevel;
    REGISTER PLW_CLASS_PCB  ppcb;
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_CONT, 
                      ptcb->TCB_ulId, LW_NULL);
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    ppcb = _GetPcb(ptcb);
    
    if (!(ptcb->TCB_usStatus & LW_THREAD_STATUS_STOP)) {                /*  已经退出了 STOP 状态        */
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        return  (ERROR_NONE);
    }
    
    if (ptcb->TCB_ulStopNesting) {
        ptcb->TCB_ulStopNesting--;
        if (ptcb->TCB_ulStopNesting == 0) {
            ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_STOP);
            if (__LW_THREAD_IS_READY(ptcb)) {                           /*  就绪 ?                      */
                ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;     /*  中断激活方式                */
                __ADD_TO_READY_RING(ptcb, ppcb);                        /*  加入到相对优先级就绪环      */
            }
        }
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _ThreadMakeGlobal
** 功能描述: 将指定线程转换为内核线程, 而不被进程回收资源 (进入内核后被调用)
** 输　入  : ulId          线程 ID
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  _ThreadMakeGlobal (LW_HANDLE  ulId)
{
             INTREG         iregInterLevel;
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        return  (ERROR_THREAD_NULL);
    }
    
    if (_Thread_Invalid(usIndex)) {
        return  (ERROR_THREAD_NULL);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    ptcb->TCB_ulOption |= LW_OPTION_OBJECT_GLOBAL;
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
    
#if LW_CFG_MODULELOADER_EN > 0
    __resHandleMakeGlobal(ulId);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    _ErrorHandle(ERROR_NONE);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _ThreadMakeLocal
** 功能描述: 将指定线程去掉内核线程属性 (进入内核后被调用)
** 输　入  : ulId          线程 ID
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  _ThreadMakeLocal (LW_HANDLE  ulId)
{
             INTREG         iregInterLevel;
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        return  (ERROR_THREAD_NULL);
    }
    
    if (_Thread_Invalid(usIndex)) {
        return  (ERROR_THREAD_NULL);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    ptcb->TCB_ulOption &= (~LW_OPTION_OBJECT_GLOBAL);
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */
    
#if LW_CFG_MODULELOADER_EN > 0
    __resHandleMakeLocal(ulId);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    _ErrorHandle(ERROR_NONE);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _ThreadTraversal
** 功能描述: 遍历所有的线程 (必须在锁定内核的前提下调用)
** 输　入  : pfunc             回调函数
**           pvArg[0 ~ 5]      遍历函数参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadTraversal (VOIDFUNCPTR    pfunc, 
                        PVOID          pvArg0,
                        PVOID          pvArg1,
                        PVOID          pvArg2,
                        PVOID          pvArg3,
                        PVOID          pvArg4,
                        PVOID          pvArg5)
{
    REGISTER PLW_CLASS_TCB         ptcb;
             PLW_LIST_LINE         plineList;
             
    for (plineList  = _K_plineTCBHeader;
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {
         
        ptcb = _LIST_ENTRY(plineList, LW_CLASS_TCB, TCB_lineManage);
        pfunc(ptcb, pvArg0, pvArg1, pvArg2, pvArg3, pvArg4, pvArg5);
    }
}
/*********************************************************************************************************
** 函数名称: _ThreadRestartProcHook
** 功能描述: 线程 Rstart 时对 hook 的处理
** 输　入  : ptcb      任务控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadRestartProcHook (PLW_CLASS_TCB  ptcb)
{
    LW_OBJECT_HANDLE  ulId = ptcb->TCB_ulId;

    _TCBCleanupPopExt(ptcb);                                            /*  cleanup 操作                */
    bspTaskDeleteHook(ulId, LW_NULL, ptcb);                             /*  用户钩子函数                */
    __LW_THREAD_DELETE_HOOK(ulId, LW_NULL, ptcb);
    bspTCBInitHook(ulId, ptcb);                                         /*  调用钩子函数                */
    __LW_THREAD_INIT_HOOK(ulId, ptcb);
    bspTaskCreateHook(ulId);                                            /*  调用钩子函数                */
    __LW_THREAD_CREATE_HOOK(ulId, ptcb->TCB_ulOption);
}
/*********************************************************************************************************
** 函数名称: _ThreadDeleteProcHook
** 功能描述: 线程 Delete 时对 hook 的处理
** 输　入  : ptcb          任务控制块
**           pvRetVal      任务返回值
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadDeleteProcHook (PLW_CLASS_TCB  ptcb, PVOID  pvRetVal)
{
    LW_OBJECT_HANDLE  ulId = ptcb->TCB_ulId;

    _TCBCleanupPopExt(ptcb);
    bspTaskDeleteHook(ulId, pvRetVal, ptcb);                            /*  用户钩子函数                */
    __LW_THREAD_DELETE_HOOK(ulId, pvRetVal, ptcb);
    _TCBDestroyExt(ptcb);                                               /*  销毁 TCB 扩展区             */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
