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
** 文   件   名: InterShow.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 03 月 04 日
**
** 描        述: 打印操作系统中断向量表信息, (打印到标准输出终端上)

** BUG:
2009.06.25  加入对中断状态的打印.
2009.12.11  仅打印有效的中断.
2010.02.24  修改打印字符.
2011.03.31  支持 inter queue 类型打印.
2013.12.12  升级为新的向量中断系统.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
/*********************************************************************************************************
  显示中断向量表的过程中, 不能执行删除操作
*********************************************************************************************************/
extern LW_OBJECT_HANDLE    _K_ulInterShowLock;
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define INTER_SHOWLOCK_LOCK()       \
        API_SemaphoreMPend(_K_ulInterShowLock, LW_OPTION_WAIT_INFINITE)
#define INTER_SHOWLOCK_UNLOCK()     \
        API_SemaphoreMPost(_K_ulInterShowLock)
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static const CHAR   _G_cInterInfoHdr[] = "\n\
 IRQ      NAME        ENTRY   CLEAR    PARAM   ENABLE RND PREEMPTIVE\n\
---- -------------- -------- -------- -------- ------ --- ----------\n";
#if LW_CFG_INTER_INFO > 0
static const CHAR   _G_cNestingInfoHdr[] = "\n\
 CPU  MAX NESTING\n\
----- -----------\n";
#endif                                                                  /*  LW_CFG_INTER_INFO > 0       */
/*********************************************************************************************************
** 函数名称: API_InterShow
** 功能描述: 显示中断向量表的所有内容
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID   API_InterShow (VOID)
{
    INT        i;
    BOOL       bIsEnable = LW_FALSE;
    PCHAR      pcIsEnable;
    PCHAR      pcRnd;
    PCHAR      pcPreem;
    ULONG      ulFlag;
    
    PLW_CLASS_INTDESC  pidesc;
    PLW_CLASS_INTACT   piaction;
    PLW_LIST_LINE      plineTemp;
        
    printf("interrupt vector show >>\n");
    printf((PCHAR)_G_cInterInfoHdr);                                    /*  打印欢迎信息                */
    
    if (_K_ulInterShowLock == LW_OBJECT_HANDLE_INVALID) {
        return;                                                         /*  还没有连接任何中断向量      */
    }
    
    INTER_SHOWLOCK_LOCK();
    
    for (i = 0; i < LW_CFG_MAX_INTER_SRC; i++) {
        API_InterVectorGetFlag((ULONG)i, &ulFlag);
        API_InterVectorIsEnable((ULONG)i, &bIsEnable);
        
        pcIsEnable = (bIsEnable)                        ? "true" : "false";
        pcRnd      = (ulFlag & LW_IRQ_FLAG_SAMPLE_RAND) ? "*"    : "";
        pcPreem    = (ulFlag & LW_IRQ_FLAG_PREEMPTIVE)  ? "*"    : "";
        
        pidesc = LW_IVEC_GET_IDESC(i);
        
        for (plineTemp  = pidesc->IDESC_plineAction;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            piaction = _LIST_ENTRY(plineTemp, LW_CLASS_INTACT, IACT_plineManage);
            printf("%4d %-14s %8lx %8lx %8lx %-6s %-3s %s\n",
                   i, 
                   piaction->IACT_cInterName, 
                   (ULONG)piaction->IACT_pfuncIsr, 
                   (ULONG)piaction->IACT_pfuncClear, 
                   (ULONG)piaction->IACT_pvArg,
                   pcIsEnable, 
                   pcRnd, 
                   pcPreem);
        }
    }
    
    INTER_SHOWLOCK_UNLOCK();
    
    printf("\n");
#if LW_CFG_INTER_INFO > 0
    printf("interrupt nesting show >>\n");
    printf((PCHAR)_G_cNestingInfoHdr);                                  /*  打印欢迎信息                */
    
    for (i = 0; i < LW_NCPUS; i++) {
        printf("%5d %11ld\n", i, LW_CPU_GET_NESTING_MAX(i));
    }
    
    printf("\ninterrupt vector base : 0x%08lx\n", 
           (ULONG)API_InterVectorBaseGet());                            /*  中断向量表地址              */
    printf("\n");
#endif                                                                  /*  LW_CFG_INTER_INFO > 0       */
}
#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
