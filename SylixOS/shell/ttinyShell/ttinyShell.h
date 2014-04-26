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
** 文   件   名: ttinyShell.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 07 月 27 日
**
** 描        述: 一个超小型的 shell 系统, 使用 tty/pty 做接口, 主要用于调试与简单交互.
*********************************************************************************************************/

#ifndef __TTINYSHELL_H
#define __TTINYSHELL_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

/*********************************************************************************************************
  回调函数类型
*********************************************************************************************************/

typedef INT               (*PCOMMAND_START_ROUTINE)(INT  iArgC, PCHAR  ppcArgV[]);

/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API VOID                 API_TShellInit(VOID);                       /*  安装 tshell 系统            */

LW_API INT                  API_TShellSetStackSize(size_t  stNewSize, size_t  *pstOldSize);
                                                                        /*  设置堆栈大小                */

LW_API LW_OBJECT_HANDLE     API_TShellCreate(INT  iTtyFd, 
                                             ULONG  ulOption);          /*  创建一个 tshell 终端        */

LW_API LW_OBJECT_HANDLE     API_TShellCreateEx(INT      iTtyFd, 
                                               ULONG    ulOption,
                                               FUNCPTR  pfuncRunCallback);
                                                                        /*  创建一个 tshell 终端扩展    */

LW_API INT                  API_TShellGetUserName(uid_t  uid, PCHAR  pcName, size_t  stSize);
                                                                        /*  通过 shell 缓冲获取用户名   */
LW_API INT                  API_TShellGetGrpName(gid_t  gid, PCHAR  pcName, size_t  stSize);
                                                                        /*  通过 shell 缓冲获取组名     */
LW_API VOID                 API_TShellFlushCache(VOID);                 /*  刷新 shell 名字缓冲         */

LW_API ULONG                API_TShellKeywordAdd(CPCHAR  pcKeyword, 
                                                 PCOMMAND_START_ROUTINE  pfuncCommand);  
                                                                        /*  向 tshell 系统中添加关键字  */
LW_API ULONG                API_TShellKeywordAddEx(CPCHAR  pcKeyword, 
                                                   PCOMMAND_START_ROUTINE  pfuncCommand, 
                                                   ULONG  ulOption);    /*  向 tshell 系统中添加关键字  */
LW_API ULONG                API_TShellFormatAdd(CPCHAR  pcKeyword, CPCHAR  pcFormat);
                                                                        /*  向某一个关键字添加格式帮助  */
LW_API ULONG                API_TShellHelpAdd(CPCHAR  pcKeyword, CPCHAR  pcHelp);
                                                                        /*  向某一个关键字添加帮助      */
                                                                        
LW_API INT                  API_TShellExec(CPCHAR  pcCommandExec);      /*  在当前的环境中, 执行一条    */
                                                                        /*  shell 指令                  */
LW_API INT                  API_TShellExecBg(CPCHAR  pcCommandExec, INT  iFd[3], BOOL  bClosed[3], 
                                             BOOL  bIsJoin, LW_OBJECT_HANDLE *pulSh);
                                                                        /*  背景执行一条 shell 命令     */
LW_API INT                  API_TShellCtlCharSend(ULONG  ulFunc, PCHAR  pcArg);
                                                                        /*  发送一个转义序列            */
                                                                        
#define tshellInit          API_TShellInit
#define tshellSetStackSize  API_TShellSetStackSize
#define tshellCreate        API_TShellCreate
#define tshellCreateEx      API_TShellCreateEx
#define tshellGetUserName   API_TShellGetUserName
#define tshellGetGrpName    API_TShellGetGrpName
#define tshellFlushCache    API_TShellFlushCache
#define tshellKeywordAdd    API_TShellKeywordAdd
#define tshellKeywordAddEx  API_TShellKeywordAddEx
#define tshellFormatAdd     API_TShellFormatAdd
#define tshellHelpAdd       API_TShellHelpAdd
#define tshellExec          API_TShellExec
#define tshellExecBg        API_TShellExecBg
#define tshellCtlCharSend   API_TShellCtlCharSend
                                                                        
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  __TTINYSHELL_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
