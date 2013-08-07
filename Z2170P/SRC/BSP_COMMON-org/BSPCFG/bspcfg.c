// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File:  bsp_cfg.c
//
#include "bsp.h"
#include "bsp_cfg.h"
#include "bsp_def.h"
#include "oal_i2c.h"
#include "ceddkex.h"
#include "oalex.h"
#include "oal_prcm.h"
#include "S35390_rtc.h"

// External variable containing the measured value of the SYS_CLK frequency
DWORD g_dwMeasuredSysClkFreq = 13000000;

//------------------------------------------------------------------------------
//  DEBUG UART
//------------------------------------------------------------------------------
//  This constants are used to initialize serial debugger output UART.
//  Serial debugger uses 115200-8-N-1
static const DEBUG_UART_CFG debugUartCfg={
    OMAP_DEVICE_UART3,
    BSP_UART_LCR,
    BSP_UART_DSIUDLL,
    BSP_UART_DSIUDLH
} ;

const DEBUG_UART_CFG* BSPGetDebugUARTConfig()
{
    return &debugUartCfg;
}



//------------------------------------------------------------------------------
//  I2C
//------------------------------------------------------------------------------
DWORD const I2CDefaultI2CTimeout = BSP_I2C_TIMEOUT_INIT;
//-----------------------------------------------------------------------------
I2CScaleTable_t _rgScaleTable[] = {
    { I2C_SS_PSC_VAL, I2C_SS_SCLL_VAL,  I2C_SS_SCLH_VAL }, // 100 KHz mode
    { I2C_FS_PSC_VAL, I2C_FS_SCLL_VAL,  I2C_FS_SCLH_VAL }, // 400 KHz mode (FS)
    { I2C_HS_PSC_VAL, I2C_1P6M_HSSCLL,  I2C_1P6M_HSSCLH }, // 1.6 MHz mode (HS)
    { I2C_HS_PSC_VAL, I2C_2P4M_HSSCLL,  I2C_2P4M_HSSCLH }, // 2.4 MHz mode (HS)
    { I2C_HS_PSC_VAL, I2C_3P2M_HSSCLL,  I2C_3P2M_HSSCLH }, // 3.2 MHz mode (HS)
};

I2CDevice_t _rgI2CDevice[] = 
{
    {
        OMAP_DEVICE_I2C1, BSP_I2C1_OA_INIT, BSP_I2C1_BAUDRATE_INIT,  BSP_I2C1_MAXRETRY_INIT, 
        BSP_I2C1_RX_THRESHOLD_INIT, BSP_I2C1_TX_THRESHOLD_INIT,
    }, {
        OMAP_DEVICE_I2C2, BSP_I2C2_OA_INIT, BSP_I2C2_BAUDRATE_INIT,  BSP_I2C2_MAXRETRY_INIT,
        BSP_I2C2_RX_THRESHOLD_INIT, BSP_I2C2_TX_THRESHOLD_INIT,
    }, {
        OMAP_DEVICE_I2C3, BSP_I2C3_OA_INIT, BSP_I2C3_BAUDRATE_INIT,  BSP_I2C3_MAXRETRY_INIT,
        BSP_I2C3_RX_THRESHOLD_INIT, BSP_I2C3_TX_THRESHOLD_INIT,
    }, {
        OMAP_DEVICE_NONE
    }
};



//------------------------------------------------------------------------------
//  NAND Flash
//------------------------------------------------------------------------------

NAND_INFO SupportedNands[] = {
    {
        0x2C,   //manufacturerId
        0xBA,   //deviceId
        2048,   //blocks
        64,     //sectorsPerBlock
        2048,   //sectorSize
        2       //wordData
    }
,
   {
        0x2C,   //manufacturerId
        0xBC,   //deviceId
        4096,   //blocks
        64,     //sectorsPerBlock
        2048,   //sectorSize
        2       //wordData
    }
,
   {
        0xAD,   //manufacturerId
        0xBC,   //deviceId
        4096,   //blocks
        64,     //sectorsPerBlock
        2048,   //sectorSize
        2       //wordData
    }

};

DWORD BSPGetNandIrqWait()
{
    return BSP_GPMC_IRQ_WAIT_EDGE;
}
DWORD BSPGetNandCS()
{
    return BSP_GPMC_NAND_CS;
}
const NAND_INFO* BSPGetNandInfo(DWORD manufacturer,DWORD device)
{
    int i;    
    for (i=0;i< dimof(SupportedNands);i++)
    {
        if ((SupportedNands[i].manufacturerId == manufacturer) && (SupportedNands[i].deviceId == device))
        {
            return &SupportedNands[i];
        }
    }
    RETAILMSG(1,(TEXT("NAND manufacturer %x device %x : no matching device found\r\n"),manufacturer,device));    
    return NULL;
}


//------------------------------------------------------------------------
// GPIO
//------------------------------------------------------------------------

#define NB_GPIO_GRP 4

static HANDLE gpioHandleTable[NB_GPIO_GRP];
static UINT   gpioRangesTable[NB_GPIO_GRP];
static DEVICE_IFC_GPIO* gpioTables[NB_GPIO_GRP];
static WCHAR* gpioNames[NB_GPIO_GRP];
GpioDevice_t BSPGpioTables = {
    0,
    0,
    gpioRangesTable,
    gpioHandleTable,
    gpioTables,
    gpioNames,
};

BOOL BSPInsertGpioDevice(UINT range,DEVICE_IFC_GPIO* fnTbl,WCHAR* name)
{
    int index = BSPGpioTables.nbGpioGrp;
    if (index >= NB_GPIO_GRP) return FALSE;
    BSPGpioTables.rgRanges[index] = range;
    BSPGpioTables.rgGpioTbls[index] = fnTbl;
    BSPGpioTables.name[index] = name;
    BSPGpioTables.nbGpioGrp++;
    return TRUE;
}

GpioDevice_t* BSPGetGpioDevicesTable()
{
    return &BSPGpioTables;
}
DWORD BSPGetGpioIrq(DWORD id)
{
    return id + IRQ_GPIO_0;
}
//------------------------------------------------------------------------


//------------------------------------------------------------------------------
//  Triton
//------------------------------------------------------------------------------
DWORD BSPGetTritonBusID()
{
    return TPS659XX_I2C_BUS_ID;
}

UINT16 BSPGetTritonSlaveAddress()
{
    return TPS659XX_I2C_SLAVE_ADDRESS;
}




//------------------------------------------------------------------------------
//  SDHC
//------------------------------------------------------------------------------
DWORD BSPGetSDHCCardDetect(DWORD slot)
{
    switch (slot)
    {
    case 1: return MMC1_CARDDET_GPIO;
    default: return (DWORD) -1;
    }
}

//------------------------------------------------------------------------------
//  IRQ
//------------------------------------------------------------------------------


//  Following arrays contain interrupt routing, level and priority
//  initialization values for ILR interrupt controller registers.
const UINT32 g_BSP_icL1Level[] = {
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 0
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 2
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 4
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 6
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 8
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 10
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 12
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 14
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 16
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 18
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 20
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 22
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 24
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 26
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 28
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 30

    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 32/0
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 34/2
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI1,    // 36/4  gptimer1 prio=0
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 38/6
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 40/8
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 42/10
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 44/12
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 46/14
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 48/16
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 50/18
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 52/20
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 54/22
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 56/24
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 58/26
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 60/28
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 62/30

    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 64/0
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 66/2
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 68/4
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 70/6
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 72/8
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 74/10
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 76/12
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 78/14
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 80/16
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 82/18
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 84/20
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 86/22
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 88/24
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 90/26
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16,   // 92/28
    IC_ILR_IRQ|IC_ILR_PRI16, IC_ILR_IRQ|IC_ILR_PRI16    // 94/30
 };

//------------------------------------------------------------------------------
//  System Timer
//------------------------------------------------------------------------------
OMAP_DEVICE BSPGetSysTimerDevice()
{
    return OMAP_DEVICE_GPTIMER1;
}

OMAP_CLOCK BSPGetSysTimer32KClock()
{
    return k32K_FCLK;
}

//------------------------------------------------------------------------------
//  Profiler and Performance counter
//------------------------------------------------------------------------------
OMAP_DEVICE BSPGetGPTPerfDevice()
{
    return OMAP_DEVICE_GPTIMER2;

}

OMAP_CLOCK BSPGetGPTPerfHighFreqClock(UINT32* pFreq)
{
#if 1
    //use SYS CLK as the timer source. better precision but cause CETK to fail because of the drift with the 32K clock
    *pFreq = (UINT32) g_dwMeasuredSysClkFreq;        
    return kSYS_CLK;
#else
   //use 32K CLK as the timer source.
    *pFreq = 32768;
    return k32K_FCLK;
#endif
}
//------------------------------------------------------------------------------
//  RTC interrupt
//------------------------------------------------------------------------------
DWORD BSPGetRTCGpioIrq()
{
    return (DWORD)-1;//BSPGetGpioIrq(RTC_IRQ_GPIO);
}

//------------------------------------------------------------------------------
//  Watchdog. returns the device used as the system watchdog
//------------------------------------------------------------------------------
OMAP_DEVICE BPSGetWatchdogDevice()
{
    return OMAP_DEVICE_WDT2;
}

