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
** 文   件   名: ThreadTestCancel.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 07 日
**
** 描        述: 检测线程是否有请求取消标志, 当条件满足时自动取消.

** BUG:
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2013.09.17  不允许在中断中被调用.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_THREAD_CANCEL_EN > 0
/*********************************************************************************************************
** 函数名称: API_ThreadTestCancel
** 功能描述: 检测线程是否有请求取消标志, 当条件满足时自动取消.
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_ThreadTestCancel (VOID)
{
    PLW_CLASS_TCB       ptcbCur;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        return;
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */

    if (ptcbCur->TCB_iCancelState == LW_THREAD_CANCEL_ENABLE   &&
        ptcbCur->TCB_iCancelType  == LW_THREAD_CANCEL_DEFERRED &&
        (ptcbCur->TCB_bCancelRequest)) {
        LW_OBJECT_HANDLE    ulId = ptcbCur->TCB_ulId;
        
#if LW_CFG_THREAD_DEL_EN > 0
        API_ThreadForceDelete(&ulId, LW_THREAD_CANCELED);
#endif                                                                  /*  LW_CFG_THREAD_DEL_EN > 0    */
#if LW_CFG_THREAD_SUSPEND_EN > 0
        API_ThreadSuspend(ulId);                                        /*  阻塞线程                    */
#endif
        for (;;) {
            API_TimeSleep(__ARCH_ULONG_MAX);
        }
    }
}
#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN > 0 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
