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
//  File:  bsp_padcfg.c
//
#include "bsp.h"
#include "omap3530_clocks.h"
#include "oal_padcfg.h"
#include "bsp_padcfg.h"
#include "oalex.h"

//------------------------------------------------------------------------------
//  Pad configuration
//------------------------------------------------------------------------------

static PAD_INFO g_allowedPadCfg[] = ALL_ALLOWED_PADS;

const PAD_INFO DSSPadConfig[] = {DSS_PADS END_OF_PAD_ARRAY};

const PAD_INFO DSSPadConfig_37XX[] = {DSS_PADS_37XX END_OF_PAD_ARRAY};

const PAD_INFO GPMCPads[] = {GPMC_PADS END_OF_PAD_ARRAY};

const PAD_INFO UART1Pads[] = {UART1_PADS END_OF_PAD_ARRAY};

const PAD_INFO UART2Pads[] = {UART2_PADS END_OF_PAD_ARRAY};

const PAD_INFO UART3Pads[] = {UART3_PADS END_OF_PAD_ARRAY};

const PAD_INFO MMC1Pads[] = {MMC1_PADS  END_OF_PAD_ARRAY};

const PAD_INFO I2C1Pads[] = {I2C1_PADS  END_OF_PAD_ARRAY};

const PAD_INFO I2C2Pads[] = {I2C2_PADS  END_OF_PAD_ARRAY};

const PAD_INFO I2C3Pads[] = {I2C3_PADS  END_OF_PAD_ARRAY};

const PAD_INFO I2C4Pads[] = {I2C4_PADS  END_OF_PAD_ARRAY};

const PAD_INFO MCSPI1Pads[] = {MCSPI1_PADS END_OF_PAD_ARRAY};

const PAD_INFO MCBSP1Pads[] = {MCBSP1_PADS END_OF_PAD_ARRAY};

const PAD_INFO MCBSP2Pads[] = {MCBSP2_PADS END_OF_PAD_ARRAY};

const PAD_INFO USBHost1Pads[] = {USBH1_PADS END_OF_PAD_ARRAY};

const PAD_INFO USBHost2Pads[] = {USBH2_PADS END_OF_PAD_ARRAY};

const PAD_INFO USBOTGPads[] = {USBOTG_PADS END_OF_PAD_ARRAY};

const PAD_INFO CameraPads[] =  {CAMERA_PADS END_OF_PAD_ARRAY};

PAD_INFO* BSPGetAllPadsInfo()
{    
    return g_allowedPadCfg;
}
const PAD_INFO* BSPGetDevicePadInfo(OMAP_DEVICE device)
{    
    switch (device)
    {
        case OMAP_DEVICE_DSS:
        {
            if (g_dwCpuFamily == CPU_FAMILY_DM37XX)
                return DSSPadConfig_37XX;
            else
                return DSSPadConfig;
        }
        case OMAP_DEVICE_GPMC: return GPMCPads;
        case OMAP_DEVICE_UART1: return UART1Pads;
        case OMAP_DEVICE_UART2: return UART2Pads;
    	case OMAP_DEVICE_UART3: return UART3Pads;
        case OMAP_DEVICE_MMC1: return MMC1Pads;
        case OMAP_DEVICE_I2C1: return I2C1Pads;
        case OMAP_DEVICE_I2C2: return I2C2Pads;
        case OMAP_DEVICE_I2C3: return I2C3Pads;
        case OMAP_DEVICE_MCSPI1: return MCSPI1Pads;
    	case OMAP_DEVICE_MCBSP1: return MCBSP1Pads;
    	case OMAP_DEVICE_MCBSP2: return MCBSP2Pads;
    	case OMAP_DEVICE_USBHOST1: return USBHost1Pads;
    	case OMAP_DEVICE_USBHOST2: return USBHost2Pads;
    	case OMAP_DEVICE_HSOTGUSB : return USBOTGPads;
        case OMAP_DEVICE_CAMERA: return CameraPads;
    }
    return NULL;
}
