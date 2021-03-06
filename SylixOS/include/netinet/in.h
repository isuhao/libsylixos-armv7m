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
** 文   件   名: in.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 09 月 03 日
**
** 描        述: include/netinet/in .
*********************************************************************************************************/

#include "in6.h"

#ifndef __NETINET_IN_H
#define __NETINET_IN_H

/*********************************************************************************************************
 Local port number conventions:
 Ports < IPPORT_RESERVED are reserved for
 privileged processes (e.g. root).
 Ports > IPPORT_USERRESERVED are reserved
 for servers, not necessarily privileged.
*********************************************************************************************************/

#define IPPORT_RESERVED     1024
#define IPPORT_USERRESERVED 5000

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN     16
#endif

/*********************************************************************************************************
  SylixOS add
*********************************************************************************************************/

#define inet_network(cp)    ntohl(inet_addr(cp))

#endif                                                                  /*  __NETINET_IN_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
