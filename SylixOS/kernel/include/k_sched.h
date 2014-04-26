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
** 文   件   名: k_sched.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 这是系统调度器函数头文件

** BUG:
2009.04.11  全面向 SMP 系统升级调度器, 而且不像 tkernel/linux 等系统不能保证实时性, 这里虽然使用多核, 调度
            器仍然是硬实时的! (tkernel/linux 的 SMP 版不支持完全的实时调度, tkernel 只能在另外一种 SMP 模
            式下才能实现核间各自独立的实时调度.)
            
            RDY_MAP() 不再是所有就绪任务的表, 而是通过位图存储: 就绪但是没有运行或没有即将运行可能的线程的
            位图表.
            (RDY_MAP -> 就绪但没有机会运行).
            有机会运行的线程都在 LW_CAND_TCB(x) 中.
            仅前 LW_NCPUS 个有效.
            
            SylixOS 中的 scheduler 支持单核与 SMP 多核处理器. 同时这个调度器将完全满足硬实时调度要求, 
            调度器对于线程的数量来说, 时间复杂度为 O(1), 但是对于 SMP 处理器个数来说是 O(N) 的. 此调度器
            可以满足任何情况下所有的 CPU 均运行具有最高优先级的线程.
2009.10.11  当线程进入运行表时, 一定要保存正确的 CPU ID, 在退出运行表时, 就可以不用扫描运行表, 而直接使用
            CPU ID 定位.
2011.07.06  修正多核情况下 __try_add_to_run_table() 判断最低优先级问题.
2011.12.05  将候选运行表操作函数转移到 _CandTable.c 中.
2013.07.29  针对 SMP 重新设计新的候选运行表.
*********************************************************************************************************/

#ifndef  __K_SCHED_H
#define  __K_SCHED_H

/*********************************************************************************************************
  候选运行表基本操作
*********************************************************************************************************/

#define LW_CAND_TCB(pcpu)   pcpu->CPU_cand.CAND_ptcbCand
#define LW_CAND_ROT(pcpu)   pcpu->CPU_cand.CAND_bNeedRotate

/*********************************************************************************************************
  判断线程是否就绪和是否正在运行
*********************************************************************************************************/

#define __LW_THREAD_IS_READY(ptcb)  \
        (ptcb->TCB_usStatus == LW_THREAD_STATUS_RDY)
        
#define __LW_THREAD_IS_RUNNING(ptcb)  \
        (LW_CPU_GET(ptcb->TCB_ulCPUId)->CPU_ptcbTCBCur == ptcb)

/*********************************************************************************************************
  运行表相应操作函数声明
*********************************************************************************************************/

BOOL  _CandTableTryAdd(PLW_CLASS_TCB  ptcb, PLW_CLASS_PCB  ppcb);
VOID  _CandTableTryDel(PLW_CLASS_TCB  ptcb, PLW_CLASS_PCB  ppcb);
VOID  _CandTableUpdate(PLW_CLASS_CPU  pcpu);
VOID  _CandTableRemove(PLW_CLASS_CPU  pcpu);

/*********************************************************************************************************
  加入 / 退出就绪表, 不加调度器锁
*********************************************************************************************************/

#define __ADD_TO_READY_RING_NOLOCK(ptcb, ppcb)                  \
        do {                                                    \
            (ppcb)->PCB_usThreadReadyCounter++;                 \
            if (!_CandTableTryAdd((ptcb), (ppcb))) {            \
                _AddTCBToReadyRing((ptcb), (ppcb), LW_FALSE);   \
            }                                                   \
        } while (0)
        
#define __DEL_FROM_READY_RING_NOLOCK(ptcb, ppcb)                \
        do {                                                    \
            (ppcb)->PCB_usThreadReadyCounter--;                 \
            if (!(ptcb)->TCB_bIsCand) {                         \
                _DelTCBFromReadyRing((ptcb), (ppcb));           \
            }                                                   \
            _CandTableTryDel((ptcb), (ppcb));                   \
        } while (0)

/*********************************************************************************************************
  加入 / 退出就绪表, 带有调度器锁
*********************************************************************************************************/

#define __ADD_TO_READY_RING(ptcb, ppcb)                         \
        do {                                                    \
            LW_SPIN_LOCK_IGNIRQ(&_K_slScheduler);               \
            __ADD_TO_READY_RING_NOLOCK(ptcb, ppcb);             \
            LW_SPIN_UNLOCK_IGNIRQ(&_K_slScheduler);             \
        } while (0)
        
#define __DEL_FROM_READY_RING(ptcb, ppcb)                       \
        do {                                                    \
            LW_SPIN_LOCK_IGNIRQ(&_K_slScheduler);               \
            __DEL_FROM_READY_RING_NOLOCK(ptcb, ppcb);           \
            LW_SPIN_UNLOCK_IGNIRQ(&_K_slScheduler);             \
        } while (0)

#endif                                                                  /*  __K_SCHED_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
