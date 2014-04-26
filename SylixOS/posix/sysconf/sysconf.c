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
** 文   件   名: sysconf.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 12 月 10 日
**
** 描        述: 兼容 posix sysconf 库. 获取当前系统配置信息
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
#include "limits.h"
#include "mqueue.h"
#include "sys/statvfs.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
** 函数名称: sysconf
** 功能描述: get configurable system variables
** 输　入  : name          represents the system variable to be queried
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
long  sysconf (int name)
{
    switch (name) {
    
    case _SC_AIO_LISTIO_MAX:
        return  (__ARCH_LONG_MAX);
        
    case _SC_AIO_MAX:
        return  (__ARCH_LONG_MAX);
        
    case _SC_ASYNCHRONOUS_IO:
        return  (1);
        
    case _SC_ARG_MAX:
        return  (LW_CFG_SHELL_MAX_KEYWORDLEN);
        
    case _SC_CLK_TCK:
        return  (LW_CFG_TICKS_PER_SEC);
        
    case _SC_DELAYTIMER_MAX:
        return  (__ARCH_INT_MAX);
        
    case _SC_IOV_MAX:
        return  (__ARCH_LONG_MAX);
        
    case _SC_LINE_MAX:
        return  (LW_CFG_SHELL_MAX_COMMANDLEN);
        
    case _SC_LOGIN_NAME_MAX:
        return  (PATH_MAX);
        
    case _SC_MQ_OPEN_MAX:
        return  (LW_CFG_MAX_EVENTS / 3);
        
    case _SC_MQ_PRIO_MAX:
        return  (MQ_PRIO_MAX);
        
    case _SC_OPEN_MAX:
        return  (LW_CFG_MAX_FILES);
        
    case _SC_PAGESIZE:
        return  (LW_CFG_VMM_PAGE_SIZE);
        
    case _SC_PRIORITY_SCHEDULING:
        return  (1);
        
    case _SC_RTSIG_MAX:
        return  (SIGRTMAX - SIGRTMIN);
        
    case _SC_REALTIME_SIGNALS:
        return  (1);
        
    case _SC_SEM_NSEMS_MAX:
        return  (LW_CFG_MAX_EVENTS);
        
    case _SC_SEM_VALUE_MAX:
        return  (__ARCH_UINT_MAX);
        
    case _SC_SEMAPHORES:
        return  (1);
        
    case _SC_SIGQUEUE_MAX:
        return  (LW_CFG_MAX_MSGQUEUES);
        
    case _SC_TIMER_MAX:
        return  (LW_CFG_MAX_TIMERS);
        
    case _SC_TIMERS:
        return  (1);
        
    case _SC_TZNAME_MAX:
        return  (10);                                                   /*  __TIMEZONE_BUFFER_SIZE      */
        
    case _SC_VERSION:
        return  (_POSIX_VERSION);
        
    case _SC_NPROCESSORS_CONF:
        return  (LW_NCPUS);
        
    case _SC_NPROCESSORS_ONLN:
        return  (LW_NCPUS);
        
    case _SC_THREAD_DESTRUCTOR_ITERATIONS:
        return  (__ARCH_LONG_MAX);
        
    case _SC_THREAD_KEYS_MAX:
        return  (__ARCH_LONG_MAX);
        
    case _SC_THREAD_STACK_MIN:
        return  (LW_CFG_PTHREAD_DEFAULT_STK_SIZE);
        
    case _SC_THREAD_THREADS_MAX:
        return  (LW_CFG_MAX_THREADS);
        
    case _SC_THREADS:
        return  (1);
        
    case _SC_THREAD_ATTR_STACKADDR:
        return  (1);
        
    case _SC_THREAD_ATTR_STACKSIZE:
        return  (1);
        
    case _SC_THREAD_PRIORITY_SCHEDULING:
        return  (1);
        
    case _SC_THREAD_PRIO_INHERIT:
        return  (1);
        
    case _SC_THREAD_PRIO_PROTECT:
        return  (1);
        
    case _SC_TTY_NAME_MAX:
        return  (PATH_MAX);
        
    case _SC_SYNCHRONIZED_IO:
        return  (1);
        
    case _SC_FSYNC:
        return  (1);
        
    case _SC_MAPPED_FILES:
        return  (1);
        
    default:
        errno = EINVAL;
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: fpathconf
** 功能描述: get configurable pathname variables
**           fd            file descriptor
** 输　入  : name          represents the system variable to be queried
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
long  fpathconf (int fd, int name)
{
    struct statvfs statvfsBuf;
    
    switch (name) {
    
    case _PC_LINK_MAX:
        return  (_S_iIoMaxLinkLevels);
        
    case _PC_MAX_CANON:
        return  (MAX_CANON);
        
    case _PC_MAX_INPUT:
        return  (MAX_INPUT);
        
    case _PC_NAME_MAX:
        return  (NAME_MAX);
        
    case _PC_PATH_MAX:
        return  (PATH_MAX);
        
    case _PC_PIPE_BUF:
        return  (PIPE_BUF);
        
    case _PC_CHOWN_RESTRICTED:
        return  (_POSIX_CHOWN_RESTRICTED);
    
    case _PC_NO_TRUNC:
        return  (_POSIX_NO_TRUNC);
    
    case _PC_VDISABLE:
        return  (_POSIX_VDISABLE);
    
    case _PC_SYNC_IO:
        return  (OPEN_MAX);
    
    case _PC_ASYNC_IO:
        return  (AIO_MAX);
    
    case _PC_PRIO_IO:
        return  (0);
    
    case _PC_SOCK_MAXBUF:
        return  (-1);
    
    case _PC_FILESIZEBITS:
        return  (FILESIZEBITS);
    
    case _PC_REC_INCR_XFER_SIZE:
        return  (-1);
    
    case _PC_REC_MAX_XFER_SIZE:
        return  (-1);
    
    case _PC_REC_MIN_XFER_SIZE:
        if (fstatvfs(fd, &statvfsBuf) < 0) {
            return  (-1);
        }
        return  (statvfsBuf.f_bsize);
    
    case _PC_REC_XFER_ALIGN:                                            /*  same as _PC_ALLOC_SIZE_MIN  */
    case _PC_ALLOC_SIZE_MIN:
        if (fstatvfs(fd, &statvfsBuf) < 0) {
            return  (-1);
        }
        return  (statvfsBuf.f_frsize);
    
    case _PC_SYMLINK_MAX:
        return  (_S_iIoMaxLinkLevels);
    
    case _PC_2_SYMLINKS:
        return  (_POSIX2_SYMLINKS);
    
    default:
        errno = EINVAL;
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: pathconf
** 功能描述: get configurable pathname variables
** 输　入  : path          pathname
**           name          represents the system variable to be queried
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
long  pathconf (const char *path, int name)
{
    int     iFd;
    long    lRet;
    
    iFd = open(path, O_RDONLY);
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    lRet = fpathconf(iFd, name);
    
    close(iFd);
    
    return  (lRet);
}
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
