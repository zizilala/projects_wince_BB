// All rights reserved ADENEO EMBEDDED 2010
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
//  File: null_batt_pdd.c
//
//  Null battery driver PDD.
//
#include "bsp.h"
#undef ZONE_ERROR
#undef ZONE_INIT
#include <battimpl.h>
#include <pm.h>
#include "ceddkex.h"

//#include <initguid.h>

//------------------------------------------------------------------------------
//
//  Function:  BatteryPDDInitialize
//
BOOL WINAPI 
BatteryPDDInitialize(
    LPCTSTR szContext
    )
{
    UNREFERENCED_PARAMETER(szContext);
    
    RETAILMSG(1, (L"\r\n+BatteryPDDInitialize()\r\n"));
    RETAILMSG(1, (L"-BatteryPDDInitialize\r\n"));
    return TRUE;
}


//------------------------------------------------------------------------------
//
//  Function:  BatteryPDDDeinitialize
//
void WINAPI 
BatteryPDDDeinitialize()
{
    RETAILMSG(1, (L"+BatteryPDDDeinitialize\r\n"));
    RETAILMSG(1, (L"-BatteryPDDDeinitialize\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function:  BatteryPDDResume
//
void WINAPI 
BatteryPDDResume()
{
    RETAILMSG(1, (L"+BatteryPDDResume\r\n"));
    RETAILMSG(1, (L"-BatteryPDDResume\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function:  BatteryPDDPowerHandler
//
void WINAPI 
BatteryPDDPowerHandler(
    BOOL off
    )
{
    UNREFERENCED_PARAMETER(off);
    RETAILMSG(1, (L"+BatteryPDDPowerHandler(%d)\r\n",  off));
    RETAILMSG(1, (L"-BatteryPDDPowerHandler\r\n"));
}


//------------------------------------------------------------------------------
//
//  Function: BatteryPDDGetLevels
//
//  Indicates how many battery levels will be reported by BatteryPDDGetStatus()
//  in the BatteryFlag and BackupBatteryFlag fields of PSYSTEM_POWER_STATUS_EX2.
//
//  Returns the main battery level in the low word, and the backup battery
//  level in the high word.
//
LONG 
BatteryPDDGetLevels()
{
    LONG lLevels = MAKELONG(
        1,      // Main battery levels
        0       // Backup battery levels
    );
    return lLevels;
}


//------------------------------------------------------------------------------
//
//  Function: BatteryPDDSupportsChangeNotification
//
//  Returns FALSE since this platform does not support battery change
//  notification.
//
BOOL 
BatteryPDDSupportsChangeNotification()
{
    return FALSE;
}


//------------------------------------------------------------------------------
//
//  Function: BatteryPddIOControl
//
//  Battery driver needs to handle D0-D4 power notifications
//
//  Returns ERROR code.
//
DWORD
BatteryPddIOControl(
    DWORD  dwContext,
    DWORD  Ioctl,
    PUCHAR pInBuf,
    DWORD  InBufLen, 
    PUCHAR pOutBuf,
    DWORD  OutBufLen,
    PDWORD pdwBytesTransferred
    )
{
    DWORD dwRet;

    UNREFERENCED_PARAMETER(dwContext);
    UNREFERENCED_PARAMETER(Ioctl);
    UNREFERENCED_PARAMETER(pInBuf);
    UNREFERENCED_PARAMETER(InBufLen);
    UNREFERENCED_PARAMETER(pOutBuf);
    UNREFERENCED_PARAMETER(OutBufLen);
    UNREFERENCED_PARAMETER(pdwBytesTransferred);
    
    switch (Ioctl)
        {
        default:
            dwRet = ERROR_NOT_SUPPORTED;
        }
    return dwRet;
}


//------------------------------------------------------------------------------
//
//  Function:  BatteryPDDGetStatus()
//
//  Obtains the battery and power status.
//
BOOL WINAPI 
BatteryPDDGetStatus(
    SYSTEM_POWER_STATUS_EX2 *pStatus, 
    BOOL *pBatteriesChangedSinceLastCall
    ) 
{
    BOOL rc = TRUE;

    RETAILMSG(1, (L"+BatteryPDDGetStatus\r\n"));
    RETAILMSG(1, (TEXT("BatteryPDDGetStatus: ...\r\n")));

    pStatus->ACLineStatus               = AC_LINE_ONLINE;
    pStatus->BatteryFlag                = BATTERY_FLAG_NO_BATTERY;
    pStatus->BatteryLifePercent         = BATTERY_PERCENTAGE_UNKNOWN;
    pStatus->Reserved1                  = 0;
    pStatus->BatteryLifeTime            = BATTERY_LIFE_UNKNOWN;
    pStatus->BatteryFullLifeTime        = BATTERY_LIFE_UNKNOWN;

    pStatus->Reserved2                  = 0;
    pStatus->Reserved3                  = 0;    
    pStatus->BackupBatteryLifeTime      = BATTERY_LIFE_UNKNOWN;
    pStatus->BackupBatteryFullLifeTime  = BATTERY_LIFE_UNKNOWN;
    
    pStatus->BackupBatteryFlag          = BATTERY_FLAG_HIGH;
    pStatus->BackupBatteryLifePercent   = 100;

    pStatus->BatteryChemistry           = BATTERY_CHEMISTRY_UNKNOWN;
    pStatus->BatteryVoltage             = 0;
    pStatus->BatteryCurrent             = 0;
    pStatus->BatteryAverageCurrent      = 0;
    pStatus->BatteryAverageInterval     = 0;
    pStatus->BatterymAHourConsumed      = 0;
    pStatus->BatteryTemperature         = 250;  // unit is 0.1 deg C
    pStatus->BackupBatteryVoltage       = 0;

    *pBatteriesChangedSinceLastCall = FALSE;

    RETAILMSG(1, (L"-BatteryPDDGetStatus(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
