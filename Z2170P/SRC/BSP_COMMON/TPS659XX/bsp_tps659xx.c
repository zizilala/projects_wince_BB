// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  File:  bsp_tps659xx.c
//


//------------------------------------------------------------------------------
//
// includes
//

#include <bsp.h>
#include "sdk_gpio.h"

#include "twl.h"
#include "tps659xx.h"
#include "tps659xx_internals.h"

#define DEFAULT_VMMC1_VR    TWL_VMMC1_3P0
#define DEFAULT_VMMC2_VR    TWL_VMMC2_1P85

//------------------------------------------------------------------------------
static 
UINT16 
DecideTwlVMMC(
              UINT16 slot
             )
{
    if(slot == 0)   // SLOT1
    {
        return TWL_VMMC1_3P0;
    }

    if(slot == 1)   // SLOT2
    {
        return TWL_VMMC2_1P85;
    } 

    return DEFAULT_VMMC1_VR;
}


static void ShowT2Reg(void* hTwl, DWORD RegAddr, TCHAR * szRegname)
{
    UINT8 Value = 0;
    TWLReadRegs(hTwl, RegAddr, &Value, sizeof(Value));
    OALLog(L"T2 %s = x%02x\r\n", szRegname, Value);
}

#define SHOW_T2_REG(reg) { \
    ShowT2Reg(hTwl, reg, TEXT(#reg)); \
}

//------------------------------------------------------------------------------
VOID 
InitTwlPower(
    )
{
    void* hTwl;
    UINT16 val;

    // initialize T2 power level and grouping
    hTwl = TWLOpen();

    // unsecure registers
    TWLWriteByteReg(hTwl, TWL_PROTECT_KEY, 0xCE);
    TWLWriteByteReg(hTwl, TWL_PROTECT_KEY, 0xEC);

/*
    SHOW_T2_REG(TWL_CFG_P1_TRANSITION);
    SHOW_T2_REG(TWL_CFG_P2_TRANSITION);
    SHOW_T2_REG(TWL_CFG_P3_TRANSITION);
    SHOW_T2_REG(TWL_CFG_P123_TRANSITION);
    SHOW_T2_REG(TWL_STS_BOOT);
    SHOW_T2_REG(TWL_CFG_BOOT);
    SHOW_T2_REG(TWL_SHUNDAN);
    SHOW_T2_REG(TWL_BOOT_BCI);
    SHOW_T2_REG(TWL_CFG_PWRANA1);
    SHOW_T2_REG(TWL_CFG_PWRANA2);
    SHOW_T2_REG(TWL_BGAP_TRIM);
    SHOW_T2_REG(TWL_BACKUP_MISC_STS);
    SHOW_T2_REG(TWL_BACKUP_MISC_CFG);
    SHOW_T2_REG(TWL_BACKUP_MISC_TST);
    SHOW_T2_REG(TWL_PROTECT_KEY);
    SHOW_T2_REG(TWL_STS_HW_CONDITIONS);
    SHOW_T2_REG(TWL_P1_SW_EVENTS);
    SHOW_T2_REG(TWL_P2_SW_EVENTS);
    SHOW_T2_REG(TWL_P3_SW_EVENTS);
    SHOW_T2_REG(TWL_STS_P123_STATE);
    SHOW_T2_REG(TWL_PB_CFG);
    SHOW_T2_REG(TWL_PB_WORD_MSB);
    SHOW_T2_REG(TWL_PB_WORD_LSB);
    SHOW_T2_REG(TWL_RESERVED_A);
    SHOW_T2_REG(TWL_RESERVED_B);
    SHOW_T2_REG(TWL_RESERVED_C);
    SHOW_T2_REG(TWL_RESERVED_D);
    SHOW_T2_REG(TWL_RESERVED_E);
    SHOW_T2_REG(TWL_SEQ_ADD_W2P);
    SHOW_T2_REG(TWL_SEQ_ADD_P2A);
    SHOW_T2_REG(TWL_SEQ_ADD_A2W);
    SHOW_T2_REG(TWL_SEQ_ADD_A2S);
    SHOW_T2_REG(TWL_SEQ_ADD_S2A12);
    SHOW_T2_REG(TWL_SEQ_ADD_S2A3);
    SHOW_T2_REG(TWL_SEQ_ADD_WARM);
    SHOW_T2_REG(TWL_MEMORY_ADDRESS);
    SHOW_T2_REG(TWL_MEMORY_DATA);

    // pm receiver (un)secure mode
    SHOW_T2_REG(TWL_SC_CONFIG);
    SHOW_T2_REG(TWL_SC_DETECT1);
    SHOW_T2_REG(TWL_SC_DETECT2);
    SHOW_T2_REG(TWL_WATCHDOG_CFG);
    SHOW_T2_REG(TWL_IT_CHECK_CFG);
    SHOW_T2_REG(TWL_VIBRATOR_CFG);
    SHOW_T2_REG(TWL_DCDC_GLOBAL_CFG);
    SHOW_T2_REG(TWL_VDD1_TRIM1);
    SHOW_T2_REG(TWL_VDD1_TRIM2);
    SHOW_T2_REG(TWL_VDD2_TRIM1);
    SHOW_T2_REG(TWL_VDD2_TRIM2);
    SHOW_T2_REG(TWL_VIO_TRIM1);
    SHOW_T2_REG(TWL_VIO_TRIM2);
    SHOW_T2_REG(TWL_MISC_CFG);
    SHOW_T2_REG(TWL_LS_TST_A);
    SHOW_T2_REG(TWL_LS_TST_B);
    SHOW_T2_REG(TWL_LS_TST_C);
    SHOW_T2_REG(TWL_LS_TST_D);
    SHOW_T2_REG(TWL_BB_CFG);
    SHOW_T2_REG(TWL_MISC_TST);
    SHOW_T2_REG(TWL_TRIM1);
    SHOW_T2_REG(TWL_TRIM2);
    SHOW_T2_REG(TWL_DCDC_TIMEOUT);
    SHOW_T2_REG(TWL_VAUX1_DEV_GRP);
    SHOW_T2_REG(TWL_VAUX1_TYPE);
    SHOW_T2_REG(TWL_VAUX1_REMAP);
    SHOW_T2_REG(TWL_VAUX1_DEDICATED);
    SHOW_T2_REG(TWL_VAUX2_DEV_GRP);
    SHOW_T2_REG(TWL_VAUX2_TYPE);
    SHOW_T2_REG(TWL_VAUX2_REMAP);
    SHOW_T2_REG(TWL_VAUX2_DEDICATED);
    SHOW_T2_REG(TWL_VAUX3_DEV_GRP);
    SHOW_T2_REG(TWL_VAUX3_TYPE);
    SHOW_T2_REG(TWL_VAUX3_REMAP);
    SHOW_T2_REG(TWL_VAUX3_DEDICATED);
    SHOW_T2_REG(TWL_VAUX4_DEV_GRP);
    SHOW_T2_REG(TWL_VAUX4_TYPE);
    SHOW_T2_REG(TWL_VAUX4_REMAP);
    SHOW_T2_REG(TWL_VAUX4_DEDICATED);
    SHOW_T2_REG(TWL_VMMC1_DEV_GRP);
    SHOW_T2_REG(TWL_VMMC1_TYPE);
    SHOW_T2_REG(TWL_VMMC1_REMAP);
    SHOW_T2_REG(TWL_VMMC1_DEDICATED);
    SHOW_T2_REG(TWL_VMMC2_DEV_GRP);
    SHOW_T2_REG(TWL_VMMC2_TYPE);
    SHOW_T2_REG(TWL_VMMC2_REMAP);
    SHOW_T2_REG(TWL_VMMC2_DEDICATED);
    SHOW_T2_REG(TWL_VPLL1_DEV_GRP);
    SHOW_T2_REG(TWL_VPLL1_TYPE);
    SHOW_T2_REG(TWL_VPLL1_REMAP);
    SHOW_T2_REG(TWL_VPLL1_DEDICATED);
    SHOW_T2_REG(TWL_VPLL2_DEV_GRP);
    SHOW_T2_REG(TWL_VPLL2_TYPE);
    SHOW_T2_REG(TWL_VPLL2_REMAP);
    SHOW_T2_REG(TWL_VPLL2_DEDICATED);
    SHOW_T2_REG(TWL_VSIM_DEV_GRP);
    SHOW_T2_REG(TWL_VSIM_TYPE);
    SHOW_T2_REG(TWL_VSIM_REMAP);
    SHOW_T2_REG(TWL_VSIM_DEDICATED);
    SHOW_T2_REG(TWL_VDAC_DEV_GRP);
    SHOW_T2_REG(TWL_VDAC_TYPE);
    SHOW_T2_REG(TWL_VDAC_REMAP);
    SHOW_T2_REG(TWL_VDAC_DEDICATED);
    SHOW_T2_REG(TWL_VINTANA1_DEV_GRP);
    SHOW_T2_REG(TWL_VINTANA1_TYPE);
    SHOW_T2_REG(TWL_VINTANA1_REMAP);
    SHOW_T2_REG(TWL_VINTANA1_DEDICATED);
    SHOW_T2_REG(TWL_VINTANA2_DEV_GRP);
    SHOW_T2_REG(TWL_VINTANA2_TYPE);
    SHOW_T2_REG(TWL_VINTANA2_REMAP);
    SHOW_T2_REG(TWL_VINTANA2_DEDICATED);
    SHOW_T2_REG(TWL_VINTDIG_DEV_GRP);
    SHOW_T2_REG(TWL_VINTDIG_TYPE);
    SHOW_T2_REG(TWL_VINTDIG_REMAP);
    SHOW_T2_REG(TWL_VINTDIG_DEDICATED);
    SHOW_T2_REG(TWL_VIO_DEV_GRP);
    SHOW_T2_REG(TWL_VIO_TYPE);
    SHOW_T2_REG(TWL_VIO_REMAP);
    SHOW_T2_REG(TWL_VIO_CFG);
    SHOW_T2_REG(TWL_VIO_MISC_CFG);
    SHOW_T2_REG(TWL_VIO_TEST1);
    SHOW_T2_REG(TWL_VIO_TEST2);
    SHOW_T2_REG(TWL_VIO_OSC);
    SHOW_T2_REG(TWL_VIO_RESERVED);
    SHOW_T2_REG(TWL_VIO_VSEL);
    SHOW_T2_REG(TWL_VDD1_DEV_GRP);
    SHOW_T2_REG(TWL_VDD1_TYPE);
    SHOW_T2_REG(TWL_VDD1_REMAP);
    SHOW_T2_REG(TWL_VDD1_CFG);
    SHOW_T2_REG(TWL_VDD1_MISC_CFG);
    SHOW_T2_REG(TWL_VDD1_TEST1);
    SHOW_T2_REG(TWL_VDD1_TEST2);
    SHOW_T2_REG(TWL_VDD1_OSC);
    SHOW_T2_REG(TWL_VDD1_RESERVED);
    SHOW_T2_REG(TWL_VDD1_VSEL);
    SHOW_T2_REG(TWL_VDD1_VMODE_CFG);
    SHOW_T2_REG(TWL_VDD1_VFLOOR);
    SHOW_T2_REG(TWL_VDD1_VROOF);
    SHOW_T2_REG(TWL_VDD1_STEP);
    SHOW_T2_REG(TWL_VDD2_DEV_GRP);
    SHOW_T2_REG(TWL_VDD2_TYPE);
    SHOW_T2_REG(TWL_VDD2_REMAP);
    SHOW_T2_REG(TWL_VDD2_CFG);
    SHOW_T2_REG(TWL_VDD2_MISC_CFG);
    SHOW_T2_REG(TWL_VDD2_TEST1);
    SHOW_T2_REG(TWL_VDD2_TEST2);
    SHOW_T2_REG(TWL_VDD2_OSC);
    SHOW_T2_REG(TWL_VDD2_RESERVED);
    SHOW_T2_REG(TWL_VDD2_VSEL);
    SHOW_T2_REG(TWL_VDD2_VMODE_CFG);
    SHOW_T2_REG(TWL_VDD2_VFLOOR);
    SHOW_T2_REG(TWL_VDD2_VROOF);
    SHOW_T2_REG(TWL_VDD2_STEP);
    SHOW_T2_REG(TWL_VUSB1V5_DEV_GRP);
    SHOW_T2_REG(TWL_VUSB1V5_TYPE);
    SHOW_T2_REG(TWL_VUSB1V5_REMAP);
    SHOW_T2_REG(TWL_VUSB1V8_DEV_GRP);
    SHOW_T2_REG(TWL_VUSB1V8_TYPE);
    SHOW_T2_REG(TWL_VUSB1V8_REMAP);
    SHOW_T2_REG(TWL_VUSB3V1_DEV_GRP);
    SHOW_T2_REG(TWL_VUSB3V1_TYPE);
    SHOW_T2_REG(TWL_VUSB3V1_REMAP);
    SHOW_T2_REG(TWL_VUSBCP_DEV_GRP);
    SHOW_T2_REG(TWL_VUSBCP_TYPE);
    SHOW_T2_REG(TWL_VUSBCP_REMAP);
    SHOW_T2_REG(TWL_VUSB_DEDICATED1);
    SHOW_T2_REG(TWL_VUSB_DEDICATED2);
    SHOW_T2_REG(TWL_REGEN_DEV_GRP);
    SHOW_T2_REG(TWL_REGEN_TYPE);
    SHOW_T2_REG(TWL_REGEN_REMAP);
    SHOW_T2_REG(TWL_NRESPWRON_DEV_GRP);
    SHOW_T2_REG(TWL_NRESPWRON_TYPE);
    SHOW_T2_REG(TWL_NRESPWRON_REMAP);
    SHOW_T2_REG(TWL_CLKEN_DEV_GRP);
    SHOW_T2_REG(TWL_CLKEN_TYPE);
    SHOW_T2_REG(TWL_CLKEN_REMAP);
    SHOW_T2_REG(TWL_SYSEN_DEV_GRP);
    SHOW_T2_REG(TWL_SYSEN_TYPE);
    SHOW_T2_REG(TWL_SYSEN_REMAP);
    SHOW_T2_REG(TWL_HFCLKOUT_DEV_GRP);
    SHOW_T2_REG(TWL_HFCLKOUT_TYPE);
    SHOW_T2_REG(TWL_HFCLKOUT_REMAP);
    SHOW_T2_REG(TWL_32KCLKOUT_DEV_GRP);
    SHOW_T2_REG(TWL_32KCLKOUT_TYPE);
    SHOW_T2_REG(TWL_32KCLKOUT_REMAP);
    SHOW_T2_REG(TWL_TRITON_RESET_DEV_GRP);
    SHOW_T2_REG(TWL_TRITON_RESET_TYPE);
    SHOW_T2_REG(TWL_TRITON_RESET_REMAP);
    SHOW_T2_REG(TWL_MAINREF_DEV_GRP);
    SHOW_T2_REG(TWL_MAINREF_TYPE);
    SHOW_T2_REG(TWL_MAINREF_REMAP);
*/

    // enable MADC and USB CP clock
    TWLWriteByteReg(hTwl, TWL_CFG_BOOT, 0x0A); // HFCLK_FREQ = 26MHz
    
    // secure registers
    TWLWriteByteReg(hTwl, TWL_PROTECT_KEY, 0x00);
    
    // vdd1 (mpu & dsp) - group 1
    TWLWriteByteReg(hTwl, TWL_VDD1_DEV_GRP, TWL_DEV_GROUP_P1);    
    TWLWriteByteReg(hTwl, TWL_VDD1_STEP, BSP_TWL_VDD1_STEP);
    TWLWriteByteReg(hTwl, TWL_VDD1_REMAP, 0);
        
    TWLWriteByteReg(hTwl, TWL_VDD2_DEV_GRP, TWL_DEV_GROUP_P1);
    TWLWriteByteReg(hTwl, TWL_VDD2_STEP, BSP_TWL_VDD2_STEP);
    TWLWriteByteReg(hTwl, TWL_VDD2_REMAP, 0);

    // EVM2: EHCI (1.8v) - group 1
    TWLWriteByteReg(hTwl, TWL_VAUX2_DEDICATED, TWL_VAUX2_1P80);
    TWLWriteByteReg(hTwl, TWL_VAUX2_DEV_GRP, TWL_DEV_GROUP_P1);

    // Camera CSI2 requires 1.8V from AUX4
    TWLWriteByteReg(hTwl, TWL_VAUX4_DEDICATED, 0x05);
    TWLWriteByteReg(hTwl, TWL_VAUX4_DEV_GRP, TWL_DEV_GROUP_P1);
    
    // LCD and backlight - group 1
    TWLWriteByteReg(hTwl, TWL_VAUX3_DEDICATED, TWL_VAUX3_2P80);
    TWLWriteByteReg(hTwl, TWL_VAUX3_DEV_GRP, TWL_DEV_GROUP_P1);

    // DSI,SDI power supplies driven by PLL2 
    // Configure for 1.8V instead of default 1.2V and move to P1
    TWLWriteByteReg(hTwl, TWL_VPLL2_DEDICATED, TWL_VPLL2_1P80);
    TWLWriteByteReg(hTwl, TWL_VPLL2_DEV_GRP, TWL_DEV_GROUP_P1);

    // sd / mmc - group 1
    val = DecideTwlVMMC(0);
    TWLWriteByteReg(hTwl, TWL_VMMC1_DEDICATED, (UINT8)val);
    TWLWriteByteReg(hTwl, TWL_VMMC1_DEV_GRP, TWL_DEV_GROUP_P1);

    val = DecideTwlVMMC(1);
    TWLWriteByteReg(hTwl, TWL_VMMC2_DEDICATED, (UINT8)val);
    TWLWriteByteReg(hTwl, TWL_VMMC2_DEV_GRP, TWL_DEV_GROUP_P1);

    // tv out - group 1 (1.8 v)
    TWLWriteByteReg(hTwl, TWL_VDAC_DEDICATED, TWL_VDAC_1P80);
    TWLWriteByteReg(hTwl, TWL_VDAC_DEV_GRP, TWL_DEV_GROUP_P1);

    // usb
    TWLWriteByteReg(hTwl, TWL_VUSB_DEDICATED1, 0x18);
    TWLWriteByteReg(hTwl, TWL_VUSB_DEDICATED2, 0x0);

    TWLWriteByteReg(hTwl, TWL_VUSB3V1_DEV_GRP, TWL_DEV_GROUP_P1);
    TWLWriteByteReg(hTwl, TWL_VUSB1V5_DEV_GRP, TWL_DEV_GROUP_P1);
    TWLWriteByteReg(hTwl, TWL_VUSB1V8_DEV_GRP, TWL_DEV_GROUP_P1);


    // Enable I2C access to the Power Bus 
    TWLWriteByteReg(hTwl, TWL_PB_CFG, 0x02);
    
    // disable vibrator
    val = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VAUX1_RES_ID, TWL_RES_OFF);
    TWLWriteByteReg(hTwl, TWL_PB_WORD_MSB, (UINT8)(val >> 8));
    TWLWriteByteReg(hTwl, TWL_PB_WORD_LSB, (UINT8)val);

    // enable camera
    val = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VAUX2_RES_ID, TWL_RES_ACTIVE);
    TWLWriteByteReg(hTwl, TWL_PB_WORD_MSB, (UINT8)(val >> 8));
    TWLWriteByteReg(hTwl, TWL_PB_WORD_LSB, (UINT8)val);

    // enable csi2 aux4 voltage output
    val = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VAUX4_RES_ID, TWL_RES_ACTIVE);
    TWLWriteByteReg(hTwl, TWL_PB_WORD_MSB, (UINT8)(val >> 8));
    TWLWriteByteReg(hTwl, TWL_PB_WORD_LSB, (UINT8)val);

    // disable LCD and backligt
    val = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VAUX3_RES_ID, TWL_RES_OFF);
    TWLWriteByteReg(hTwl, TWL_PB_WORD_MSB, (UINT8)(val >> 8));
    TWLWriteByteReg(hTwl, TWL_PB_WORD_LSB, (UINT8)val);

//    val = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VPLL2_RES_ID, TWL_RES_OFF);
//    TWLWriteByteReg(hTwl, TWL_PB_WORD_MSB, (UINT8)(val >> 8));
//    TWLWriteByteReg(hTwl, TWL_PB_WORD_LSB, (UINT8)val);

    // disable sd
//    val = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VMMC1_RES_ID, TWL_RES_OFF);
//    TWLWriteByteReg(hTwl, TWL_PB_WORD_MSB, (UINT8)(val >> 8));
//    TWLWriteByteReg(hTwl, TWL_PB_WORD_LSB, (UINT8)val);
    
    // enable sd
    val = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VMMC1_RES_ID, TWL_RES_ACTIVE);
    TWLWriteByteReg(hTwl, TWL_PB_WORD_MSB, (UINT8)(val >> 8));
    TWLWriteByteReg(hTwl, TWL_PB_WORD_LSB, (UINT8)val);

    val = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VMMC2_RES_ID, TWL_RES_OFF);
    TWLWriteByteReg(hTwl, TWL_PB_WORD_MSB, (UINT8)(val >> 8));
    TWLWriteByteReg(hTwl, TWL_PB_WORD_LSB, (UINT8)val);

    // disable usb 
    val = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VUSB_1V5_RES_ID, TWL_RES_OFF);
    TWLWriteByteReg(hTwl, TWL_PB_WORD_MSB, (UINT8)(val >> 8));
    TWLWriteByteReg(hTwl, TWL_PB_WORD_LSB, (UINT8)val);

    val = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VUSB_1V8_RES_ID, TWL_RES_OFF);
    TWLWriteByteReg(hTwl, TWL_PB_WORD_MSB, (UINT8)(val >> 8));
    TWLWriteByteReg(hTwl, TWL_PB_WORD_LSB, (UINT8)val);

    val = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VUSB_3V1_RES_ID, TWL_RES_OFF);
    TWLWriteByteReg(hTwl, TWL_PB_WORD_MSB, (UINT8)(val >> 8));
    TWLWriteByteReg(hTwl, TWL_PB_WORD_LSB, (UINT8)val);

    // Enable the update of VDD1 and VDD2 through Voltage Controller I2C 
    TWLWriteByteReg(hTwl, TWL_DCDC_GLOBAL_CFG, BSP_TWL_DCDC_GLOBAL_CFG);    

    TWLClose(hTwl);
}

BOOL BSPSetT2MSECURE(BOOL fSet)
{
#ifdef BSP_EVM2
    HANDLE hGPIO = GPIOOpen();
                
    if (hGPIO == NULL) return FALSE;
    
    if (fSet)
        GPIOSetBit(hGPIO,TPS659XX_MSECURE_GPIO);
    else
        GPIOClrBit(hGPIO,TPS659XX_MSECURE_GPIO);

    GPIOClose(hGPIO);
#else
    UNREFERENCED_PARAMETER(fSet);
#endif
    return TRUE;
}
//------------------------------------------------------------------------------
//
// end of bsp_tps659xx.c
