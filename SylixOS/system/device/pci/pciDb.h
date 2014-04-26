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
** 文   件   名: pciDb.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 09 月 30 日
**
** 描        述: PCI 总线信息数据库.
*********************************************************************************************************/

#ifndef __PCIDB_H
#define __PCIDB_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)

VOID  __pciDbInit(VOID);
VOID  __pciDbGetClassStr(UINT8 ucClass, UINT8 ucSubClass, UINT8 ucProgIf, PCHAR pcBuffer, size_t  stSize);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCIDB_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
