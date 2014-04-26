/**********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: armSpinlock.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARM 体系构架自旋锁驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  spinlock 状态
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#include "armMpCore.h"
/*********************************************************************************************************
  L1 cache 同步请参考: http://www.cnblogs.com/jiayy/p/3246133.html
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: archSpinInit
** 功能描述: 初始化一个 spinlock
** 输　入  : psl        spinlock 指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archSpinInit (spinlock_t  *psl)
{
    psl->SL_sltData   = 0;                                              /*  0: 未锁定状态  1: 锁定状态  */
    psl->SL_pcpuOwner = LW_NULL;
    psl->SL_ulCounter = 0ul;                                            /*  重入锁计数                  */
}
/*********************************************************************************************************
** 函数名称: archSpinDelay
** 功能描述: 短延迟
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archSpinDelay (VOID)
{
    LW_SPINLOCK_DELAY();
}
/*********************************************************************************************************
** 函数名称: archSpinLock
** 功能描述: spinlock 上锁
** 输　入  : psl        spinlock 指针
** 输　出  : 0: 没有获取
**           1: 正常加锁
** 全局变量: 
** 调用模块: 
** 注  意  : 为了提高 spinlock 效率, 当检测到 psl->SL_sltData 为空闲时, 才调用 SWAP 操作.
*********************************************************************************************************/
INT  archSpinLock (spinlock_t  *psl)
{
    if (psl->SL_pcpuOwner == LW_CPU_GET_CUR()) {
        psl->SL_ulCounter++;
        return  (1);                                                    /*  重复调用                    */
    }
    
    armSpinLock(&psl->SL_sltData);
    
    psl->SL_pcpuOwner = LW_CPU_GET_CUR();                               /*  保存当前 CPU                */
    
    return  (1);                                                        /*  加锁成功                    */
}
/*********************************************************************************************************
** 函数名称: archSpinTryLock
** 功能描述: spinlock 试图上锁
** 输　入  : psl        spinlock 指针
** 输　出  : 0: 没有获取
**           1: 正常加锁
** 全局变量: 
** 调用模块: 
** 注  意  : 当使用 LW_CFG_ARM_SPIN_BSP 时, trylock 必须保证等到.
*********************************************************************************************************/
INT  archSpinTryLock (spinlock_t  *psl)
{
    if (psl->SL_pcpuOwner == LW_CPU_GET_CUR()) {
        psl->SL_ulCounter++;
        return  (1);                                                    /*  重复调用                    */
    }
    
    if (armSpinTryLock(&psl->SL_sltData)) {                             /*  尝试加锁                    */
        return  (0);
    }
    
    psl->SL_pcpuOwner = LW_CPU_GET_CUR();                               /*  保存当前 CPU                */
    
    return  (1);
}
/*********************************************************************************************************
** 函数名称: archSpinUnlock
** 功能描述: spinlock 解锁
** 输　入  : psl        spinlock 指针
** 输　出  : 0: 没有获取
**           1: 正常解锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archSpinUnlock (spinlock_t  *psl)
{
    if (psl->SL_pcpuOwner != LW_CPU_GET_CUR()) {
        return  (0);                                                    /*  没有权利释放                */
    }
    
    if (psl->SL_ulCounter) {
        psl->SL_ulCounter--;                                            /*  减少重复调用次数            */
        return  (1);
    }

    psl->SL_pcpuOwner = LW_NULL;                                        /*  没有 CPU 获取               */
    
    armSpinUnlock(&psl->SL_sltData);                                    /*  解锁                        */
    
    return  (1);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
