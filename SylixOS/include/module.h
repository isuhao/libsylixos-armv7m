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
** 文   件   名: module.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 03 月 02 日
**
** 描        述: 内核模块装载器相关参数与 api.
*********************************************************************************************************/

#ifndef __MODULE_H
#define __MODULE_H

#ifdef __GNUC__
#if __GNUC__ < 2  || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#ifndef __attribute__
#define __attribute__(args)
#endif                                                                  /*  __attribute__               */
#endif                                                                  /*  __GNUC__ < 2  || ...        */
#endif                                                                  /*  __GNUC__                    */

/*********************************************************************************************************
  内核模块符号导出
*********************************************************************************************************/

#define LW_SYMBOL_KERNEL_SECTION        ".__sylixos_kernel"

/*********************************************************************************************************
  内核模块引出符号声明
  
  例如: 
        LW_SYMBOL_EXPORT int foo ()  -> 此函数装载到内核模块, 符号整个内核可见
        {
            ...;
        }
        
        int foo ()                   -> 此函数符号内核内不可见. 如当做动态库使用, 符号可见.
        {
            ...;
        }
        
        LW_SYMBOL_LOCAL int foo ()   -> 此函数符号内核内不可见, 如当做动态库使用, 符号也不可见.
        {
            ...;
        }
*********************************************************************************************************/

#define LW_SYMBOL_EXPORT                __attribute__((section(LW_SYMBOL_KERNEL_SECTION)))
#define LW_SYMBOL_LOCAL                 __attribute__((visibility("hidden")))

/*********************************************************************************************************
  内核模块初始化函数如卸载函数 (这是内核模块的入口和出口函数, 每个模块都应该具有)
  注意, 这两个函数在模块源文件中不能加 LW_SYMBOL_EXPORT 或 LW_SYMBOL_LOCAL
*********************************************************************************************************/

#define LW_MODULE_INIT                  "module_init"
#define LW_MODULE_EXIT                  "module_exit"

/*********************************************************************************************************
  注册和解除注册内核模块
*********************************************************************************************************/

#define API_ModuleRegister(pcFile)                      \
        API_ModuleLoadEx(pcFile,                        \
                         LW_OPTION_LOADER_SYM_GLOBAL,   \
                         LW_MODULE_INIT,                \
                         LW_MODULE_EXIT,                \
                         LW_NULL,                       \
                         LW_SYMBOL_KERNEL_SECTION,      \
                         LW_NULL)
                         
#define API_ModuleUnregister(pvModule)                  \
        API_ModuleUnload(pvModule)
        
#define moduleRegister                  API_ModuleRegister
#define moduleUnregister                API_ModuleUnregister

#endif                                                                  /*  __MODULE_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
