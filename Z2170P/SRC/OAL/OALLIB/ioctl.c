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
//  This file implements the OEM's IO Control (IOCTL) functions and declares
//  global variables used by the IOCTL component.
//
#include "bsp.h"
#include "bsp_version.h"
#include <oal.h>
#include "oalex.h"
#include <bldver.h>
#include "sdk_i2c.h"
#include <pkfuncs.h>
#include "omap3530_oal.h"

//------------------------------------------------------------------------------
//
//  Global:  g_oalIoCtlPlatformType/OEM
//
//  Platform Type/OEM
//

LPCWSTR g_oalIoCtlPlatformType = L"OMAP35xx";
LPCWSTR g_oalIoCtlPlatformOEM  = L"Texas Instruments";

//------------------------------------------------------------------------------
//
// Global: g_oalIoctlPlatformManufacturer/Name
//
//

LPCWSTR g_oalIoCtlPlatformManufacturer = L"Texas Instruments";
LPCWSTR g_oalIoCtlPlatformName = L"EVM OMAP35xx";


//------------------------------------------------------------------------------
//
//  Global:  g_oalIoCtlProcessorVendor/Name/Core
//
//  Processor information
//

LPCWSTR g_oalIoCtlProcessorVendor = L"Texas Instruments";
LPCWSTR g_oalIoCtlProcessorName   = L"OMAP3530";
LPCWSTR g_oalIoCtlProcessorCore   = L"Cortex-A8";

//------------------------------------------------------------------------------
//
//  Global:  g_oalIoctlInstructionSet/ClockSpeed
//
//  Processor instruction set identifier and maximal CPU speed
//
UINT32 g_oalIoCtlInstructionSet = PROCESSOR_FLOATINGPOINT;
UINT32 g_oalIoCtlClockSpeed = 0;

//------------------------------------------------------------------------------
//
//  Function:  BSPIoCtlHalInitRegistry
//
//  Implements the IOCTL_HAL_INITREGISTRY handler.

BOOL BSPIoCtlHalInitRegistry(
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer,
    UINT32 outSize, UINT32 *pOutSize
)
{
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pInpBuffer);
    UNREFERENCED_PARAMETER(inpSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);

    if (g_oalKitlEnabled == FALSE)
    {
        // If KITL isn't enabled, ensure that USB and Ethernet drivers are not
        // blocked.  This logic prevents a persistent registry from inadvertently blocking
        // these drivers when KITL has been removed from an image.
#if 0
        OEMEthernetDriverEnable(TRUE);
        OEMUsbDriverEnable(TRUE);
#endif
    }

    OALIoCtlHalInitRTC(IOCTL_HAL_INIT_RTC, NULL, 0, NULL, 0, NULL);

    return TRUE;
}

extern void DumpPrcmRegs();

//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalDumpRegisters
//
//
BOOL OALIoCtlHalDumpRegisters(UINT32 code, VOID *pInpBuffer,
    UINT32 inpSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
{
    BOOL rc = TRUE;
    
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalDumpRegisters\r\n"));

    // Check input parameters
    if (pInpBuffer == NULL || inpSize != sizeof(DWORD))
        {
        goto cleanUp;
        }

    // could use input parameter to select device to dump (not implemented)
    switch (*(DWORD *)pInpBuffer)
	{
        case IOCTL_HAL_DUMP_REGISTERS_PRCM:
		    DumpPrcmRegs();
			break;
			
		default:
		    rc = FALSE;
	}

cleanUp:
    OALMSG(OAL_IOCTL && OAL_FUNC, (
        L"-OALIoCtlHalDumpRegisters(rc = %d)\r\n", rc
    ));
    return rc;

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//  Global:  g_oalIoCtlTable[]
//
//  IOCTL handler table. This table includes the IOCTL code/handler pairs
//  defined in the IOCTL configuration file. This global array is exported
//  via oal_ioctl.h and is used by the OAL IOCTL component.
//
const OAL_IOCTL_HANDLER g_oalIoCtlTable[] = {
#include "ioctl_tab.h"
};


//------------------------------------------------------------------------------
//
//  Function: OALIoCtlHalGetBspVersion
//
//
BOOL OALIoCtlHalGetBspVersion(UINT32 code, VOID *pInpBuffer,
    UINT32 inpSize, VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize)
{
    BOOL                        rc = FALSE;
    IOCTL_HAL_GET_BSP_VERSION_OUT*    pBspVersion;


    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(inpSize);
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pInpBuffer);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalGetBspVersion\r\n"));

    // Check input parameters
    if (
        pOutBuffer == NULL || outSize < sizeof(IOCTL_HAL_GET_BSP_VERSION_OUT)
    ) {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (
            L"WARN: IOCTL_HAL_GET_BSP_VERSION_OUT invalid parameters\r\n"
        ));
        goto cleanUp;
    }

    //  Copy to passed in struct
    pBspVersion = (IOCTL_HAL_GET_BSP_VERSION_OUT*)pOutBuffer;

    pBspVersion->dwVersionMajor    = BSP_VERSION_MAJOR;
    pBspVersion->dwVersionMinor    = BSP_VERSION_MINOR;
	pBspVersion->dwVersionQFES	   = BSP_VERSION_QFES;
    pBspVersion->dwVersionIncremental = BSP_VERSION_INCREMENTAL;

    rc = TRUE;

cleanUp:
    OALMSG(OAL_IOCTL && OAL_FUNC, (
        L"-OALIoCtlHalGetBspVersion(rc = %d)\r\n", rc
    ));
    return rc;

}
//------------------------------------------------------------------------------
