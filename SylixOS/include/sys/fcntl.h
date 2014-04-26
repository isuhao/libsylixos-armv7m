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
** 文   件   名: fcntl.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 04 月 02 日
**
** 描        述: 兼容 C 库.
*********************************************************************************************************/

#ifndef __SYS_FCNTL_H
#define __SYS_FCNTL_H

#ifndef __SYLIXOS_H
#include "SylixOS.h"
#endif                                                                  /*  __SYLIXOS_H                 */

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

#if LW_CFG_DEVICE_EN > 0
extern int  fcntl(int  iFd, int  iCmd, ...);
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SYS_FCNTL_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
