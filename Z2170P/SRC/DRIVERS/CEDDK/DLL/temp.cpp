// All rights reserved ADENEO EMBEDDED 2010
#include "bsp.h"
#include "ceddkex.h"
#include "oalex.h"
#include "bsp_cfg.h"

//------------------------------------------------------------------------------
//
//  Function:  BusClockRequest
//
BOOL
BusClockRequest(
    HANDLE hBus, 
    UINT id
    )
{
    UNREFERENCED_PARAMETER(hBus);
    UNREFERENCED_PARAMETER(id);
   return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  BusClkRelease
//
BOOL
BusClockRelease(
    HANDLE hBus, 
    UINT id
    )
{
    UNREFERENCED_PARAMETER(hBus);
    UNREFERENCED_PARAMETER(id);
    return TRUE;
}

//------------------------------------------------------------------------------
//
//  Function:  BusSourceClocks
//
BOOL
BusSourceClocks(
                HANDLE hBus, 
                UINT id,
                UINT count,
                UINT rgSourceClocks[]
)
{
    UNREFERENCED_PARAMETER(hBus);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(count);
    UNREFERENCED_PARAMETER(rgSourceClocks);
    return TRUE;
}

void
GetDisplayResolutionFromBootArgs(
    DWORD * pDispRes
    )
{    
    DWORD   dwKernelRet = 0;
    if (!KernelIoControl(IOCTL_HAL_GET_DISPLAY_RES,
                         NULL, 0, pDispRes, sizeof(DWORD), &dwKernelRet))
    {
        RETAILMSG( TRUE,(TEXT("Failed to read Display resolution\r\n")));
        return;
    }   
}

BOOL
IsDVIMode()
{
    DWORD dispRes;    
    GetDisplayResolutionFromBootArgs(&dispRes);
    if (dispRes==OMAP_LCD_DEFAULT)
        return FALSE;
    else
        return TRUE;        
}

DWORD 
ConvertCAtoPA(DWORD * va)
{
    DWORD   dwKernelRet = 0;
    DWORD   pa = 0;
    if (!KernelIoControl(IOCTL_HAL_CONVERT_CA_TO_PA,
                         va, sizeof(DWORD), &pa, sizeof(DWORD), &dwKernelRet))
    {
        RETAILMSG( TRUE,(TEXT("Failed to convert CA to PA\r\n")));
        return 0;
    }
    else
    {
        return pa;
    }
}

