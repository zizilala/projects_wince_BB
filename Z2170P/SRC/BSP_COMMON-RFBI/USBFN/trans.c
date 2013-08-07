// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007 BSQUARE Corporation. All rights reserved.

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
//------------------------------------------------------------------------------
//
//  File:  trans.c
//
#pragma warning (push)
#pragma warning (disable: 4115 4201 4214)
#include <windows.h>
#include <oal.h>
#include <oalex.h>

#include "bsp.h"
#include "soc_cfg.h"
#include "omap_musbcore.h"

#ifdef BUILDING_KITL
#include <nkintr.h>
#include <initguid.h>
#include "omap_guid.h"
#endif
#pragma warning (pop)

#ifdef BSP_OMAP_MUSBOTG_TPS659XX
#include "tps659xxusb.h"
#include "twl.h"
#include "tps659xx.h"
#include "tps659xx_internals.h"
#endif

#ifdef BUILDING_KITL
    static OMAP_DEVCLKMGMT_FNTABLE s_OalDevClkMgmtTable;
    extern BOOL OEMIoControl(DWORD code, VOID * pInBuffer, DWORD inSize, VOID * pOutBuffer, DWORD outSize, DWORD * pOutSize);

    //------------------------------------------------------------------------------ 
    // 
    //  Function:  KernelIoControl() 
    // 
    BOOL KernelIoControl( 
        DWORD dwIoControlCode, 
        LPVOID lpInBuf, 
        DWORD nInBufSize, 
        LPVOID lpOutBuf, 
        DWORD nOutBufSize, 
        LPDWORD lpBytesReturned 
        ) 
    { 
        return OEMIoControl(dwIoControlCode,lpInBuf,nInBufSize,lpOutBuf,nOutBufSize,lpBytesReturned); 
    } 
#else
       extern BOOL EnableDeviceClocks(UINT devId, BOOL bEnable);
#endif

#ifdef BSP_OMAP_MUSBOTG_TPS659XX

#define CARKIT_CTRL_CARKITPWR                   (1 << 0)
#define CARKIT_CTRL_TXDEN                       (1 << 2)
#define CARKIT_CTRL_RXDEN                       (1 << 3)
#define CARKIT_CTRL_SPKLEFTEN                   (1 << 4)
#define CARKIT_CTRL_MICEN                       (1 << 6)

#define OAL_RNDIS                               FALSE
//------------------------------------------------------------------------------
//
//  Function:  PowerOnT2USBTransceiver()
//

BOOL 
PowerOnT2USBTransceiver( 
        HANDLE hTWL
    )
{
    BOOL  rc = FALSE;
    UCHAR value = 0;

    // Enable writing to Power Configuration registers
    value = 0xC0;
    if (!TWLWriteByteReg(hTWL, TWL_PROTECT_KEY, value))
        {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to power on T2 USB PHY\r\n"
            ));
        goto cleanUp;
        }

    value = 0x0C;
    if (!TWLWriteByteReg(hTWL, TWL_PROTECT_KEY, value))
        {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to power on T2 USB PHY\r\n"
            ));
        goto cleanUp;
        }

    value = 0x18;
    if (!TWLWriteByteReg(hTWL, TWL_VUSB_DEDICATED1, value))
        {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to wakeup T2 USB PHY\r\n"
            ));
        goto cleanUp;
        }

    // Exit from sleep
    value = 0;
    if (!TWLWriteByteReg(hTWL, TWL_VUSB_DEDICATED2, value))
        {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to wakeup T2 USB PHY\r\n"
            ));
        goto cleanUp;
        }

    // Setting for 3.1V regulator
    value = 0xE0;
    if (!TWLWriteByteReg(hTWL, TWL_VUSB3V1_DEV_GRP, value))
        {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to set T2 USB PHY 3.1V regualtor\r\n"
            ));
        goto cleanUp;
        }

    // Setting for 1.5V regulator
    value = 0xE0;
    if (!TWLWriteByteReg(hTWL, TWL_VUSB1V5_DEV_GRP, value))
        {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to set T2 USB PHY 1.5V regualtor\r\n"
            ));
        goto cleanUp;
        }

    // Setting for 1.8V regulator
    value = 0xE0;
    if (!TWLWriteByteReg(hTWL, TWL_VUSB1V8_DEV_GRP, value))
        {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to set T2 USB PHY 1.8V regualtor\r\n"
            ));
        goto cleanUp;
        }
    
    // Disable writing to Power Configuration registers
    value = 0x0;
    if (!TWLWriteByteReg(hTWL, TWL_PROTECT_KEY, value))
        {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to power on T2 USB PHY\r\n"
            ));
        goto cleanUp;
        }

    // enable usb regulators 
    {
        UINT16 value16;
        
        value16 = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VUSB_1V5_RES_ID, TWL_RES_ACTIVE);
        TWLWriteByteReg(hTWL, TWL_PB_WORD_MSB, (UINT8)(value16 >> 8));
        TWLWriteByteReg(hTWL, TWL_PB_WORD_LSB, (UINT8)value16);

        value16 = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VUSB_1V8_RES_ID, TWL_RES_ACTIVE);
        TWLWriteByteReg(hTWL, TWL_PB_WORD_MSB, (UINT8)(value16 >> 8));
        TWLWriteByteReg(hTWL, TWL_PB_WORD_LSB, (UINT8)value16);

        value16 = TwlTargetMessage(TWL_PROCESSOR_GRP1, TWL_VUSB_3V1_RES_ID, TWL_RES_ACTIVE);
        TWLWriteByteReg(hTWL, TWL_PB_WORD_MSB, (UINT8)(value16 >> 8));
        TWLWriteByteReg(hTWL, TWL_PB_WORD_LSB, (UINT8)value16);
    }
    
    // Done
    rc = TRUE;

cleanUp:

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  RequestPHYAccess()
//

BOOL 
RequestPHYAccess( 
        HANDLE hTWL
    )
{
    UCHAR value = 0;
	

    if (!TWLReadByteReg(hTWL, TWL_PHY_CLK_CTRL, &value))
        {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: RequestPHYAccess: "
            L"Failed to read PHY clock ctrl\r\n"
            ));
        return FALSE;
        }

    if (!(value & PHY_CLK_CTRL_REQ_PHY_DPLL_CLK))
    {
        value |= PHY_CLK_CTRL_REQ_PHY_DPLL_CLK;

        if (!TWLWriteByteReg(hTWL, TWL_PHY_CLK_CTRL, value))
            {
            OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: RequestPHYAccess: "
                L"Failed to request PHY access\r\n"
                ));
            return FALSE;
            }

        OALStall (2000);
    }

    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  InitTransceiver
//
BOOL 
InitTransceiver( 
        HANDLE hTWL
    )
{
    BOOL    rc = FALSE;
    UCHAR   value = 0;


    if (TWLReadByteReg(hTWL, TWL_VENDOR_ID_LO, &value))
        {
        OALMSG(OAL_RNDIS, (L"UfnPdd_Init: "
            L"Read T2 USB vendor ID LO = %x\r\n", value));
        }
    else
        {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to read T2 USB vendor ID LO 0x%x\r\n", value));
        }
    
    if (TWLReadByteReg(hTWL, TWL_VENDOR_ID_HI, &value))
        {
        OALMSG(OAL_RNDIS, (L"UfnPdd_Init: "
            L"Read T2 USB vendor ID HI = %x\r\n", value));
        }
    else
        {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to read T2 USB vendor ID HI 0x%x\r\n", value));
        }
    
    if (TWLReadByteReg(hTWL, TWL_PRODUCT_ID_LO, &value))
        {
        OALMSG(OAL_RNDIS, (L"UfnPdd_Init: "
            L"Read T2 USB product ID LO = %x\r\n", value));
        }
    else
        {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to read T2 USB product ID LO 0x%x\r\n", value));
        }
    
    if (TWLReadByteReg(hTWL, TWL_PRODUCT_ID_HI, &value))
        {
        OALMSG(OAL_RNDIS, (L"UfnPdd_Init: "
            L"Read T2 USB product ID HI = %x\r\n", value));
        }
    else
        {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: UfnPdd_Init: "
            L"Failed to read T2 USB product ID HI 0x%x\r\n", value));
        }


    // set for interrupt from ULPI Rx CMD to Link  
    value = OTHER_IFC_CTRL_ALT_INT_REROUTE;
    if (!TWLWriteByteReg(hTWL, TWL_OTHER_IFC_CTRL_SET, value))
        goto cleanUp;
    
    // Setting to IFC_CTRL register for ULPI operation mode
    value = IFC_CTRL_CARKITMODE | IFC_CTRL_FSLSSERIALMODE_3PIN;
    if (!TWLWriteByteReg(hTWL, TWL_IFC_CTRL_CLR, value))
        goto cleanUp;
        
    // Setting to TWL_CARKIT_CTRL_CLR register for ULPI operation mode
    value = CARKIT_CTRL_CARKITPWR |CARKIT_CTRL_TXDEN |CARKIT_CTRL_RXDEN;
    value |= CARKIT_CTRL_SPKLEFTEN |CARKIT_CTRL_MICEN;
    if (!TWLWriteByteReg(hTWL, TWL_CARKIT_CTRL_CLR, value))
        goto cleanUp;
        
    // Power on OTG
    value = POWER_CTRL_OTG_EN;
    if (!TWLWriteByteReg(hTWL, TWL_POWER_CTRL_SET, value))
        goto cleanUp;

    // Clear DM/DP PULLDOWN bits
    value = OTG_CTRL_DM_PULLDOWN | OTG_CTRL_DP_PULLDOWN;
    if (!TWLWriteByteReg(hTWL, TWL_OTG_CTRL_CLR, value))
        goto cleanUp;

    // Set OPMODE normal, XCVRSELECT: FS transceiver
    value = FUNC_CTRL_OPMODE_MASK | FUNC_CTRL_XCVRSELECT_MASK | FUNC_CTRL_TERMSELECT;
    if (!TWLWriteByteReg(hTWL, TWL_FUNC_CTRL_CLR, value))
        goto cleanUp;

    value = FUNC_CTRL_OPMODE_NORMAL | FUNC_CTRL_XCVRSELECT_FS;
    if (!TWLWriteByteReg(hTWL, TWL_FUNC_CTRL_SET, value))
        goto cleanUp;

    // Make sure PHY power enabled
    value = 0;
    if (!TWLWriteByteReg(hTWL, TWL_PHY_PWR_CTRL, value))
        goto cleanUp;

    // Done
    rc = TRUE;

cleanUp:

    return rc;
}
#endif

//------------------------------------------------------------------------------
//
//  Function:  InitializeHardware
//

BOOL InitializeHardware()
{
    BOOL   rc   = FALSE;

#ifdef BSP_OMAP_MUSBOTG_TPS659XX
    HANDLE hTWL;
#endif

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"+InitializeHardware\r\n"));

#ifdef BUILDING_KITL
    if (!OEMIoControl(IOCTL_PRCM_DEVICE_GET_DEVICEMANAGEMENTTABLE, (void*)&KERNEL_DEVCLKMGMT_GUID,
            sizeof(KERNEL_DEVCLKMGMT_GUID), &s_OalDevClkMgmtTable,
            sizeof(OMAP_DEVCLKMGMT_FNTABLE),
            NULL))
    {
        OALMSG(OAL_ERROR, (L"ERROR: KITL UfnPdd_Init: Failed get clock management table from kernel\r\n"));
        rc = FALSE;
    }
#endif

#ifdef BSP_OMAP_MUSBOTG_TPS659XX

    // Initialize Triton
    hTWL = TWLOpen();
    if (!hTWL)
    {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: InitializeHardware: "
            L"Failed to open TRITON II\r\n"
            ));
        goto cleanUp;
    }

    if(!PowerOnT2USBTransceiver(hTWL))
    {
        goto cleanUp;
    }
    
    if(!RequestPHYAccess(hTWL))
    {
        goto cleanUp;
     }

    // Configure transceiver
    if (!InitTransceiver (hTWL))
        {
        goto cleanUp;
        }

    // Done
    rc = TRUE;

cleanUp:
	
	// We are done with Triton
	if (hTWL)
	{
		TWLClose(hTWL);
	}

#endif

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"-InitializeHardware - rc = %d\r\n", rc));

    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  ConnectHardware
//
void ResetHardware()
{
}

//------------------------------------------------------------------------------
//
//  Function:  ConnectHardware
//

void ConnectHardware()
{
#ifdef BSP_OMAP_MUSBOTG_TPS659XX

    HANDLE hTWL;
    UCHAR   value = 0;

#endif

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"+ConnectHardware\r\n"));

#ifdef BSP_OMAP_MUSBOTG_TPS659XX

    // Initialize Triton
    hTWL = TWLOpen();
    if (!hTWL)
    {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: ConnectHardware: "
            L"Failed to open TRITON II\r\n"
            ));
        return;
    }

    value = FUNC_CTRL_TERMSELECT;

    // Enable D+ pullup resistor
    if (!TWLWriteByteReg(hTWL, TWL_FUNC_CTRL_SET, value))
    {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: ConnectHardware: "
            L"Failed to set TWL_FUNC_CTRL_SET\r\n"
            ));
    }

    // We are done with Triton
    TWLClose(hTWL);

#endif

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"-ConnectHardware\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  DisconnectHardware
//

void DisconnectHardware()
{
#ifdef BSP_OMAP_MUSBOTG_TPS659XX

    HANDLE hTWL;
    UCHAR   value = 0;

#endif

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"+DisconnectHardware\r\n"));

#ifdef BSP_OMAP_MUSBOTG_TPS659XX

    // Initialize Triton
    hTWL = TWLOpen();
    if (!hTWL)
    {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: DisconnectHardware: "
            L"Failed to open TRITON II\r\n"
            ));
        return;
    }

    value = FUNC_CTRL_TERMSELECT;

    // Disable D+ pullup resistor
    if(!TWLWriteByteReg(hTWL, TWL_FUNC_CTRL_CLR, value))
    {
        OALMSG(OAL_LOG_ERROR, (L"RNDIS ERROR: DisconnectHardware: "
            L"Failed to clr TWL_FUNC_CTRL_CLR\r\n"
            ));
    }

    // We are done with Triton
    TWLClose(hTWL);

#endif

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"-DisconnectHardware\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  EnableUSBClocks
//

BOOL EnableUSBClocks(BOOL bEnable)
{
	BOOL bRet = TRUE;

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"+EnableUSBClocks\r\n"));

#ifdef BUILDING_KITL

    bRet &= s_OalDevClkMgmtTable.pfnEnableDeviceIClock(OMAP_DEVICE_HSOTGUSB, bEnable);
    bRet &= s_OalDevClkMgmtTable.pfnEnableDeviceFClock(OMAP_DEVICE_HSOTGUSB, bEnable);

#else
	bRet = EnableDeviceClocks(OMAP_DEVICE_HSOTGUSB, bEnable);
#endif

    OALMSG(OAL_ETHER&&OAL_FUNC, (L"-EnableUSBClocks\r\n"));

	return bRet;
}

//------------------------------------------------------------------------------

