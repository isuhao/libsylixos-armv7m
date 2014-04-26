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
** 文   件   名: InterLock.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 18 日
**
** 描        述: 系统关闭中断
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_InterLock
** 功能描述: 系统关闭中断
** 输　入  : piregInterLevel   保存中断控制字
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_InterLock (INTREG  *piregInterLevel)
{
#if LW_CFG_ARG_CHK_EN > 0
    if (!piregInterLevel) {
        _ErrorHandle(ERROR_INTER_LEVEL_NULL);
        return  (ERROR_INTER_LEVEL_NULL);
    }
#endif
    
    _ErrorHandle(ERROR_NONE);
    
    *piregInterLevel = KN_INT_DISABLE();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_InterUnlock
** 功能描述: 系统打开中断
** 输　入  : iregInterLevel    需要恢复的中断控制字
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_InterUnlock (INTREG  iregInterLevel)
{
    KN_INT_ENABLE(iregInterLevel);
    
    _ErrorHandle(ERROR_NONE);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

