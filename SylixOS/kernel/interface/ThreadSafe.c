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
** 文   件   名: ThreadSafe.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 线程进入安全模式
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadSafe
** 功能描述: 线程进入安全模式
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
ULONG  API_ThreadSafe (VOID)
{
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    _ThreadSafeInternal();
    
    _ErrorHandle(ERROR_NONE);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
