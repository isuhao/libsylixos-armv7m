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
** 文   件   名: cache.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 09 月 27 日
**
** 描        述: 平台无关的高速缓冲器管理部件. BSP 或 驱动程序需要 #include "cache.h"

** BUG
2008.05.02 加入相关的注释.
2008.12.05 加入对齐 DMA 内存管理.
2009.06.18 取消 CACHE 对地址映射的管理, 完全交由 VMM 组件完成.
2013.12.20 取消 PipeFlush 功能, 由驱动统一在 DCACHE 适当的位置进行管理.
*********************************************************************************************************/

#ifndef __CACHE_H
#define __CACHE_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
/*********************************************************************************************************
  CACHE MODE DEFINE 
*********************************************************************************************************/

#define CACHE_DISABLED                  0x00                            /*  CACHE 旁路模式              */
#define CACHE_WRITETHROUGH              0x01                            /*  CACHE 写通模式              */
#define CACHE_COPYBACK                  0x02                            /*  CACHE 回写模式              */

#define CACHE_SNOOP_DISABLE             0x00                            /*  CACHE 可能与内存数据不一致  */
#define CACHE_SNOOP_ENABLE              0x10                            /*  总线检视, CACHE 与内存一致  */

/*********************************************************************************************************
  CACHE TYPE
*********************************************************************************************************/

typedef enum {
    INSTRUCTION_CACHE = 0,                                              /*  指令 CACHE                  */
    DATA_CACHE        = 1                                               /*  数据 CACHE                  */
} LW_CACHE_TYPE;

/*********************************************************************************************************
  CACHE MODE
*********************************************************************************************************/

typedef UINT    CACHE_MODE;                                             /*  CACHE 模式                  */

/*********************************************************************************************************
  CACHE OP STRUCT
*********************************************************************************************************/

typedef struct {
    INT             CACHEOP_iILoc;                                      /*  指令 cache 位置             */
    INT             CACHEOP_iDLoc;                                      /*  数据 cache 位置             */
#define CACHE_LOCATION_VIVT         0                                   /*  虚拟地址 CACHE              */
#define CACHE_LOCATION_PIPT         1                                   /*  物理地址 CACHE              */
#define CACHE_LOCATION_VIPT         2                                   /*  虚拟 INDEX 物理 TAG         */

    INT             CACHEOP_iCacheLine;                                 /*  cache line 字节数           */
    
    FUNCPTR         CACHEOP_pfuncEnable;                                /*  启动 CACHE                  */
    FUNCPTR         CACHEOP_pfuncDisable;                               /*  停止 CACHE                  */
    
    FUNCPTR         CACHEOP_pfuncLock;                                  /*  锁定 CACHE                  */
    FUNCPTR         CACHEOP_pfuncUnlock;                                /*  解锁 CACHE                  */
    
    FUNCPTR         CACHEOP_pfuncFlush;                                 /*  将 CACHE 指定内容回写内存   */
    FUNCPTR         CACHEOP_pfuncInvalidate;                            /*  使 CACHE 指定内容无效       */
    FUNCPTR         CACHEOP_pfuncClear;                                 /*  清空并无效所有 CACHE 内容   */
    FUNCPTR         CACHEOP_pfuncTextUpdate;                            /*  清空 D CACHE 无效 I CACHE   */
    
    PVOIDFUNCPTR    CACHEOP_pfuncDmaMalloc;                             /*  开辟一块非缓冲的内存        */
    PVOIDFUNCPTR    CACHEOP_pfuncDmaMallocAlign;                        /*  开辟一块非缓冲的内存(对齐)  */
    VOIDFUNCPTR     CACHEOP_pfuncDmaFree;                               /*  交还一块非缓冲的内存        */
} LW_CACHE_OP;                                                          /*  CACHE 操作库                */
      
/*********************************************************************************************************
  通用 CACHE 库初始化操作, 如果是 SMP 系统, 则只需要主核在 API_KernelStart 回调中调用即可
*********************************************************************************************************/

LW_API ULONG        API_CacheLibPrimaryInit(CACHE_MODE  uiInstruction, 
                                            CACHE_MODE  uiData, 
                                            CPCHAR      pcMachineName);

#define API_CacheLibInit    API_CacheLibPrimaryInit

#if LW_CFG_SMP_EN > 0
LW_API ULONG        API_CacheLibSecondaryInit(CPCHAR  pcMachineName);
#endif                                                                  /*  LW_CFG_SMP_EN               */

/*********************************************************************************************************
  通用 CACHE 库 API 操作
*********************************************************************************************************/

LW_API LW_CACHE_OP *API_CacheGetLibBlock(VOID);
LW_API INT          API_CacheLocation(LW_CACHE_TYPE  cachetype);
LW_API INT          API_CacheLine(VOID);

LW_API ULONG        API_CacheEnable(LW_CACHE_TYPE  cachetype);
LW_API ULONG        API_CacheDisable(LW_CACHE_TYPE cachetype);

LW_API ULONG	    API_CacheLock(LW_CACHE_TYPE   cachetype, PVOID  pvAdrs, size_t  stBytes);
LW_API ULONG	    API_CacheUnlock(LW_CACHE_TYPE cachetype, PVOID  pvAdrs, size_t  stBytes);

LW_API ULONG	    API_CacheFlush(LW_CACHE_TYPE      cachetype, PVOID  pvAdrs, size_t  stBytes);
LW_API ULONG	    API_CacheInvalidate(LW_CACHE_TYPE cachetype, PVOID  pvAdrs, size_t  stBytes);
LW_API ULONG	    API_CacheClear(LW_CACHE_TYPE cachetype, PVOID  pvAdrs, size_t  stBytes);
LW_API ULONG	    API_CacheTextUpdate(PVOID  pvAdrs, size_t  stBytes);

LW_API PVOID        API_CacheDmaMalloc(size_t   stBytes);
LW_API PVOID        API_CacheDmaMallocAlign(size_t   stBytes, size_t  stAlign);
LW_API VOID         API_CacheDmaFree(PVOID      pvBuf);

LW_API ULONG	    API_CacheDmaFlush(PVOID  pvAdrs, size_t  stBytes);
LW_API ULONG	    API_CacheDmaInvalidate(PVOID  pvAdrs, size_t  stBytes);
LW_API ULONG	    API_CacheDmaClear(PVOID  pvAdrs, size_t  stBytes);

/*********************************************************************************************************
  根据 CACHE 类型, 优化相应的函数设置.
*********************************************************************************************************/

LW_API VOID         API_CacheFuncsSet(VOID);

/*********************************************************************************************************
  VxWorks 兼容 CACHE 函数库
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL

#define cacheGetLibBlock            API_CacheGetLibBlock
#define cacheLocation               API_CacheLocation
#define cacheLine                   API_CacheLine

#define cacheEnable                 API_CacheEnable
#define cacheDisable                API_CacheDisable

#define cacheLock                   API_CacheLock
#define cacheUnlock                 API_CacheUnlock

#define cacheFlush                  API_CacheFlush
#define cacheInvalidate             API_CacheInvalidate
#define cacheClear                  API_CacheClear
#define cacheTextUpdate             API_CacheTextUpdate

#define cacheDmaMalloc              API_CacheDmaMalloc
#define cacheDmaMallocAlign         API_CacheDmaMallocAlign
#define cacheDmaFree                API_CacheDmaFree

#define cacheDmaFlush               API_CacheDmaFlush
#define cacheDmaInvalidate          API_CacheDmaInvalidate
#define cacheDmaClear               API_CacheDmaClear

#define cacheFuncsSet               API_CacheFuncsSet

#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  __CACHE_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
