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
** 文   件   名: coresdLib.c
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2010 年 11 月 23 日
**
** 描        述: sd卡特殊操作接口源文件

** BUG:
2010.11.23 修改__sdCoreDevSendIfCond(),在SPI和SD模式下,检验码在应答中的位置不同.
2010.11.27 增加了几个API.
2010.12.02 由于最初测试不周密, 发现__getBits() 函数有错误,改正之.
2011.02.12 更改SD卡复位函数__sdCoreDevReset().对其增加SPI模式下的复位操作.
2011.02.21 更改__sdCoreDevSendAllCSD() 和 __sdCoreDevSendAllCID()以适应spi模式.
2011.02.21 更改__sdCoreDevGetStatus()函数.今天才发现cmd13是一个app命令,对一些卡来说,cmd13可以当作一般命令
           发送没有问题,但有的卡就不行了.
2011.03.30 增加__sdCoreDevMmcSetRelativeAddr () 以支持MMC.
2011.03.30 修改__sdCoreDecodeCID(),加入对MMC卡CID的解析
2011.04.12 修改__sdCoreDevSendAppOpCond().其传入的参数uiOCR为主控支持的电压,但是对于memory卡,其卡电压是
           有一定范围要求的.所以在发送命令时,将uiOCR进行处理后再作为参数发送.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "spec_sd.h"
/*********************************************************************************************************
 CSD 中TACC域中的指数值(以纳秒为单位)和系数(为了避免使用浮点,已经乘以了10)查找表
*********************************************************************************************************/
static const UINT32    CSD_TACC_EXP[] = {
        1, 10, 100, 1000, 10000, 100000, 1000000, 10000000
};
static const UINT32    CSD_TACC_MNT[] = {
        0,  10, 12, 13, 15, 20, 25, 30,
        35, 40, 45, 50, 55, 60, 70, 80
};
/*********************************************************************************************************
 CSD 中TRAN_SPEED域中的指数(以bit/s为单位)值和系数(为了避免使用浮点,已经乘以了10)查找表
*********************************************************************************************************/
static const UINT32    CSD_TRSP_EXP[] = {
        10000, 100000, 1000000, 10000000,
            0,      0,       0,        0
};
static const UINT32    CSD_TRSP_MNT[] = {
        0,  10, 12, 13, 15, 20, 25, 30,
        35, 40, 45, 50, 55, 60, 70, 80
};
/*********************************************************************************************************
** 函数名称: __getBits
** 功能描述: 在应答内容中,获得指定位的数据.
**           原始的应答数据是 UINT32类型,大小固定为4个元素(128位).
**           !!!注意：函数未作参数检查,调用时应该注意.
** 输    入: puiResp       应答的原始数据
**           uiStart       数据起始位置(位)
**                         !!!注意: uiStart为0 对应原始数据最后一个元素的最低位.
**                         如RSP[4] = {0,0,0,0X0000003F}  则 __getBits(RSP, 0, 8) 的值为0x3F.
**           uiSize        数据宽度(位)
**           pcDeviceName  设备名称
** 输    出: NONE
** 返    回: 最终提取的数据(最大为32位).
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32 __getBits (UINT32  *puiResp, UINT32 uiStart, UINT32 uiSize)
{
    UINT32  uiRes;
    UINT32  uiMask  = (uiSize < 32 ? 1 << uiSize : 0) - 1;
    INT     iOff    = 3 - (uiStart / 32);
    INT     iSft    = uiStart & 31;

    uiRes = puiResp[iOff] >> iSft;
    if (uiSize + iSft > 32) {
        uiRes |= puiResp[iOff - 1] << ((32 - iSft) % 32);
    }
    uiRes &= uiMask;

    return  (uiRes);
}
/*********************************************************************************************************
** 函数名称: __printCID
** 功能描述: 打印CID
** 输    入: psdcid
**           ucType
** 输    出: NONE
** 返    回: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#ifdef  __SYLIXOS_DEBUG

static void  __printCID (LW_SDDEV_CID *psdcid, UINT8 ucType)
{
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */

#define __CID_PNAME(iN)   (psdcid->DEVCID_pucProductName[iN])

    printk("\nCID INFO >>\n");
    printk("Manufacturer :  %08x\n", psdcid->DEVCID_ucMainFid);
    if (ucType == SDDEV_TYPE_MMC) {
        printk("OEM ID       :  %08x\n", psdcid->DEVCID_usOemId);
    } else {
        printk("OEM ID       :  %c%c\n", psdcid->DEVCID_usOemId >> 8,
                                     psdcid->DEVCID_usOemId & 0xff);
    }
    printk("Product Name :  %c%c%c%c%c\n",
                                        __CID_PNAME(0),
                                        __CID_PNAME(1),
                                        __CID_PNAME(2),
                                        __CID_PNAME(3),
                                        __CID_PNAME(4));
    printk("Product Vsn  :  %02d.%02d\n", psdcid->DEVCID_ucProductVsn >> 4,
                                          psdcid->DEVCID_ucProductVsn & 0xf);
    printk("Serial Num   :  %08x\n", psdcid->DEVCID_uiSerialNum);
    printk("Year         :  %04d\n", psdcid->DEVCID_uiYear);
    printk("Month        :  %02d\n", psdcid->DEVCID_ucMonth);
}

#endif                                                                  /*  __SYLIXOS_DEBUG             */
/*********************************************************************************************************
** 函数名称: __sdCoreDecodeCID
** 功能描述: 解码CID
** 输    入: pRawCID         原始CID数据
**           ucType          卡的类型
** 输    出: psdcidDec       解码后的CID
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __sdCoreDecodeCID (LW_SDDEV_CID  *psdcidDec, UINT32 *pRawCID, UINT8 ucType)
{
    if (!psdcidDec || !pRawCID) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    lib_bzero(psdcidDec, sizeof(LW_SDDEV_CID));

    switch (ucType) {
    case SDDEV_TYPE_MMC:
        psdcidDec->DEVCID_ucMainFid          =  __getBits(pRawCID, 120, 8);
        psdcidDec->DEVCID_usOemId            =  __getBits(pRawCID, 104, 16);
        psdcidDec->DEVCID_pucProductName[0]  =  __getBits(pRawCID, 88,  8);
        psdcidDec->DEVCID_pucProductName[1]  =  __getBits(pRawCID, 80,  8);
        psdcidDec->DEVCID_pucProductName[2]  =  __getBits(pRawCID, 72,  8);
        psdcidDec->DEVCID_pucProductName[3]  =  __getBits(pRawCID, 64,  8);
        psdcidDec->DEVCID_pucProductName[4]  =  __getBits(pRawCID, 56,  8);
        psdcidDec->DEVCID_ucProductVsn       =  __getBits(pRawCID, 48,  8);
        psdcidDec->DEVCID_uiSerialNum        =  __getBits(pRawCID, 16,  32);
        psdcidDec->DEVCID_ucMonth            =  __getBits(pRawCID, 12,  4);
        psdcidDec->DEVCID_uiYear             =  __getBits(pRawCID, 8,   4);
        psdcidDec->DEVCID_uiYear            +=  1997;
        break;

    default:
        psdcidDec->DEVCID_ucMainFid          =  __getBits(pRawCID, 120, 8);
        psdcidDec->DEVCID_usOemId            =  __getBits(pRawCID, 104, 16);
        psdcidDec->DEVCID_pucProductName[0]  =  __getBits(pRawCID, 96,  8);
        psdcidDec->DEVCID_pucProductName[1]  =  __getBits(pRawCID, 88,  8);
        psdcidDec->DEVCID_pucProductName[2]  =  __getBits(pRawCID, 80,  8);
        psdcidDec->DEVCID_pucProductName[3]  =  __getBits(pRawCID, 72,  8);
        psdcidDec->DEVCID_pucProductName[4]  =  __getBits(pRawCID, 64,  8);
        psdcidDec->DEVCID_ucProductVsn       =  __getBits(pRawCID, 56,  8);
        psdcidDec->DEVCID_uiSerialNum        =  __getBits(pRawCID, 24,  32);
        psdcidDec->DEVCID_uiYear             =  __getBits(pRawCID, 12,  8);
        psdcidDec->DEVCID_ucMonth            =  __getBits(pRawCID, 8,   4);
        psdcidDec->DEVCID_uiYear            +=  2000;
        break;
    }

#ifdef  __SYLIXOS_DEBUG
    __printCID(psdcidDec, ucType);
#endif                                                                  /*  __SYLIXOS_DEBUG             */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __printCSD
** 功能描述: 打印CSD
** 输    入: psdcsd
** 输    出: NONE
** 返    回: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#ifdef  __SYLIXOS_DEBUG

static void  __printCSD (LW_SDDEV_CSD *psdcsd)
{
#ifndef printk
#define printk
#endif                                                                  /*  printk                      */

    printk("\nCSD INFO >>\n");
    printk("CSD structure:  %08d\n", psdcsd->DEVCSD_ucStructure);
    printk("Tacc(Ns)     :  %08d\n", psdcsd->DEVCSD_uiTaccNs);
    printk("TACC(CLK)    :  %08d\n", psdcsd->DEVCSD_usTaccClks);
    printk("Tran Speed   :  %08d\n", psdcsd->DEVCSD_uiTranSpeed);
    printk("R2W Factor   :  %08d\n", psdcsd->DEVCSD_ucR2W_Factor);
    printk("Read Blk Len :  %08d\n", 1 << psdcsd->DEVCSD_ucReadBlkLenBits);
    printk("Write Blk Len:  %08d\n", 1 << psdcsd->DEVCSD_ucWriteBlkLenBits);

    printk("Erase Enable :  %08d\n", psdcsd->DEVCSD_bEraseEnable);
    printk("Erase Blk Len:  %08d\n", psdcsd->DEVCSD_ucEraseBlkLen);
    printk("Sector Size  :  %08d\n", psdcsd->DEVCSD_ucSectorSize);

    printk("Read MisAlign:  %08d\n", psdcsd->DEVCSD_bReadMissAlign);
    printk("Writ MisAlign:  %08d\n", psdcsd->DEVCSD_bWriteMissAlign);
    printk("Read Partial :  %08d\n", psdcsd->DEVCSD_bReadBlkPartial);
    printk("Write Partial:  %08d\n", psdcsd->DEVCSD_bWriteBlkPartial);

    printk("Support Cmd  :  %08x\n", psdcsd->DEVCSD_usCmdclass);
    printk("Block Number :  %08d\n", psdcsd->DEVCSD_uiCapacity);
}

#endif                                                                  /*  __SYLIXOS_DEBUG             */
/*********************************************************************************************************
** 函数名称: __sdCoreDecodeCSD
** 功能描述: 解码CSD
** 输    入: pRawCSD         原始CSD数据
**           ucType          卡的类型
** 输    出: psdcsdDec       解码后的CSD
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __sdCoreDecodeCSD (LW_SDDEV_CSD  *psdcsdDec, UINT32 *pRawCSD, UINT8 ucType)
{
    UINT8   ucStruct;
    UINT32  uiExp;
    UINT32  uiMnt;

    if (!psdcsdDec || !pRawCSD) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    lib_bzero(psdcsdDec, sizeof(LW_SDDEV_CSD));

    /*
     * SD规范中，CSD有两个版本. v1.0用于针对一般SD卡.之后出现了v2.0,是为了支持SDHC和SDXC卡.
     * 在SDHC和SDXC中,很多域都是固定的.
     */
    ucStruct = __getBits(pRawCSD, 126, 2);
    psdcsdDec->DEVCSD_ucStructure = ucStruct;

    if (ucType == SDDEV_TYPE_MMC) {
        goto __decsd_mmc_sd;
    }

    switch (ucStruct) {
    case CSD_STRUCT_VER_1_0:
__decsd_mmc_sd:                                                         /*  mmc与sd1.0的csd结构基本相同 */
                                                                        /*  这里只解析了其中关键的数据域*/
        uiMnt = __getBits(pRawCSD, 115, 4);
        uiExp = __getBits(pRawCSD, 112, 3);
        /*
         * 这里除以10,是因为在查找表里面乘了10
         */
        psdcsdDec->DEVCSD_uiTaccNs   = (CSD_TACC_EXP[uiExp] * CSD_TACC_MNT[uiMnt] + 9) / 10;
        psdcsdDec->DEVCSD_usTaccClks = __getBits(pRawCSD, 104, 8) * 100;

        uiMnt = __getBits(pRawCSD, 99, 4);
        uiExp = __getBits(pRawCSD, 96, 3);
        psdcsdDec->DEVCSD_uiTranSpeed = CSD_TRSP_EXP[uiExp] * CSD_TRSP_MNT[uiMnt] ;
        psdcsdDec->DEVCSD_usCmdclass  = __getBits(pRawCSD, 84, 12);

        uiExp = __getBits(pRawCSD, 47, 3);
        uiMnt = __getBits(pRawCSD, 62, 12);
        psdcsdDec->DEVCSD_uiCapacity = (1 + uiMnt) << (uiExp + 2);

        psdcsdDec->DEVCSD_ucR2W_Factor      = __getBits(pRawCSD, 36, 3);

        psdcsdDec->DEVCSD_ucReadBlkLenBits  = __getBits(pRawCSD, 80, 4);
        psdcsdDec->DEVCSD_ucWriteBlkLenBits = __getBits(pRawCSD, 22, 4);

        psdcsdDec->DEVCSD_bReadMissAlign    = __getBits(pRawCSD, 77, 1);
        psdcsdDec->DEVCSD_bWriteMissAlign   = __getBits(pRawCSD, 78, 1);
        psdcsdDec->DEVCSD_bReadBlkPartial   = __getBits(pRawCSD, 79, 1);
        psdcsdDec->DEVCSD_bWriteBlkPartial  = __getBits(pRawCSD, 21, 1);

        /*
         * 擦块的大小在规范中定义：
         *   如果CSD中指明了  使能擦除(ERASE_EN = 1)那么,擦块的大小为1.在这种情况下,
         * 擦除的就仅仅是从擦除起始地址到结束地址的数据...
         *   如果CSD中指明了  禁止擦除(ERASE_EN = 0)那么 ,擦除块的大小由CSD中的SIZE_SECTOR
         * 域来指明.而且还依赖于WRT_BLK_LEN.这种情况下,指定擦除范围所在的块将全部被擦除
         */
        uiExp = __getBits(pRawCSD, 46, 1);
        if (uiExp) {
            psdcsdDec->DEVCSD_ucEraseBlkLen = 1;
        } else if (psdcsdDec->DEVCSD_ucWriteBlkLenBits >= 9) {
            psdcsdDec->DEVCSD_ucEraseBlkLen =  (__getBits(pRawCSD, 47, 3) + 1) <<
                                               (psdcsdDec->DEVCSD_ucWriteBlkLenBits - 9);
        }

        break;

    case CSD_STRUCT_VER_2_0:
        uiMnt = __getBits(pRawCSD, 99, 4);
        uiExp = __getBits(pRawCSD, 96, 3);
        psdcsdDec->DEVCSD_uiTranSpeed = CSD_TRSP_EXP[uiExp] *  CSD_TRSP_MNT[uiMnt];
        psdcsdDec->DEVCSD_usCmdclass  = __getBits(pRawCSD, 84, 12);

        uiMnt = __getBits(pRawCSD, 48, 22);
        psdcsdDec->DEVCSD_uiCapacity = (1 + uiMnt) << 10;

        psdcsdDec->DEVCSD_ucReadBlkLenBits  = 9;
        psdcsdDec->DEVCSD_ucWriteBlkLenBits = 9;
        psdcsdDec->DEVCSD_ucEraseBlkLen     = 1;

        psdcsdDec->DEVCSD_ucR2W_Factor      = 4;
        psdcsdDec->DEVCSD_uiTaccNs          = 1000000;                  /*  固定为1ms                   */
        psdcsdDec->DEVCSD_usTaccClks        = 0;                        /*  固定为0                     */
        break;

    default:
        break;
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown CSD structure.\r\n");
        return  (PX_ERROR);
    }

#ifdef  __SYLIXOS_DEBUG
    __printCSD(psdcsdDec);
#endif                                                                  /*  __SYLIXOS_DEBUG             */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreDevReset
** 功能描述: 复位设备
** 输    入: psdcoredevice 设备结构指针
** 输    出: NONE
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __sdCoreDevReset (PLW_SDCORE_DEVICE psdcoredevice)
{
    LW_SD_COMMAND  sdcmd;
    INT            iError;
    INT            iRetry = 50;                                        /*  重试 50 次复位命令           */

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_GO_IDLE_STATE;
    sdcmd.SDCMD_uiArg    = 0;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_NONE | SD_CMD_BC;

    do {
        SD_DELAYMS(1);
        iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
        if (iError != ERROR_NONE) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "send reset cmd error.\r\n");
            return  (PX_ERROR);
        }

        if (COREDEV_IS_SD(psdcoredevice)) {                             /*  SD模式下不检查响应          */
            return  (ERROR_NONE);
        }

        if ((sdcmd.SDCMD_uiResp[0] == 0x01)) {
            return  (ERROR_NONE);
        }
    } while (iRetry--);

    if (iRetry <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "reset timeout.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);                                               /*  PREVENT WARNING             */
}
/*********************************************************************************************************
** 函数名称: __sdCoreDevSendIfCond
** 功能描述: 检查卡操作接口条件(CMD8).在复位之后,主机并不知道卡能支持的电压是多少,或在哪个范围.
**           于是CMD8（SEND_IF_COND）就用来做这个事.
** 输    入: psdcoredevice 设备结构指针
** 输    出: NONE
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __sdCoreDevSendIfCond (PLW_SDCORE_DEVICE psdcoredevice)
{
    LW_SD_COMMAND   sdcmd;
    INT             iError;
    UINT8           ucChkPattern = 0xaa;                                /* 标准建议在Cmd8中使用0xaa校验 */
    UINT32          uiOCR;

    /*
     * 获得适配器的电源支持情况
     */
    iError = API_SdCoreDevCtl(psdcoredevice,
                              SDBUS_CTRL_GETOCR,
                              (LONG)&uiOCR);
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "get adapter ocr failed.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_IF_COND;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R7 | SD_RSP_R7 | SD_CMD_BCR;

    /*
     * 在SD_SEND_IF_COND(Cmd8)里面的有效参数格式:
     * ----------------------------------------------------------
     *  bits: |            4          |       8          |
     *  ......| HVS(host vol support) |  check pattern   |......
     * ----------------------------------------------------------
     * 目前版本里面的HVS只定义了一个值1表示适配器支持2.7~3.6V的电压.
     */
    sdcmd.SDCMD_uiArg    = (((uiOCR & SD_OCR_MEM_VDD_MSK) != 0) << 8) | ucChkPattern;

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);

    /*
     * 对于Cmd8,如果卡支持参数中的uiOCR信息,那么会回送这些信息,
     * 否则,不会应答,并且卡自身进入空闲模式.
     * TODO: 告诉上层具体失败的原因.
     */
    if (iError == ERROR_NONE) {

        if (COREDEV_IS_SD(psdcoredevice)) {
            ucChkPattern = sdcmd.SDCMD_uiResp[0] & 0xff;
        } else if (COREDEV_IS_SPI(psdcoredevice)) {
            ucChkPattern = sdcmd.SDCMD_uiResp[1] & 0xff;
        } else {
            return  (PX_ERROR);
        }

        if (ucChkPattern != 0xaa) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "the check pattern is not correct, it maybe I/0 error.\r\n");
            return  (PX_ERROR);
        } else {
            return  (ERROR_NONE);
        }
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "the device can't supply the voltage.\r\n");
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __sdCoreDevSendAppOpCond
** 功能描述: 发送ACMD41.
**           当该命令的参数OCR为0,称作"查询ACMD41",效果同CMD8.
**           当该命令的参数OCR不为0,称作"第一个ACMD41",用来启动卡初始化的同时,决定最终的操作电压.
**           该函数为了一次性测出卡的类型(MMC),所以增加了MMC的识别.
** 输    入: psdcoredevice   设备结构指针
**           uiOCR           代表适配器的OCR
** 输    出: psddevocrOut    设备应答的OCR信息
**           pucType         设备的类型
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __sdCoreDevSendAppOpCond (PLW_SDCORE_DEVICE  psdcoredevice,
                              UINT32             uiOCR,
                              LW_SDDEV_OCR      *psddevocrOut,
                              UINT8             *pucType)
{
    LW_SD_COMMAND   sdcmd;
    INT             iError;
    INT             i;
    BOOL            bMmc = LW_FALSE;

    if (!psddevocrOut) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    /*
     * 首先针对SD卡作初始化
     */
    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = APP_SD_SEND_OP_COND;

    /*
     * 存储卡设备的电压范围为2.7 ~ 3.6 v,如果主控提供的电压有不在这个范围内的,则该函数会失败.
     */
    sdcmd.SDCMD_uiArg    = (uiOCR & SD_OCR_HCS) ?
                           (uiOCR & SD_OCR_MEM_VDD_MSK) | SD_OCR_HCS :
                           (uiOCR & SD_OCR_MEM_VDD_MSK) ;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1| SD_RSP_R3 | SD_CMD_BCR;

    for (i = 0; i < SD_OPCOND_DELAY_CONTS; i++) {
        iError = API_SdCoreDevAppCmd(psdcoredevice,
                                     &sdcmd,
                                     LW_FALSE,
                                     SD_CMD_GEN_RETRY);

        if (iError != ERROR_NONE) {
            goto __mmc_ident;
        }

        /*
         * 对于查询ACMD41,只需要一次
         */
        if (uiOCR == 0) {
            break;
        }

        if (COREDEV_IS_SPI(psdcoredevice)) {
            if (!(sdcmd.SDCMD_uiResp[0] & R1_SPI_IDLE)) {               /*  spi 模式下,r1 的最低位清零  */
                break;
            }
        } else {
            if (sdcmd.SDCMD_uiResp[0] & SD_OCR_BUSY) {                  /*  busy置位,表示完成           */
                                                                        /*  进入ready 状态              */
                break;
            }
        }

        SD_DELAYMS(1);
    }

    if (i >= SD_OPCOND_DELAY_CONTS) {                                   /*  sd卡识别失败                */
        goto __mmc_ident;
    } else {
        goto __ident_done;
    }

__mmc_ident:                                                            /*  mmc 识别                    */

    /*
     * 加入MMC的识别
     */
    sdcmd.SDCMD_uiOpcode = SD_SEND_OP_COND;
    sdcmd.SDCMD_uiArg    = COREDEV_IS_SPI(psdcoredevice) ? 0 : uiOCR;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1| SD_RSP_R3 | SD_CMD_BCR;
    for (i = 0; i < SD_OPCOND_DELAY_CONTS; i++) {
        iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
        if (iError != ERROR_NONE) {                                     /*  错误退出                    */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "can't send cmd1.\r\n");
            return  (iError);
        }

        if (uiOCR == 0) {
            break;
        }

        if (COREDEV_IS_SPI(psdcoredevice)) {
            if (!(sdcmd.SDCMD_uiResp[0] & R1_SPI_IDLE)) {               /*  spi 模式下,r1 的最低位清零  */
                break;
            }
        } else {
            if (sdcmd.SDCMD_uiResp[0] & SD_OCR_BUSY) {                  /*  busy置位,表示完成           */
                                                                        /*  进入ready 状态              */
                break;
            }
        }
    }

    if (i >= SD_OPCOND_DELAY_CONTS) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "timeout(may device is not sd or mmc card).\r\n");
        return  (PX_ERROR);
    } else {
        bMmc = LW_TRUE;
    }

__ident_done:                                                           /*  识别完成                    */

    if (COREDEV_IS_SD(psdcoredevice)) {
        *psddevocrOut = sdcmd.SDCMD_uiResp[0];
    } else {
        /*
         * SPI 模式下使用专有命令读取设备OCR寄存器
         */
        lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
        
        sdcmd.SDCMD_uiOpcode = SD_SPI_READ_OCR;
        sdcmd.SDCMD_uiArg    = 0;                                       /*  TODO: 默认支持HIGH CAP      */
        sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R3;

        iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 1);
        if (iError != ERROR_NONE) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "spi read ocr error.\r\n");
            return  (iError);
        }

        *psddevocrOut = sdcmd.SDCMD_uiResp[1];
    }

    /*
     * 判断设备类型
     * TODO:SDXC ?
     */
    if (bMmc) {
        *pucType = SDDEV_TYPE_MMC;
    } else if (*psddevocrOut & SD_OCR_HCS) {
        *pucType = SDDEV_TYPE_SDHC;
    } else {
        *pucType = SDDEV_TYPE_SDSC;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreDevSendRelativeAddr
** 功能描述: 让卡发送自己的本地地址.
**           !!注意,该函数只能用于SD总线上的设备.
** 输    入: psdcoredevice   设备结构指针
** 输    出: puiRCA          卡应答的RCA地址
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __sdCoreDevSendRelativeAddr (PLW_SDCORE_DEVICE psdcoredevice, UINT32 *puiRCA)
{
    INT              iError;
    LW_SD_COMMAND    sdcmd;

    if (!puiRCA) {
        return  (PX_ERROR);
    }

    if (!COREDEV_IS_SD(psdcoredevice)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "function just for sd bus.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_RELATIVE_ADDR;
    sdcmd.SDCMD_uiArg    = 0;
    sdcmd.SDCMD_uiFlag   = SD_RSP_R6 | SD_CMD_BCR;

    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);

    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    *puiRCA = sdcmd.SDCMD_uiResp[0] >> 16;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreDevMmcSetRelativeAddr
** 功能描述: 设置MMC卡的RCA.
**           MMC的本地地址是由用户设置的(SD是由卡获得的).
** 输    入: psdcoredevice   设备结构指针
**           uiRCA           MMC的RCA
** 输    出: NONE
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __sdCoreDevMmcSetRelativeAddr (PLW_SDCORE_DEVICE psdcoredevice, UINT32 uiRCA)
{
    INT              iError;
    LW_SD_COMMAND    sdcmd;

    if (!COREDEV_IS_SD(psdcoredevice)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "function just for sd bus.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_RELATIVE_ADDR;
    sdcmd.SDCMD_uiArg    = uiRCA << 16;
    sdcmd.SDCMD_uiFlag   = SD_RSP_R1 | SD_CMD_AC;

    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);

    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreDevSendAllCID
** 功能描述: 获得卡的CID
** 输    入: psdcoredevice   设备结构指针
** 输    出: psdcid          卡应答的CID(已经解码)
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __sdCoreDevSendAllCID (PLW_SDCORE_DEVICE psdcoredevice, LW_SDDEV_CID *psdcid)
{
    INT              iError;
    LW_SD_COMMAND    sdcmd;
    UINT8            pucCidBuf[16];
    UINT8            ucType;

    if (!psdcid) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "param error.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = COREDEV_IS_SD(psdcoredevice) ? SD_ALL_SEND_CID : SD_SEND_CID;
    sdcmd.SDCMD_uiArg    = 0;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R2 | SD_CMD_BCR;
    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    /*
     * SD模式下直接用命令获取CID
     */
    API_SdCoreDevTypeView(psdcoredevice, &ucType);
    if (COREDEV_IS_SD(psdcoredevice)) {
        __sdCoreDecodeCID(psdcid, sdcmd.SDCMD_uiResp, ucType);
        return  (ERROR_NONE);
    }

    /*
     * SPI模式使用特殊的读CID寄存器方式
     */
    API_SdCoreSpiRegisterRead(psdcoredevice, pucCidBuf, 16);
    API_SdCoreSpiCxdFormat(sdcmd.SDCMD_uiResp, pucCidBuf);
    __sdCoreDecodeCID(psdcid, sdcmd.SDCMD_uiResp, ucType);

    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreDevSendCSD
** 功能描述: 获得卡的CSD
** 输    入: psdcoredevice   设备结构指针
** 输    出: psdcsd          卡应答的CSD(已经解码)
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __sdCoreDevSendAllCSD (PLW_SDCORE_DEVICE psdcoredevice, LW_SDDEV_CSD *psdcsd)
{
    INT              iError;
    LW_SD_COMMAND    sdcmd;
    UINT32           uiRca;
    UINT8            pucCsdBuf[16];
    UINT8            ucType;

    if (!psdcsd) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "param error.\r\n");
        return  (PX_ERROR);
    }

    iError = API_SdCoreDevRcaView(psdcoredevice, &uiRca);

    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device without rca.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_CSD;
    sdcmd.SDCMD_uiArg    = COREDEV_IS_SPI(psdcoredevice) ? 0 : uiRca << 16;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R2 | SD_CMD_BCR;
    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    /*
     * SD模式直接从命令的应答中获取
     */
    API_SdCoreDevTypeView(psdcoredevice, &ucType);
    if (COREDEV_IS_SD(psdcoredevice)) {
        __sdCoreDecodeCSD(psdcsd, sdcmd.SDCMD_uiResp, ucType);
        return  (ERROR_NONE);
    }

    /*
     * SPI 模式
     */
    API_SdCoreSpiRegisterRead(psdcoredevice, pucCsdBuf, 16);
    API_SdCoreSpiCxdFormat(sdcmd.SDCMD_uiResp, pucCsdBuf);
    __sdCoreDecodeCSD(psdcsd, sdcmd.SDCMD_uiResp, ucType);

    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdCoreSelectDev
** 功能描述: 设备选择
** 输    入: psdcoredevice   设备结构指针
**           bSel            选择\取消
** 输    出: NONE
** 返    回: ERROR    CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdCoreSelectDev (PLW_SDCORE_DEVICE psdcoredevice, BOOL bSel)
{
    INT             iError;
    LW_SD_COMMAND   sdcmd;
    UINT32          uiRca;

    if (COREDEV_IS_SPI(psdcoredevice)) {                                /*  SPI设备使用物理片选         */
        return  (ERROR_NONE);
    }

    iError = API_SdCoreDevRcaView(psdcoredevice, &uiRca);
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device without rca.\r\n");
        return  (PX_ERROR);
    }

    lib_bzero(&sdcmd, sizeof(LW_SD_COMMAND));
    
    sdcmd.SDCMD_uiOpcode   = SD_SELECT_CARD;

    if (bSel) {
        sdcmd.SDCMD_uiArg  = uiRca << 16;
        sdcmd.SDCMD_uiFlag = SD_RSP_R1B | SD_CMD_AC;
    } else {
        sdcmd.SDCMD_uiArg  = 0;
        sdcmd.SDCMD_uiFlag = SD_RSP_NONE | SD_CMD_BC;
    }

    iError = API_SdCoreDevCmd(psdcoredevice,
                              &sdcmd,
                              SD_CMD_GEN_RETRY);
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");
        return  (PX_ERROR);
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: __sdCoreDevSelect
 ** 功能描述: 设备选择
 ** 输    入: psdcoredevice     设备结构指针
 ** 输    出: NONE
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT __sdCoreDevSelect (PLW_SDCORE_DEVICE psdcoredevice)
{
    INT iError;
    iError = __sdCoreSelectDev(psdcoredevice, LW_TRUE);

    return  (iError);
}
/*********************************************************************************************************
 ** 函数名称: __sdCoreDevDeSelect
 ** 功能描述: 设备取消
 ** 输    入: psdcoredevice     设备结构指针
 ** 输    出: NONE
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT __sdCoreDevDeSelect (PLW_SDCORE_DEVICE psdcoredevice)
{
    INT iError;

    iError = __sdCoreSelectDev(psdcoredevice, LW_FALSE);

    return  (iError);
}
/*********************************************************************************************************
 ** 函数名称: __sdCoreDevSetBusWidth
 ** 功能描述: 设置总线通信位宽
 ** 输    入: psdcoredevice     设备结构指针
 ** 输    出: NONE
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT __sdCoreDevSetBusWidth (PLW_SDCORE_DEVICE psdcoredevice, INT iBusWidth)
{
    INT             iError = ERROR_NONE;
    LW_SD_COMMAND   sdcmd;

    iError = __sdCoreDevSelect(psdcoredevice);
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "select device failed.\r\n");
        return  (PX_ERROR);
    }

    if (!psdcoredevice) {
        __sdCoreDevDeSelect(psdcoredevice);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((iBusWidth != SDBUS_WIDTH_1) && (iBusWidth != SDBUS_WIDTH_4)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "invalid bus width in current sd version.\r\n");
        __sdCoreDevDeSelect(psdcoredevice);
        return  (PX_ERROR);
    }

    /*
     * 只在SD模式下才设置, SPI模式下忽略
     */
    if (COREDEV_IS_SD(psdcoredevice)) {
        lib_bzero(&sdcmd, sizeof(sdcmd));
        
        sdcmd.SDCMD_uiOpcode = APP_SET_BUS_WIDTH;
        sdcmd.SDCMD_uiFlag   = SD_RSP_R1 | SD_CMD_AC;
        sdcmd.SDCMD_uiArg    = iBusWidth;

        iError = API_SdCoreDevAppCmd(psdcoredevice,
                                     &sdcmd,
                                     LW_FALSE,
                                     0);
    }
    __sdCoreDevDeSelect(psdcoredevice);

    return  (iError);
}
/*********************************************************************************************************
 ** 函数名称: __sdCoreDevSetBlkLen
 ** 功能描述: 设置记忆卡的块长度.该函数影响之后的数据读、写、锁定中的块大小参数.
 **           但是在SDHC和SDXC中,只影响锁定命令中的参数(因为读写固定为512byte块大小).
 ** 输    入: psdcoredevice   设备结构指针
 **           iBlkLen         块大小
 ** 输    出: NONE
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT __sdCoreDevSetBlkLen (PLW_SDCORE_DEVICE psdcoredevice, INT iBlkLen)
{
    INT             iError;
    LW_SD_COMMAND   sdcmd;

    sdcmd.SDCMD_uiOpcode = SD_SET_BLOCKLEN;
    sdcmd.SDCMD_uiArg    = iBlkLen;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_AC;

    iError = __sdCoreDevSelect(psdcoredevice);
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "select device failed.\r\n");
        return  (PX_ERROR);
    }

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "send cmd16 failed.\r\n");
        __sdCoreDevDeSelect(psdcoredevice);
        return  (PX_ERROR);
    }

    if (COREDEV_IS_SPI(psdcoredevice)) {
        if ((sdcmd.SDCMD_uiResp[0] & 0xff) != 0) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "spi response error.\r\n");
            return  (PX_ERROR);
        }
    }

    return  (__sdCoreDevDeSelect(psdcoredevice));
}
/*********************************************************************************************************
 ** 函数名称: __sdCoreDevGetStatus
 ** 功能描述: 得到卡的状态字
 ** 输    入: psdcoredevice   设备结构指针
 ** 输    出: puiStatus       状态
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT __sdCoreDevGetStatus (PLW_SDCORE_DEVICE psdcoredevice, UINT32 *puiStatus)
{
    LW_SD_COMMAND  sdcmd;
    UINT32         uiRca;
    INT            iError;

    API_SdCoreDevRcaView(psdcoredevice, &uiRca);

    lib_bzero(&sdcmd, sizeof(sdcmd));
    
    sdcmd.SDCMD_uiOpcode = SD_SEND_STATUS;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R2 | SD_RSP_R1 | SD_CMD_AC;
    sdcmd.SDCMD_uiArg    = uiRca << 16;
    sdcmd.SDCMD_uiRetry  = 0;

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 1);
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "send cmd13 failed.\r\n");
        return  (PX_ERROR);
    }

    *puiStatus = sdcmd.SDCMD_uiResp[0];

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: __sdCoreDevSetPreBlkLen
 ** 功能描述: 设置预先擦除块计数.在 "<多块写>" 操作中,该功能用于让卡预先擦除指定的块大小,这样可以提高速度.
 ** 输    入: psdcoredevice   设备结构指针
 **           iPreBlkLen   块大小
 ** 输    出: NONE
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT __sdCoreDevSetPreBlkLen (PLW_SDCORE_DEVICE psdcoredevice, INT iPreBlkLen)
{
    LW_SD_COMMAND  sdcmd;
    INT            iError;

    lib_bzero(&sdcmd, sizeof(sdcmd));
    
    sdcmd.SDCMD_uiOpcode = SD_SET_BLOCK_COUNT;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1 | SD_RSP_R1 | SD_CMD_AC;
    sdcmd.SDCMD_uiArg    = iPreBlkLen;
    sdcmd.SDCMD_uiRetry  = 0;

    iError = API_SdCoreDevAppCmd(psdcoredevice,
                                 &sdcmd,
                                 LW_FALSE,
                                 1);
    if (iError != ERROR_NONE) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "send acmd23 failed.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: __sdCoreDevIsBlockAddr
 ** 功能描述: 判断设备是否是块寻址
 ** 输    入: psdcoredevice     设备结构指针
 ** 输    出: pbResult          结果
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT __sdCoreDevIsBlockAddr (PLW_SDCORE_DEVICE psdcoredevice, BOOL *pbResult)
{
    UINT8   ucType;

    API_SdCoreDevTypeView(psdcoredevice, &ucType);

    switch (ucType) {
    case SDDEV_TYPE_MMC  :
    case SDDEV_TYPE_SDSC :
        *pbResult = LW_FALSE;
        break;

    case SDDEV_TYPE_SDHC :
    case SDDEV_TYPE_SDXC :
        *pbResult = LW_TRUE;
        break;

    case SDDEV_TYPE_SDIO :
    case SDDEV_TYPE_COMM :
    default:
        _DebugHandle(__ERRORMESSAGE_LEVEL, "don't support.\r\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: __sdCoreDevSpiClkDely
 ** 功能描述: 在SPI模式下的时钟延时
 ** 输    入: psdcoredevice     设备结构指针
 **           iClkConts         延时clock个数
 ** 输    出: NONE
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT __sdCoreDevSpiClkDely (PLW_SDCORE_DEVICE psdcoredevice, INT iClkConts)
{
    INT iError;

    iClkConts = (iClkConts + 7) / 8;

    iError = API_SdCoreDevCtl(psdcoredevice, SDBUS_CTRL_DELAYCLK, iClkConts);
    return  (iError);
}
/*********************************************************************************************************
 ** 函数名称: __sdCoreDevSpiCrcEn
 ** 功能描述: 在SPI模式下使能设备的CRC校验
 ** 输    入: psdcoredevice     设备结构指针
 **           bEnable           是否使能.(0:禁止crc  1:使能crc)
 ** 输    出: NONE
 ** 返    回: ERROR    CODE
 ** 全局变量:
 ** 调用模块:
 ********************************************************************************************************/
INT __sdCoreDevSpiCrcEn (PLW_SDCORE_DEVICE psdcoredevice, BOOL bEnable)
{
    LW_SD_COMMAND  sdcmd;
    INT            iError;

    lib_bzero(&sdcmd, sizeof(sdcmd));
    
    sdcmd.SDCMD_uiOpcode = SD_SPI_CRC_ON_OFF;
    sdcmd.SDCMD_uiFlag   = SD_RSP_SPI_R1;
    sdcmd.SDCMD_uiArg    = bEnable ? 1 : 0;
    sdcmd.SDCMD_uiRetry  = 0;

    iError = API_SdCoreDevCmd(psdcoredevice, &sdcmd, 0);
    if (iError == ERROR_NONE) {
        if ((sdcmd.SDCMD_uiResp[0] & 0xff) != 0x00) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "set crc failed.\r\n");
            return  (PX_ERROR);
        } else {
            return  (ERROR_NONE);
        }
    }

    _DebugHandle(__ERRORMESSAGE_LEVEL, "send cmd error.\r\n");

    return  (PX_ERROR);
}
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
