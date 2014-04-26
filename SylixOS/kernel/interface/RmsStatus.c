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
** 文   件   名: RmsStatus.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 24 日
**
** 描        述: 获得精度单调调度器的状态

** BUG
2007.11.04  加入 _DebugHandle() 功能.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  注意：
        使用精度单调调度器，不可以设置系统时间，但是可是设置 RTC 时间。
        
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: API_RmsStatus
** 功能描述: 获得精度单调调度器的状态
** 输　入  : 
**           ulId                          RMS 句柄
**           pucStatus                     名字缓冲区    可以为 NULL
**           pulTimeLeft                   等待剩余时间  可以为 NULL
**           pulOwnerId                    所有者 ID     可以为 NULL
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

LW_API  
ULONG  API_RmsStatus (LW_OBJECT_HANDLE  ulId,
                      UINT8            *pucStatus,
                      ULONG            *pulTimeLeft,
                      LW_OBJECT_HANDLE *pulOwnerId)
{
             INTREG              iregInterLevel;
    REGISTER PLW_CLASS_RMS       prms;
    REGISTER UINT16              usIndex;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_RMS)) {                           /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Rms_Index_Invalid(usIndex)) {                                  /*  缓冲区索引检查              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
    if (_Rms_Type_Invalid(usIndex)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS handle invalidate.\r\n");
        _ErrorHandle(ERROR_RMS_NULL);
        return  (ERROR_RMS_NULL);
    }
#else
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
#endif
    
    prms = &_K_rmsBuffer[usIndex];
    
    if (pucStatus) {
        *pucStatus = prms->RMS_ucStatus;
    }
    
    if (prms->RMS_ucStatus == LW_RMS_EXPIRED) {                         /*  有线程进行周期等待          */
        if (pulOwnerId) {
            *pulOwnerId = prms->RMS_ptcbOwner->TCB_ulId;
        }
        if (pulTimeLeft) {
            _WakeupStatus(&_K_wuDelay, &prms->RMS_ptcbOwner->TCB_wunDelay, pulTimeLeft);
        }
    
    } else {                                                            /*  无效状态                    */
        if (pulOwnerId) {
            *pulOwnerId = LW_OBJECT_HANDLE_INVALID;
        }
        if (pulTimeLeft) {
            *pulTimeLeft = 0ul;
        }
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核                    */
    
    _ErrorHandle(ERROR_NONE);
    return  (ERROR_NONE);
}
#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
