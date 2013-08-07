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
//  File:  bsp_bootstubs.c
//

// stub routines which exist in full images but not in bootloader images

//-----------------------------------------------------------------------------

#include "bsp.h"
#include "bsp_cfg.h"
#include "twl.h"
//-----------------------------------------------------------------------------
BOOL
INTERRUPTS_STATUS()
{
    return FALSE;
}
#if 0
//------------------------------------------------------------------------------
// dummy implementation of LocalAlloc. It alocates bunch of memory without providing 
// any way to release it
HLOCAL 
LocalAlloc(
    UINT flags, 
    UINT size
    )
{
    HLOCAL hResult;
    static UINT8 heap[512];
    static DWORD remaining = sizeof(heap);    
    
    UNREFERENCED_PARAMETER(flags);
#define ALIGN_MASK  0x3
    size = (size + ALIGN_MASK) & ~ALIGN_MASK;
    if (size > remaining)
    {
        ERRORMSG(1,(TEXT("LocalAlloc failed (0x%x vs 0x%x)\r\n"), size,remaining));
        return NULL;
    }
    hResult = (HLOCAL) (heap + sizeof(heap) - remaining);
    remaining -= size;
    memset(hResult,0,size);
    return hResult;
}    

//------------------------------------------------------------------------------
HLOCAL 
LocalFree(
    HLOCAL hMemory 
    )
{    
    UNREFERENCED_PARAMETER(hMemory);
    return NULL;
}
#endif
//-----------------------------------------------------------------------------
BOOL CloseHandle(
  HANDLE hObject
)
{
    UNREFERENCED_PARAMETER(hObject);
    return TRUE;
}

//-----------------------------------------------------------------------------
void 
WINAPI 
EnterCriticalSection(
    LPCRITICAL_SECTION lpcs
    )  
{
    UNREFERENCED_PARAMETER(lpcs);
}

//-----------------------------------------------------------------------------
void 
WINAPI 
LeaveCriticalSection(
    LPCRITICAL_SECTION lpcs
    )  
{
    UNREFERENCED_PARAMETER(lpcs);
}


//-----------------------------------------------------------------------------
void 
WINAPI 
InitializeCriticalSection(
    LPCRITICAL_SECTION lpcs
    )  
{
    UNREFERENCED_PARAMETER(lpcs);
}
//-----------------------------------------------------------------------------

void DeleteCriticalSection(
  LPCRITICAL_SECTION lpCriticalSection
)
{
    UNREFERENCED_PARAMETER(lpCriticalSection);
}

//-----------------------------------------------------------------------------
HANDLE
WINAPI   
SC_CreateMutex(
    LPSECURITY_ATTRIBUTES lpsa, 
    BOOL bInitialOwner, 
    LPCTSTR lpName
    )
{
    UNREFERENCED_PARAMETER(lpsa);
    UNREFERENCED_PARAMETER(bInitialOwner);
    UNREFERENCED_PARAMETER(lpName);
    return NULL;
}

//-----------------------------------------------------------------------------
DWORD
WINAPI   
SC_WaitForMultiple(
    DWORD cObjects, 
    CONST HANDLE *lphObjects, 
    BOOL fWaitAll, 
    DWORD dwTimeout
    )
{
    UNREFERENCED_PARAMETER(cObjects);
    UNREFERENCED_PARAMETER(lphObjects);
    UNREFERENCED_PARAMETER(fWaitAll);
    UNREFERENCED_PARAMETER(dwTimeout);
    return 0;
}

//-----------------------------------------------------------------------------
BOOL
WINAPI
SC_ReleaseMutex(
    HANDLE hMutex)
{
    UNREFERENCED_PARAMETER(hMutex);
    return TRUE;
}

#ifdef BOOT_XLDR
//-----------------------------------------------------------------------------

BOOL
EnableDeviceClocks(
    UINT devId, 
    BOOL bEnable
    )
{
    OMAP_CM_REGS* pCmRegs;

    switch (devId)
        {
        case OMAP_DEVICE_MMC1:
            pCmRegs = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_MMC1);
                SETREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_MMC1);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_MMC1);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_MMC1);
                CLRREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_MMC1);
                }
            break;

        case OMAP_DEVICE_I2C1:
            pCmRegs = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
            if (bEnable)
                {
                SETREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_I2C1);
                SETREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_I2C1);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_I2C1);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_I2C1);
                CLRREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_I2C1);
                }
                
            break;

        case OMAP_DEVICE_GPIO1:
            pCmRegs = OALPAtoUA(OMAP_PRCM_WKUP_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPIO1);
                SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPIO1);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_GPIO1);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPIO1);
                CLRREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPIO1);
                }
            break;
         
        }
    return TRUE;
}

#else
//-----------------------------------------------------------------------------

BOOL
EnableDeviceClocks(
    UINT devId, 
    BOOL bEnable
    )
{
    OMAP_CM_REGS* pCmRegs;

    switch (devId)
        {
/*
        case OMAP_DEVICE_GPTIMER1:
            pCmRegs = OALPAtoUA(OMAP_PRCM_WKUP_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPT1);
                SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPT1);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_GPT1);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_GPT1);
                CLRREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_GPT1);
                }
            break;
*/        

        case OMAP_DEVICE_GPTIMER2:
            pCmRegs = OALPAtoUA(OMAP_PRCM_PER_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPT2);
                SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPT2);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_GPT2);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPT2);
                CLRREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPT2);
                }
            break;

        case OMAP_DEVICE_MMC1:
            pCmRegs = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_MMC1);
                SETREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_MMC1);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_MMC1);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_MMC1);
                CLRREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_MMC1);
                }
            break;

        case OMAP_DEVICE_MMC2:
            pCmRegs = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_MMC2);
                SETREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_MMC2);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_MMC2);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_MMC2);
                CLRREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_MMC2);
                }
            break;

        case OMAP_DEVICE_I2C1:
            pCmRegs = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
            if (bEnable)
                {
                SETREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_I2C1);
                SETREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_I2C1);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_I2C1);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_I2C1);
                CLRREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_I2C1);
                }
                
            break;

        case OMAP_DEVICE_I2C2:
            pCmRegs = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
            if (bEnable)
                {
                SETREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_I2C2);
                SETREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_I2C2);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_I2C2);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_I2C2);
                CLRREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_I2C2);
                }
            break;

        case OMAP_DEVICE_I2C3:
            pCmRegs = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_I2C3);
                SETREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_I2C3);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_I2C3);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_I2C3);
                CLRREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_I2C3);
                }
            break;
/*
        case OMAP_DEVICE_MCSPI1:
            pCmRegs = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_MCSPI1);
                SETREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_MCSPI1);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_MCSPI1);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_MCSPI1);
                CLRREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_MCSPI1);
                }
            break;
*/
        case OMAP_DEVICE_UART1:
            pCmRegs = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_UART1);
                SETREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_UART1);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_UART1);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_UART1);
                CLRREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_UART1);
                }
            break;

        case OMAP_DEVICE_UART2:
            pCmRegs = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_UART2);
                SETREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_UART2);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_UART2);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_UART2);
                CLRREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_UART2);
                }
            break;

        case OMAP_DEVICE_UART3:
            pCmRegs = OALPAtoUA(OMAP_PRCM_PER_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_UART3);
                SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_UART3);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_UART3);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_UART3);
                CLRREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_UART3);
                }
            break;

        case OMAP_DEVICE_UART4:
            pCmRegs = OALPAtoUA(OMAP_PRCM_PER_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_UART4);
                SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_UART4);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_UART4);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_UART4);
                CLRREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_UART4);
                }
            break;

        case OMAP_DEVICE_GPIO1:
            pCmRegs = OALPAtoUA(OMAP_PRCM_WKUP_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPIO1);
                SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPIO1);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_GPIO1);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPIO1);
                CLRREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPIO1);
                }
            break;

        case OMAP_DEVICE_GPIO2:
            pCmRegs = OALPAtoUA(OMAP_PRCM_PER_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPIO2);
                SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPIO2);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_GPIO2);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPIO2);
                CLRREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPIO2);
                }
            break;

        case OMAP_DEVICE_GPIO3:
            pCmRegs = OALPAtoUA(OMAP_PRCM_PER_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPIO3);
                SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPIO3);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_GPIO3);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPIO3);
                CLRREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPIO3);
                }
            break;

        case OMAP_DEVICE_GPIO4:
            pCmRegs = OALPAtoUA(OMAP_PRCM_PER_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPIO4);
                SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPIO4);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_GPIO4);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPIO4);
                CLRREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPIO4);
                }
            break;

        case OMAP_DEVICE_GPIO5:
            pCmRegs = OALPAtoUA(OMAP_PRCM_PER_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPIO5);
                SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPIO5);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_GPIO5);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPIO5);
                CLRREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPIO5);
                }
            break;

        case OMAP_DEVICE_GPIO6:
            pCmRegs = OALPAtoUA(OMAP_PRCM_PER_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPIO6);
                SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPIO6);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_GPIO6);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_GPIO6);
                CLRREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_GPIO6);
                }
            break;

		case OMAP_DEVICE_MCBSP1:
            pCmRegs = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_MCBSP1);
                SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_MCBSP1);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_MCBSP1);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_MCBSP1);
                CLRREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_MCBSP1);
                }
            break;
/*
        case OMAP_DEVICE_DSS:
            pCmRegs = OALPAtoUA(OMAP_PRCM_DSS_CM_REGS_PA);
            if (bEnable)
                {                
                SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_DSS1 | CM_CLKEN_DSS2 | CM_CLKEN_TV);
                SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_DSS);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_DSS1 | CM_CLKEN_DSS2);
                CLRREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_DSS);
                }
            break;
*/
        case OMAP_DEVICE_HSOTGUSB:
            pCmRegs = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
            if (bEnable)
                {
                SETREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_HSOTGUSB);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_HSOTGUSB_IDLE);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_HSOTGUSB);
                }
            break;
/*
        case OMAP_DEVICE_D2D:
            pCmRegs = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
            if (bEnable)
                {
                SETREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_D2D);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_D2D);
                }
            else
                {
                CLRREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_D2D);
                }
            break;
*/    
        }
    return TRUE;
}
#endif

BOOL
PrcmDeviceGetContextState(
    UINT                    devId, 
    BOOL                    bSet
    )
{
    UNREFERENCED_PARAMETER(devId);
    UNREFERENCED_PARAMETER(bSet);
    return TRUE;

}

BOOL
EnableDeviceClocksNoRefCount(
    UINT devId, 
    BOOL bEnable
    )
{
	return EnableDeviceClocks( devId, bEnable );
}

BOOL
BusClockRelease(
    HANDLE hBus, 
    UINT id
    )
{
//    RETAILMSG(1,(TEXT("clockrelease %d\r\n"),id));
    UNREFERENCED_PARAMETER(hBus);
    return EnableDeviceClocks(id,FALSE);
}

BOOL
BusClockRequest(
    HANDLE hBus, 
    UINT id
    )
{
//    RETAILMSG(1,(TEXT("clockrequest %d\r\n"),id));
    UNREFERENCED_PARAMETER(hBus);
    return EnableDeviceClocks(id,TRUE);
}

VOID MmUnmapIoSpace( 
  PVOID BaseAddress, 
  ULONG NumberOfBytes 
)
{
    UNREFERENCED_PARAMETER(BaseAddress);
    UNREFERENCED_PARAMETER(NumberOfBytes);
}

PVOID MmMapIoSpace( 
  PHYSICAL_ADDRESS PhysicalAddress, 
  ULONG NumberOfBytes, 
  BOOLEAN CacheEnable 
)
{
    UNREFERENCED_PARAMETER(NumberOfBytes);
    return OALPAtoVA(PhysicalAddress.LowPart,CacheEnable);
}

HANDLE CreateBusAccessHandle (
  LPCTSTR lpActiveRegPath
)
{    
    UNREFERENCED_PARAMETER(lpActiveRegPath);
    return (HANDLE) 0xAA;
}

void
HalContextUpdateDirtyRegister(
    UINT32 ffRegister
    )
{
    UNREFERENCED_PARAMETER(ffRegister);
}


//-----------------------------------------------------------------------------
