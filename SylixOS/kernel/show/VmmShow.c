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
** 文   件   名: pageLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 12 月 09 日
**
** 描        述: 平台无关虚拟内存管理, 基础页操作库.

** BUG:
2009.09.30  显示 virtuals 时, 更加详细.
2009.11.13  加入 DMA 域打印与利用率打印.
2011.05.17  加入缺页中断信息显示.
2011.08.03  当虚拟页面没有 Link 物理页面时, 需要使用页表查询来确定映射的物理地址.
2013.06.04  显示虚拟空间信息时, 不再显示物理地址.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
#if LW_CFG_VMM_EN > 0
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static const CHAR   _G_cZoneInfoHdr[] = "\n\
ZONE PHYSICAL   SIZE   PAGESIZE    PGD   FREEPAGE  DMA  USED\n\
---- -------- -------- -------- -------- -------- ----- ----\n";
static const CHAR   _G_cAreaInfoHdr[] = "\n\
VIRTUAL    SIZE   WRITE CACHE\n\
-------- -------- ----- -----\n";
/*********************************************************************************************************
  全局变量声明
*********************************************************************************************************/
extern LW_OBJECT_HANDLE     _G_ulVmmLock;
extern LW_VMM_ZONE          _G_vmzonePhysical[LW_CFG_VMM_ZONE_NUM];     /*  物理区域                    */
#define __VMM_LOCK()        API_SemaphoreMPend(_G_ulVmmLock, LW_OPTION_WAIT_INFINITE)
#define __VMM_UNLOCK()      API_SemaphoreMPost(_G_ulVmmLock)
/*********************************************************************************************************
  内部函数声明
*********************************************************************************************************/
extern ULONG                __vmmLibVirtualToPhysical(addr_t  ulVirtualAddr, addr_t  *pulPhysical);
/*********************************************************************************************************
** 函数名称: API_VmmPhysicalShow
** 功能描述: 显示 vmm 物理存储器信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_VmmPhysicalShow (VOID)
{
    REGISTER INT                i;
             PLW_MMU_CONTEXT    pmmuctx = __vmmGetCurCtx();
             PCHAR              pcDma;
             size_t             stUsed;

    printf("vmm physical zone show >>\n");
    printf((PCHAR)_G_cZoneInfoHdr);                                     /*  打印欢迎信息                */
    
    __VMM_LOCK();
    for (i = 0; i < LW_CFG_VMM_ZONE_NUM; i++) {
        pcDma  = (_G_vmzonePhysical[i].ZONE_uiAttr & LW_ZONE_ATTR_DMA) ? "true" : "false";
        stUsed = (_G_vmzonePhysical[i].ZONE_stSize 
               - (size_t)(_G_vmzonePhysical[i].ZONE_ulFreePage * LW_CFG_VMM_PAGE_SIZE));
        stUsed = (stUsed / (_G_vmzonePhysical[i].ZONE_stSize / 100));   /*  防止溢出                    */
        
        printf("%4d %08lx %8zx %8zx %08lx %8ld %-5s %3zd%%\n",
                      i, 
                      _G_vmzonePhysical[i].ZONE_ulAddr,
                      _G_vmzonePhysical[i].ZONE_stSize,
                      (size_t)LW_CFG_VMM_PAGE_SIZE,
                      (addr_t)pmmuctx->MMUCTX_pgdEntry,
                      _G_vmzonePhysical[i].ZONE_ulFreePage,
                      pcDma,
                      stUsed);
    }
    __VMM_UNLOCK();
    
    printf("\n");
}
/*********************************************************************************************************
** 函数名称: __vmmVirtualPrint
** 功能描述: 打印信息回调函数
** 输　入  : pvmpage  页面信息控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __vmmVirtualPrint (PLW_VMM_PAGE  pvmpage)
{
    addr_t  ulVirtualAddr  = pvmpage->PAGE_ulPageAddr;
    
    printf("%08lx %8lx ", ulVirtualAddr, (pvmpage->PAGE_ulCount * LW_CFG_VMM_PAGE_SIZE));
                  
    if (pvmpage->PAGE_ulFlags & LW_VMM_FLAG_WRITABLE) {
        printf("true  ");
    } else {
        printf("false ");
    }
    if (pvmpage->PAGE_ulFlags & LW_VMM_FLAG_CACHEABLE) {
        printf("true\n");
    } else {
        printf("false\n");
    }
}
/*********************************************************************************************************
** 函数名称: API_VmmVirtualShow
** 功能描述: 显示 vmm 虚拟存储器信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_VmmVirtualShow (VOID)
{
    printf("vmm virtual area show >>\n");
    printf("vmm virtual area from : 0x%08lx, size : 0x%08lx\n", 
                  (addr_t)LW_CFG_VMM_VIRTUAL_START,
                  (addr_t)LW_CFG_VMM_VIRTUAL_SIZE);
    printf("vmm virtual area usage as follow :\n");
                  
    printf((PCHAR)_G_cAreaInfoHdr);                                     /*  打印欢迎信息                */
    
    __VMM_LOCK();
    __areaVirtualSpaceTraversal(__vmmVirtualPrint);                     /*  遍历虚拟空间树, 打印信息    */
    __VMM_UNLOCK();
    
    printf("\n");
}
/*********************************************************************************************************
** 函数名称: API_VmmAbortShow
** 功能描述: 显示 vmm 访问中止信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_VmmAbortShow (VOID)
{
    INT64     i64AbortCounter;
    INT64     i64PageFailCounter;
    INT64     i64PageLackCounter;
    INT64     i64MapErrCounter;
    
    API_VmmAbortStatus(&i64AbortCounter,
                       &i64PageFailCounter,
                       &i64PageLackCounter,
                       &i64MapErrCounter);

    printf("vmm abort statistics infomation show >>\n");
    printf("vmm abort (memory access error) counter : %lld\n", i64AbortCounter);
    printf("vmm page fail (alloc success) counter   : %lld\n", i64PageFailCounter);
    printf("vmm alloc physical page error counter   : %lld\n", i64PageLackCounter);
    printf("vmm page map error counter              : %lld\n", i64MapErrCounter);
    
    printf("\n");
}
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
