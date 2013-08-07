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
//  File:  main.c
//
//  This file contains X-Loader implementation for OMAP35XX
//
#include "bsp.h"
#include "bsp_oal.h"

#pragma warning(push)
#pragma warning(disable: 4201)
#include <blcommon.h>
#pragma warning(pop)


#include "oalex.h"
#include "boot_args.h"
#include "sdk_i2c.h"
#include "sdk_gpio.h"

#define OAL
#include "oal_alloc.h"
#include "oal_clock.h"
#include "bsp_cfg.h"

//------------------------------------------------------------------------------
#ifdef SHIP_BUILD
    #define XLDRMSGINIT
    #define XLDRMSGDEINIT
    #define XLDRMSG(msg)
#else
	#define XLDRMSGINIT         OEMInitDebugSerial()
    #define XLDRMSGDEINIT       OEMDeinitDebugSerial()
    #define XLDRMSG(msg)        OEMWriteDebugString(msg)
#endif

//------------------------------------------------------------------------------
// External Variables
extern DEVICE_IFC_GPIO Omap_Gpio;
extern DEVICE_IFC_GPIO Tps659xx_Gpio;

//------------------------------------------------------------------------------
//  Global variables
ROMHDR * volatile const pTOC = (ROMHDR *)-1;
const volatile DWORD dwOEMHighSecurity = OEM_HIGH_SECURITY_GP;
BOOT_CFG g_bootCfg;

unsigned int  gCPU_family;
//------------------------------------------------------------------------------
//  External Functions
extern VOID PlatformSetup();
extern VOID JumpTo();
extern VOID EnableCache_GP();
extern VOID EnableCache_HS();

extern UINT32 BLSDCardDownload(WCHAR *filename);
extern BOOL   BLSDCardReadData(ULONG size, UCHAR *pData);

//------------------------------------------------------------------------------
//  Local Functions
static BOOL SetupCopySection(ROMHDR *const pTableOfContents);
static void OutputNumHex(unsigned long n, long depth);

void BSPGpioInit()
{
	XLDRMSG( TEXT("BSPGpioInit()\r\n"));
	BSPInsertGpioDevice(0,&Omap_Gpio,NULL);
	BSPInsertGpioDevice(TRITON_GPIO_PINID_START,&Tps659xx_Gpio,NULL);
}
//------------------------------------------------------------------------------
//
//  Function:  XLDRMain
//
VOID XLDRMain()
{
    //OMAP_GPTIMER_REGS *pTimerRegs;
    static UCHAR allocationPool[512];
    LPCWSTR ProcessorName   = L"3530";

    // Setup global variables
    if (!SetupCopySection(pTOC))
        goto CleanUp;

    OALLocalAllocInit(allocationPool,sizeof(allocationPool));

    //  Enable cache based on device type
    if( dwOEMHighSecurity == OEM_HIGH_SECURITY_HS )
    {
        EnableCache_HS();
    }
    else
    {
        EnableCache_GP();
    }
    
    gCPU_family = CPU_FAMILY(Get_CPUVersion());
    if( gCPU_family == CPU_FAMILY_DM37XX)
    {
       ProcessorName = L"37XX";
    }

    PlatformSetup();

    // Initialize debug serial output
    XLDRMSGINIT;

    // Print information...
    XLDRMSG( TEXT("\r\nZEBEX Windows CE SD X-Loader for Z-2170 plus - Brian"));    
    XLDRMSG( TEXT("\r\n") TEXT("Built ") TEXT(__DATE__) TEXT(" at ") TEXT(__TIME__) TEXT(" ("));
	XLDRMSG( (UINT16 *)ProcessorName);
	XLDRMSG( TEXT(")\r\n"));
    GPIOInit();	// \COMMON_TI_V1\COMMON_TI\GPIO\DISPATCH\OAL\gpio_dispatch.c
    /*pTimerRegs = (OMAP_GPTIMER_REGS *)OALPAtoUA(OMAP_GPTIMER1_REGS_PA); // General-Purpose Timers

    // Soft reset GPTIMER1
    OUTREG32(&pTimerRegs->TIOCP, SYSCONFIG_SOFTRESET); 
	// check GPTIMER1 reset completed ?
    while ((INREG32(&pTimerRegs->TISTAT) & GPTIMER_TISTAT_RESETDONE) == 0)
		;

    // Enable posted mode
    OUTREG32(&pTimerRegs->TSICR, GPTIMER_TSICR_POSTED);
    // Start timer
    OUTREG32(&pTimerRegs->TCLR, GPTIMER_TCLR_AR|GPTIMER_TCLR_ST);
    // need to init OAL tick functions
*/

    memset(&g_bootCfg, 0, sizeof(g_bootCfg));
    //g_bootCfg.signature = BOOT_CFG_SIGNATURE;
    //g_bootCfg.version = BOOT_CFG_VERSION;
    //g_bootCfg.bootDevLoc.IfcType = Internal;
    //g_bootCfg.bootDevLoc.BusNumber = 0;
    //g_bootCfg.bootDevLoc.LogicalLoc = BSP_LAN9115_REGS_PA;
    //g_bootCfg.kitlDevLoc.IfcType = Internal;
    //g_bootCfg.kitlDevLoc.BusNumber = 0;
    //g_bootCfg.kitlDevLoc.LogicalLoc = BSP_LAN9115_REGS_PA;
    //g_bootCfg.kitlFlags |= OAL_KITL_FLAGS_DHCP;//|OAL_KITL_FLAGS_ENABLED;
    //g_bootCfg.kitlFlags |= OAL_KITL_FLAGS_EXTNAME;
    //g_bootCfg.kitlFlags |= OAL_KITL_FLAGS_VMINI;
    //g_bootCfg.ipAddress = 0;
    //g_bootCfg.ipMask = 0;
    //g_bootCfg.ipRoute = 0;
    //g_bootCfg.deviceID = 0;
    //g_bootCfg.osPartitionSize = IMAGE_WINCE_CODE_SIZE;
    wcscpy(g_bootCfg.filename, L"ebootsd.nb0");

    XLDRMSG(TEXT("open ebootsd.nb0 file\r\n"));

    if (BL_ERROR == BLSDCardDownload(g_bootCfg.filename))
    {
        XLDRMSG(TEXT("SD boot failed to open file\r\n"));
        goto CleanUp;
    }

    //XLDRMSG(TEXT("read ebootsd.nb0 file\r\n"));

    if (BL_ERROR == BLSDCardReadData(0x000c0000, (UCHAR *) IMAGE_STARTUP_IMAGE_PA))
    {
        XLDRMSG(TEXT("SD boot failed to read file\r\n"));
        goto CleanUp;
    }

    XLDRMSG(TEXT("jumping to ebootsd image\r\n"));

    // Wait for serial port
    XLDRMSGDEINIT;

    // Jump to image
    JumpTo((VOID*)IMAGE_STARTUP_IMAGE_PA);

CleanUp:
    XLDRMSG(L"\r\nHALT\r\n");
    for(;;);
}


//------------------------------------------------------------------------------
//
//  Function:  OALPAtoVA
//
VOID* OALPAtoVA(UINT32 address, BOOL cached)
{
    UNREFERENCED_PARAMETER(cached);
    return (VOID*)address;
}


//------------------------------------------------------------------------------
//
//  Function:  OALVAtoPA
//
UINT32 OALVAtoPA(VOID *pVA)
{
    return (UINT32)pVA;
}


//------------------------------------------------------------------------------
//
//  Function:  SetupCopySection
//
//  Copies image's copy section data (initialized globals) to the correct
//  fix-up location.  Once completed, initialized globals are valid. Optimized
//  memcpy is too big for X-Loader.
//
static BOOL SetupCopySection(ROMHDR *const pTableOfContents)
{
    BOOL rc = FALSE;
    UINT32 loop, count;
    COPYentry *pCopyEntry;
    UINT8 *pSrc, *pDst;

    if (pTableOfContents == (ROMHDR *const) -1) goto CleanUp;

    for (loop = 0; loop < pTableOfContents->ulCopyEntries; loop++)
        {
        pCopyEntry = (COPYentry *)(pTableOfContents->ulCopyOffset + loop*sizeof(COPYentry));

        count = pCopyEntry->ulCopyLen;
        pDst = (UINT8*)pCopyEntry->ulDest;
        pSrc = (UINT8*)pCopyEntry->ulSource;
        while (count-- > 0) *pDst++ = *pSrc++;
        count = pCopyEntry->ulDestLen - pCopyEntry->ulCopyLen;
        while (count-- > 0) *pDst++ = 0;
        }

    rc = TRUE;

CleanUp:
    return rc;
}

/*****************************************************************************
*
*
*   @func   void    |   OutputNumHex | Print the hex representation of a number through the monitor port.
*
*   @rdesc  none
*
*   @parm   unsigned long |   n |
*               The number to print.
*
*   @parm   long | depth |
*               Minimum number of digits to print.
*
*/

static void OutputNumHex(unsigned long n, long depth)
{
    if (depth)
    {
        depth--;
    }

    if ((n & ~0xf) || depth)
    {
        OutputNumHex(n >> 4, depth);
        n &= 0xf;
    }

    if (n < 10)
    {
        OEMWriteDebugByte((BYTE)(n + '0'));
    }
    else
    {
        OEMWriteDebugByte((BYTE)(n - 10 + 'A'));
    }
}


/*****************************************************************************
*
*
*   @func   void    |   OutputNumDecimal | Print the decimal representation of a number through the monitor port.
*
*   @rdesc  none
*
*   @parm   unsigned long |   n |
*               The number to print.
*
*/
static void OutputNumDecimal(unsigned long n)
{
    if (n >= 10)
    {
        OutputNumDecimal(n / 10);
        n %= 10;
    }
    OEMWriteDebugByte((BYTE)(n + '0'));
}

#ifndef SHIP_BUILD
//------------------------------------------------------------------------------
//
//  Function:  NKvDbgPrintfW
//
//
VOID NKvDbgPrintfW(LPCWSTR pszFormat, va_list vl)
{
    // Stubbed out to shrink XLDR binary size
    TCHAR c;

    while (*pszFormat)
    {
        c = *pszFormat++;
        switch ((BYTE)c)
        {
            case '%':
                c = *pszFormat++;
                switch (c)
                {
                    case 'x':
                        OutputNumHex(va_arg(vl, unsigned long), 0);
                        break;

                    case 'B':
                        OutputNumHex(va_arg(vl, unsigned long), 2);
                        break;

                    case 'H':
                        OutputNumHex(va_arg(vl, unsigned long), 4);
                        break;

                    case 'X':
                        OutputNumHex(va_arg(vl, unsigned long), 8);
                        break;

                    case 'd':
                    {
                        long    l;

                        l = va_arg(vl, long);
                        if (l < 0)
                        {
                            OEMWriteDebugByte('-');
                            l = - l;
                        }
                        OutputNumDecimal((unsigned long)l);
                        break;
                    }

                    case 'u':
                        OutputNumDecimal(va_arg(vl, unsigned long));
                        break;

                    case 's':
                        OEMWriteDebugString(va_arg(vl, LPWSTR));
                        break;

                    case '%':
                        OEMWriteDebugByte('%');
                        break;

                    case 'c':
                        c = va_arg(vl, TCHAR);
                        OEMWriteDebugByte((BYTE)c);
                        break;

                    default:
                        OEMWriteDebugByte(' ');
                        break;
                }
                break;

            case '\n':
                OEMWriteDebugByte('\r');
                // fall through

            default:
                OEMWriteDebugByte((BYTE)c);
        }
    }
}

//------------------------------------------------------------------------------
//
//  Function:  NKDbgPrintfW
//
//
VOID NKDbgPrintfW(LPCWSTR pszFormat, ...)
{
    // Can be stubbed out to shrink XLDR binary size...
    va_list vl;

    va_start(vl, pszFormat);
    NKvDbgPrintfW(pszFormat, vl);
    va_end(vl);
}
#endif
//------------------------------------------------------------------------------
