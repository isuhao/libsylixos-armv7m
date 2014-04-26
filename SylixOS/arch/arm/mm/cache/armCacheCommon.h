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
** 文   件   名: armCacheCommon.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARM 体系架构 CACHE 通用函数支持.
*********************************************************************************************************/

#ifndef __ARMCACHECOMMON_H
#define __ARMCACHECOMMON_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0

UINT32  armCacheTypeReg(VOID);
VOID    armCacheRetireRR(VOID);
VOID    armCacheRetireDefault(VOID);
VOID    armICacheEnable(VOID);
VOID    armDCacheEnable(VOID);
VOID    armICacheDisable(VOID);
VOID    armICacheInvalidate(PVOID  pvAdrs, size_t  stBytes);
VOID    armDCacheFlushMVA(PVOID  pvAdrs);

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  __ARMCACHECOMMON_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
