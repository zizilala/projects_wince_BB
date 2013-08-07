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

#include "bsp.h"
#include "oalex.h"
#include "ceddkex.h"

#include "oal_alloc.h"
#include "oal_i2c.h"
#include "bsp_cfg.h"
#include "oal_padcfg.h"
#include "bsp_padcfg.h"
#include "oal_clock.h"
#include "oal_gptimer.h"

#include "s35390_rtc.h"
#include "omap_cpuver.h"

#ifdef TEST_TPS65023
#include "triton.h"
#include "tps65023.h"
#include "sdk_i2c.h"
#endif
//------------------------------------------------------------------------------
//  External functions

extern DWORD GetCp15ControlRegister(void);
extern DWORD GetCp15AuxiliaryControlRegister(void);
extern DWORD OALGetL2Aux(void);
extern void EnableUnalignedAccess(void);

extern LPCWSTR g_oalIoCtlPlatformType;
extern LPCWSTR g_oalIoCtlPlatformName;
extern LPCWSTR g_oalIoCtlProcessorName;

//------------------------------------------------------------------------------
//  Global FixUp variables
//
//
const volatile DWORD dwOEMDrWatsonSize     = 0x0004B000; // 300K, for error reporting dump
const volatile DWORD dwOEMHighSecurity     = OEM_HIGH_SECURITY_GP;

//Those variable are used to tell whether the system should claime the BANK1 and CMEM Dsp RAM area for itself
// They are updating during the image generation (not at compilation time).
const volatile DWORD dwBank1Enabled     = (DWORD)-1;
const volatile DWORD dwCMemDSPEnabled   = (DWORD)-1;
const volatile DWORD dwDSP720pEnabled = (DWORD)-1;


//------------------------------------------------------------------------------
//  Global variables

//-----------------------------------------------------------------------------
//
//  Global:  g_CpuFamily
//
//  Set during OEMInit to indicate CPU family.
//
DWORD g_dwCpuFamily;

//-----------------------------------------------------------------------------
//
//  Global:  g_CpuFamily
//
//  Set during OEMInit to indicate CPU family.
//

DWORD g_dwCpuRevision = (DWORD)CPU_REVISION_UNKNOWN;

//------------------------------------------------------------------------------
//
//  Global:  dwOEMSRAMStartOffset
//
//  offset to start of SRAM where SRAM routines will be copied to.
//  Reinitialized in config.bib (FIXUPVAR)
//
DWORD dwOEMSRAMStartOffset = 0x00008000;

#if 0
//------------------------------------------------------------------------------
//
//  Global:  dwOEMVModeSetupTime
//
//  Setup time for DVS transitions. Reinitialized in config.bib (FIXUPVAR)
//
DWORD dwOEMVModeSetupTime = 2;
#endif
//------------------------------------------------------------------------------
//
//  Global:  dwOEMPRCMCLKSSetupTime
//
//  Timethe PRCM waits for system clock stabilization.
//  Reinitialized in config.bib (FIXUPVAR)
//
const volatile DWORD dwOEMPRCMCLKSSetupTime = 0x140;//0x2;

//------------------------------------------------------------------------------
//
//  Global:  dwOEMMPUContextRestore
//
//  location to store context restore information from off mode (PA)
//
const volatile DWORD dwOEMMPUContextRestore = CPU_INFO_ADDR_PA;

//------------------------------------------------------------------------------
//
//  Global:  dwOEMMaxIdlePeriod
//
//  maximum idle period during OS Idle in milliseconds
//
DWORD dwOEMMaxIdlePeriod = 1000;

//------------------------------------------------------------------------------

//extern DWORD gdwFailPowerPaging;
//extern DWORD cbNKPagingPoolSize;

//------------------------------------------------------------------------------
//
//  Global:  g_oalKitlEnabled
//
//  Save kitl state return by KITL intialization
//
//
DWORD g_oalKitlEnabled;

//-----------------------------------------------------------------------------
//
//  Global:  g_oalRetailMsgEnable
//
//  Used to enable retail messages
//
BOOL   g_oalRetailMsgEnable = FALSE;

//-----------------------------------------------------------------------------
//
//  Global:  g_ResumeRTC
//
//  Used to inform RTC code that a resume occured
//
BOOL g_ResumeRTC = FALSE;

//-----------------------------------------------------------------------------
//
//  Global:  g_dwMeasuredSysClkFreq
//
//  The measured SysClk frequency
//
extern DWORD g_dwMeasuredSysClkFreq;

//-----------------------------------------------------------------------------
//
//  Global:  g_pTimerRegs
//
//  32K timer register
//
extern OMAP_GPTIMER_REGS* g_pTimerRegs;
//-----------------------------------------------------------------------------
//
//  Global:  gDevice_prefix
//
//  BSP_prefix
//

CHAR  *gDevice_prefix;

extern  UINT32 g_oalIoCtlClockSpeed;

//------------------------------------------------------------------------------
//  Local functions
//
static void OALGPIOSetDefaultValues();
static void OALCalibrateSysClk();
static DWORD OEMEnumExtensionDRAM(PMEMORY_SECTION pMemSections,DWORD cMemSections);


//------------------------------------------------------------------------------
//
//  Function:  OEMInit
//
//  This is Windows CE OAL initialization function. It is called from kernel
//  after basic initialization is made.
//
VOID OEMInit( )
{    
    BOOL	*pColdBoot;
    BOOL	*pRetailMsgEnable;
    UINT32	CpuRevision;	
    static const PAD_INFO gpioPads_37xx[] = {GPIO_PADS_37XX END_OF_PAD_ARRAY};	
    static UCHAR allocationPool[2048];

    //----------------------------------------------------------------------
    // Initialize OAL log zones
    //----------------------------------------------------------------------

	OALLogSetZones( //(0xFFFF & ~((1<<OAL_LOG_CACHE)|(1<<OAL_LOG_INTR)))|
			(1<<OAL_LOG_VERBOSE)  |
			(1<<OAL_LOG_INFO)     |
			(1<<OAL_LOG_ERROR)    |
			(1<<OAL_LOG_WARN)     |
			(1<<OAL_LOG_IOCTL)    |
     //		(1<<OAL_LOG_FUNC)     |
			(1<<OAL_LOG_INTR)         
			);
    OALMSG(OAL_FUNC, (L"+OEMInit\r\n"));

    //----------------------------------------------------------------------
    // Initialize the OAL memory allocation system (TI code)
    //----------------------------------------------------------------------
    OALLocalAllocInit(allocationPool,sizeof(allocationPool));

    //----------------------------------------------------------------------
    // Determion CPU revison
    //----------------------------------------------------------------------
    CpuRevision = Get_CPUVersion(); // COMMON\SRC\SOC\COMMON_TI_V1\OMAP3530\OAL\CPUVER\oalcpuver.c
    g_dwCpuRevision = CpuRevision & CPU_REVISION_MASK;
    g_dwCpuFamily = (CpuRevision >> CPU_FAMILY_SHIFT) & CPU_REVISION_MASK;
    /* save CPU family names */
    if(g_dwCpuFamily == CPU_FAMILY_DM37XX) // entry
    {
        g_oalIoCtlPlatformType = L"EVM3730";
        g_oalIoCtlPlatformName = L"EVM3730";
        g_oalIoCtlProcessorName   = L"DM3730";	  
        gDevice_prefix = "EVM3730-";
    }
    else if (g_dwCpuFamily == CPU_FAMILY_OMAP35XX) // xx
    {
        g_oalIoCtlPlatformType = L"EVM3530";
        g_oalIoCtlPlatformName = L"EVM3530";		
        g_oalIoCtlProcessorName   = L"OMAP3530";	
        gDevice_prefix = "EVM3530-";
    }
    else
    {
        OALMSG(OAL_ERROR, (L"OEMInit: unknow CPU Family %d\r\n", g_dwCpuFamily));
        g_oalIoCtlPlatformType = L"EVM";
        g_oalIoCtlPlatformName = L"EVM";		
        g_oalIoCtlProcessorName   = L"OMAP3";	
        gDevice_prefix = "EVM-";
		
    }
    //OALMSG(OAL_ERROR, (L"OAL: CPU revision 0x%x:%s\r\n", g_dwCpuRevision, g_oalIoCtlProcessorName));
	OALMSG(1, (L"OAL: CPU = %x\r\n", (CpuRevision >> CHIP_ID_SHIFT) & CHIP_ID_MASK));
    OALMSG(1, (L"OAL: CPU L2 Aux register 0x%x\r\n", OALGetL2Aux()));
    //----------------------------------------------------------------------
    // Update platform specific variables
    //----------------------------------------------------------------------

    //----------------------------------------------------------------------
    // Update kernel variables
    //----------------------------------------------------------------------
    dwNKDrWatsonSize = dwOEMDrWatsonSize;

    // Alarm has resolution 10 seconds (actually has 1 second resolution, 
	// but setting alarm too close to suspend will cause problems).
    dwNKAlarmResolutionMSec = 10000;

    // Set extension functions
    pOEMIsProcessorFeaturePresent = OALIsProcessorFeaturePresent;
    pfnOEMSetMemoryAttributes     = OALSetMemoryAttributes;
    g_pOemGlobal->pfnEnumExtensionDRAM = OEMEnumExtensionDRAM;
    
    //----------------------------------------------------------------------
    // Windows Mobile backward compatibility issue...
    //----------------------------------------------------------------------
/*
    switch (dwOEMTargetProject)
	{
        case OEM_TARGET_PROJECT_SMARTFON:
        case OEM_TARGET_PROJECT_WPC:
            CEProcessorType = PROCESSOR_STRONGARM;
            break;
	}
*/
    //----------------------------------------------------------------------
    // Initialize cache globals
    //----------------------------------------------------------------------
	OALMSG(OAL_FUNC, (L"OALCacheGlobalsInit()\r\n"));
    OALCacheGlobalsInit();
	OALMSG(OAL_FUNC, (L"EnableUnalignedAccess()\r\n"));
    EnableUnalignedAccess();

    #ifdef DEBUG
        OALMSG(1, (L"CPU CP15 Control Register = 0x%x\r\n", GetCp15ControlRegister()));
        OALMSG(1, (L"CPU CP15 Auxiliary Control Register = 0x%x\r\n", GetCp15AuxiliaryControlRegister()));
    #endif
    //----------------------------------------------------------------------
    // Initialize PAD cfg
    //----------------------------------------------------------------------
    OALMSG(OAL_FUNC, (L"OALPadCfgInit..()\r\n"));
    OALPadCfgInit();
	
    //----------------------------------------------------------------------
    // configure pin mux
    //----------------------------------------------------------------------
    OALMSG(OAL_FUNC, (L"ConfigurePadArray()\r\n"));
    ConfigurePadArray(BSPGetAllPadsInfo());
    
    //----------------------------------------------------------------------
    // Initialize Power Domains
    //----------------------------------------------------------------------
    OALPowerInit();

    //----------------------------------------------------------------------
    // Initialize Vector Floating Point co-processor
    //----------------------------------------------------------------------
	OALMSG(OAL_FUNC, (L"OALVFPInitialize()\r\n"));
    OALVFPInitialize(g_pOemGlobal);

    //----------------------------------------------------------------------
    // Initialize interrupt
    //----------------------------------------------------------------------
    if (!OALIntrInit())
        {
        OALMSG(OAL_ERROR, (
            L"ERROR: OEMInit: failed to initialize interrupts\r\n"
            ));
        goto cleanUp;
        }

    //----------------------------------------------------------------------
    // Initialize system clock
    //----------------------------------------------------------------------
	OALMSG(OAL_FUNC, (L"OALTimerInit()\r\n"));
    if (!OALTimerInit(1, 0, 0))
        {
        OALMSG(OAL_ERROR, (
            L"ERROR: OEMInit: Failed to initialize system clock\r\n"
            ));
        goto cleanUp;
        }
	OALMSG(OAL_FUNC, (L"ConfigurePadArray()\r\n"));
    // Configure the pads for the DSS (to keep the splashscreen active)
    // do not request it, it may make the DSS driver fail to load (because it will not ba able to request its pads)
    ConfigurePadArray(BSPGetDevicePadInfo(OMAP_DEVICE_DSS)); // DSS_PADS_37XX
    //same thing for the UART3 (used for our OAL serial output
    ConfigurePadArray(BSPGetDevicePadInfo(OMAP_DEVICE_UART1)); // barcode
    ConfigurePadArray(BSPGetDevicePadInfo(OMAP_DEVICE_HSOTGUSB));

    //all other pads are to be requested (GPMC is never reserved by drivers, I2C is handled by the kernel)
    // GPIOs reservation may be split on per-GPIO basis and moved into the drivers that needs the GPIO. TBD
	// if (!RequestDevicePads(OMAP_DEVICE_GPMC)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for GPMC\r\n")));
    if (!RequestDevicePads(OMAP_DEVICE_I2C1)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for I2C1\r\n")));
    if (!RequestDevicePads(OMAP_DEVICE_I2C2)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for I2C2\r\n")));
    if (!RequestDevicePads(OMAP_DEVICE_I2C3)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for I2C3\r\n")));
    if(g_dwCpuFamily == CPU_FAMILY_DM37XX)
    {
        if (!RequestAndConfigurePadArray(gpioPads_37xx)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for the GPIOs\r\n")));
    }
    else if(g_dwCpuFamily == CPU_FAMILY_OMAP35XX)
    {
        //if (!RequestAndConfigurePadArray(gpioPads)) OALMSG(OAL_ERROR, (TEXT("Failed to request pads for the GPIOs\r\n")));
    }
    else
    {
        OALMSG(OAL_ERROR, (
            L"ERROR: OEMInit: CPU family %d is not supported\r\n", g_dwCpuFamily));
    }

    GPIOInit();

    //----------------------------------------------------------------------
    // Set GPIOs default values (like the buffers' OE)
    //----------------------------------------------------------------------
    OALGPIOSetDefaultValues();

    //----------------------------------------------------------------------
    // Initialize SRAM Functions
    //----------------------------------------------------------------------
    OALSRAMFnInit();
    
    //----------------------------------------------------------------------
    // kSYS_CLK calibration
    // Now compute the real kSYS_CLK clock value. 
    //----------------------------------------------------------------------
    OALCalibrateSysClk();

    //----------------------------------------------------------------------
    // Initialize high performance counter and profiling function pointers
    //----------------------------------------------------------------------
    OALPerformanceTimerInit();
    
#ifdef TEST_TPS65023	//xx
    { // Temporary : test of the TPS65023 interface
        HANDLE hTwl;
        HANDLE hI2CADC;
        UINT16 mvMeasured=0;
        volatile int debug=1;
        UINT32 mv,mv1,mv4,mv5;
        hI2CADC = I2COpen(OMAP_DEVICE_I2C1);
        I2CSetSlaveAddress(hI2CADC,0x41);
        I2CSetSubAddressMode(hI2CADC,I2C_SUBADDRESS_MODE_0);
        hTwl = TWLOpen();
        RETAILMSG(1,(TEXT("triton ID 0x%x \r\n"),TWLReadIDCode(hTwl)));
        TWLGetVoltage(VDCDC1,&mv1);
        TWLGetVoltage(VLDO1,&mv4);
        TWLGetVoltage(VLDO2,&mv5);
        RETAILMSG(1,(TEXT("vdcdc1 %d \r\n"),mv1));
        RETAILMSG(1,(TEXT("ldo1 %d \r\n"),mv4));
        RETAILMSG(1,(TEXT("ldo2 %d \r\n"),mv5));

        while(debug)
        {
            for (mv=1200;mv<1400;mv+=25)
            {
                TWLSetVoltage(VDCDC1,mv);
                TWLGetVoltage(VDCDC1,&mv1);
                //I2CRead(hI2CADC,0,&mvMeasured,2);// Not working because some resistor are not placed
                RETAILMSG(1,(TEXT("vdcdc1 %d 0x%x \r\n"),mv1,mvMeasured));
                OALStall(1000);
            }
            for (mv=1400;mv>=1200;mv-=25)
            {
                TWLSetVoltage(VDCDC1,mv);
                TWLGetVoltage(VDCDC1,&mv1);
                //I2CRead(hI2CADC,0,&mvMeasured,2);// Not working because some resistor are not placed
                RETAILMSG(1,(TEXT("vdcdc1 %d 0x%x \r\n"),mv1,mvMeasured));
                OALStall(1000);
            }
        }
    }
#endif

    //----------------------------------------------------------------------
    // Initialize KITL
    //----------------------------------------------------------------------
    g_oalKitlEnabled = KITLIoctl(IOCTL_KITL_STARTUP, NULL, 0, NULL, 0, NULL);

    //----------------------------------------------------------------------
    // Initialize the watchdog
    //----------------------------------------------------------------------
#ifdef BSP_OMAP_WATCHDOG // xx
    OALWatchdogInit(BSP_WATCHDOG_PERIOD_MILLISECONDS,BSP_WATCHDOG_THREAD_PRIORITY);
#endif

    //----------------------------------------------------------------------
    // Check for retail messages enabled
    //----------------------------------------------------------------------
    pRetailMsgEnable = OALArgsQuery(OAL_ARGS_QUERY_OALFLAGS);
    if (pRetailMsgEnable && (*pRetailMsgEnable & OAL_ARGS_OALFLAGS_RETAILMSG_ENABLE))
        g_oalRetailMsgEnable = TRUE;

    //----------------------------------------------------------------------
    // Deinitialize serial debug
    //----------------------------------------------------------------------
    //?? if (!g_oalRetailMsgEnable)
    //??    OEMDeinitDebugSerial();

// not available under CE6
#if (_WINCEOSVER >= 700)
    //----------------------------------------------------------------------
    // Make Page Tables walk L2 cacheable. There are 2 new fields in OEMGLOBAL
    // that we need to update:
    // dwTTBRCacheBits - the bits to set for TTBR to change page table walk
    //                   to be L2 cacheable. (Cortex-A8 TRM, section 3.2.31)
    //                   Set this to be "Outer Write-Back, Write-Allocate".
    // dwPageTableCacheBits - bits to indicate cacheability to access Level
    //                   L2 page table. We need to set it to "inner no cache,
    //                   outer write-back, write-allocate. i.e.
    //                      TEX = 0b101, and C=B=0.
    //                   (ARM1176 TRM, section 6.11.2, figure 6.7, small (4k) page)
    //----------------------------------------------------------------------
    g_pOemGlobal->dwTTBRCacheBits = 0x8;            // TTBR RGN set to 0b01 - outer write back, write-allocate
    g_pOemGlobal->dwPageTableCacheBits = 0x140;     // Page table cacheability uses 1BB/AA format, where AA = 0b00 (inner non-cached)
#endif

    g_oalIoCtlClockSpeed = Get_CPUMaxSpeed(g_dwCpuFamily); //get MPU clock rate

    //----------------------------------------------------------------------
    // Check for a clean boot of device
    //----------------------------------------------------------------------
    pColdBoot = OALArgsQuery(OAL_ARGS_QUERY_COLDBOOT);
    if ((pColdBoot == NULL)|| ((pColdBoot != NULL) && *pColdBoot))
        NKForceCleanBoot();
    
cleanUp:
    OALMSG(OAL_FUNC, (L"-OEMInit\r\n"));
}

//------------------------------------------------------------------------------

void
OALCalibrateSysClk()
{
    DWORD dw32k_prev,dw32k, dw32k_diff;
    DWORD dwSysk_prev,dwSysk, dwSys_diff;
    DWORD dwOld;
    OMAP_DEVICE gptPerfDevice = BSPGetGPTPerfDevice();
    OMAP_GPTIMER_REGS   *pPerfTimer = OALPAtoUA(GetAddressByDevice(gptPerfDevice));
    EnableDeviceClocks(gptPerfDevice, TRUE);

	OALMSG(OAL_FUNC, (L"OALCalibrateSysClk()\r\n"));
    // configure performance timer
    //---------------------------------------------------
    // Soft reset GPTIMER and wait until finished
    SETREG32(&pPerfTimer->TIOCP, SYSCONFIG_SOFTRESET);
    while ((INREG32(&pPerfTimer->TISTAT) & GPTIMER_TISTAT_RESETDONE) == 0);
 
    // Enable smart idle and autoidle
    // Set clock activity - FCLK can be  switched off, 
    // L4 interface clock is maintained during wkup.
    OUTREG32(&pPerfTimer->TIOCP, 
        0x200 | SYSCONFIG_SMARTIDLE|SYSCONFIG_ENAWAKEUP|SYSCONFIG_AUTOIDLE);
        
    // clear interrupts
    OUTREG32(&pPerfTimer->TISR, 0x00000000);

    //  Start the timer.  Also set for auto reload
    SETREG32(&pPerfTimer->TCLR, GPTIMER_TCLR_ST);
    while ((INREG32(&pPerfTimer->TWPS) & GPTIMER_TWPS_TCLR) != 0);
    
#if SHOW_SYS_CLOCK_VARIATION
    int i;
    for (i=0; i<100;i++)
    {
#endif

    dwOld = OALTimerGetReg(&g_pTimerRegs->TCRR);
    do 
    {
        dwSysk_prev = INREG32(&pPerfTimer->TCRR); 
        dw32k_prev = OALTimerGetReg(&g_pTimerRegs->TCRR);
    } while (dw32k_prev == dwOld);

    OALStall(100000);

    dwOld = OALTimerGetReg(&g_pTimerRegs->TCRR);
    do
    {
        dwSysk = INREG32(&pPerfTimer->TCRR);
        dw32k = OALTimerGetReg(&g_pTimerRegs->TCRR);
    } while (dw32k == dwOld);

    dw32k_diff = dw32k - dw32k_prev;
    dwSys_diff = dwSysk - dwSysk_prev;
    
    g_dwMeasuredSysClkFreq =  (DWORD) (((INT64)dwSys_diff * 32768) / ((INT64)dw32k_diff)) ;

    OALMSG(OAL_FUNC,(L"SysClock calibrate Frequency = %d\r\n", g_dwMeasuredSysClkFreq)); // 26MHz

#if SHOW_SYS_CLOCK_VARIATION
    }
#endif

    EnableDeviceClocks(gptPerfDevice, FALSE);

}

//------------------------------------------------------------------------------


DWORD
OALMux_UpdateOnDeviceStateChange(
    UINT devId,
    UINT oldState,
    UINT newState,
    BOOL bPreStateChange
    )
{
    UNREFERENCED_PARAMETER(devId);
    UNREFERENCED_PARAMETER(oldState);
    UNREFERENCED_PARAMETER(newState);
    UNREFERENCED_PARAMETER(bPreStateChange);
    return (DWORD) -1;
}

void
OALMux_InitMuxTable()
{
}

void EnableDebugSerialClock(UINT uart_id)
{  
#define OMAP_PRCM_PER_CM_REGS_PA            0x48005000

    OMAP_CM_REGS* pCmRegs;
    
    pCmRegs = (OMAP_CM_REGS*) (OMAP_PRCM_CORE_CM_REGS_PA);
   
    switch(uart_id)
    	{
        case OMAP_DEVICE_UART1:
                SETREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_UART1);
                SETREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_UART1);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_UART1);
            break;

/*brian        case OMAP_DEVICE_UART2:
                SETREG32(&pCmRegs->CM_FCLKEN1_xxx, CM_CLKEN_UART2);
                SETREG32(&pCmRegs->CM_ICLKEN1_xxx, CM_CLKEN_UART2);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_UART2);
            break;*/

        case OMAP_DEVICE_UART3:
                pCmRegs = (OMAP_CM_REGS*) (OMAP_PRCM_PER_CM_REGS_PA);
                SETREG32(&pCmRegs->CM_FCLKEN_xxx, CM_CLKEN_UART3);
                SETREG32(&pCmRegs->CM_ICLKEN_xxx, CM_CLKEN_UART3);
                while (INREG32(&pCmRegs->CM_IDLEST1_xxx) & CM_IDLEST_ST_UART3);
            break;	
    	}
}

//------------------------------------------------------------------------------
// External Variables
extern DEVICE_IFC_GPIO Omap_Gpio;
extern DEVICE_IFC_GPIO Tps659xx_Gpio;
extern UINT32 g_ffContextSaveMask;

void BSPGpioInit()
{
	OALMSG(OAL_FUNC, (L"BSPGpioInit()\r\n"));
	BSPInsertGpioDevice(0,&Omap_Gpio,NULL);
	BSPInsertGpioDevice(TRITON_GPIO_PINID_START,&Tps659xx_Gpio,NULL);
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
void
HalContextUpdateDirtyRegister(
                              UINT32 ffRegister
                              )
{
    g_ffContextSaveMask |= ffRegister;
}

void OALGPIOSetDefaultValues()
{
    HANDLE hGPIO = GPIOOpen();
//#ifdef BSP_EVM2

	//OALMSG(OAL_FUNC, (L"OALGPIOSetDefaultValues()\r\n"));
	OALMSG(1, (L"OALGPIOSetDefaultValues()\r\n"));
    // make TPS659XX MSECURE pin low
    //GPIOClrBit(hGPIO,TPS659XX_MSECURE_GPIO);            
    //GPIOSetMode(hGPIO, TPS659XX_MSECURE_GPIO,GPIO_DIR_OUTPUT);

    GPIOClrBit(hGPIO,AUDIO_MUTE_GPIO); // SPK_EN GPIO_37 low_active
    GPIOSetMode(hGPIO, AUDIO_MUTE_GPIO,GPIO_DIR_OUTPUT);

	GPIOClrBit(hGPIO,136); // VIBRATOR
	GPIOSetMode(hGPIO, 136,GPIO_DIR_OUTPUT);
	
	//GPIOSetBit(hGPIO,16); // WLAN_EN
	GPIOClrBit(hGPIO,16); // WLAN_EN
    GPIOSetMode(hGPIO, 16,GPIO_DIR_OUTPUT);
    
	//GPIOSetBit(hGPIO,15); // BT_EN
	GPIOClrBit(hGPIO,15); // BT_EN
    GPIOSetMode(hGPIO, 15,GPIO_DIR_OUTPUT);
    
	GPIOClrBit(hGPIO,34); // FM_EN
    GPIOSetMode(hGPIO, 34,GPIO_DIR_OUTPUT);
    
	GPIOClrBit(hGPIO,38); // ENG_PWEN
    GPIOSetMode(hGPIO, 38,GPIO_DIR_OUTPUT);
    
    // turn on nFULL_MODEM_EN (GPIO_55 = 0)
    //GPIOClrBit(hGPIO,nFULL_MODEM_EN_GPIO);            
    //GPIOSetMode(hGPIO, nFULL_MODEM_EN_GPIO,GPIO_DIR_OUTPUT);

    // route USB2 signals to transceiver (GPIO_61 = 0)
    //GPIOClrBit(hGPIO,USB2_ROUTE_SELECT_GPIO);            
    //GPIOSetMode(hGPIO, USB2_ROUTE_SELECT_GPIO,GPIO_DIR_OUTPUT);

    // maintain TVP in reset state (to reduce power consumption)
    //GPIOClrBit(hGPIO, VIDEO_CAPTURE_RESET);
    //GPIOSetMode(hGPIO, VIDEO_CAPTURE_RESET, GPIO_DIR_OUTPUT);

    // enable new EVM2 control signals (T2_GPIO2 = 0)    
    //GPIOClrBit(hGPIO,NEW_EVM2_CTRL_GPIO);            
    //GPIOSetMode(hGPIO, NEW_EVM2_CTRL_GPIO,GPIO_DIR_OUTPUT);
//#endif
    GPIOClose(hGPIO);
}

DWORD OEMEnumExtensionDRAM(PMEMORY_SECTION pMemSections,DWORD cMemSections)
{
    DWORD cSections = 0;
    
    OALMSG(OAL_FUNC, (L"OEMEnumExtensionDRAM()\r\n"));
    // If BANK1 is enabled, give it the OS (V)
    if ((cSections < cMemSections) && (dwBank1Enabled == 1))
    {
        pMemSections[cSections].dwFlags = 0;
        pMemSections[cSections].dwStart = IMAGE_WINCE_RAM_BANK1_CA;
        pMemSections[cSections].dwLen = IMAGE_WINCE_RAM_BANK1_SIZE;
        cSections++;
    }
    // If DSP 720p region is not used for the DSP, give it to the OS (V)
    if ((cSections < cMemSections) && (dwBank1Enabled == 1) && (dwDSP720pEnabled != 1))
    {
        pMemSections[cSections].dwFlags = 0;
        pMemSections[cSections].dwStart = IMAGE_DSP_720P_CA;
        pMemSections[cSections].dwLen = IMAGE_DSP_720P_SIZE;
        cSections++;
    }
    // If CMEM region is not used for the DSP, give it to the OS (V)
    if ((cSections < cMemSections) && (dwCMemDSPEnabled != 1))
    {
        pMemSections[cSections].dwFlags = 0;
        pMemSections[cSections].dwStart = IMAGE_CMEM_CA;
        pMemSections[cSections].dwLen = IMAGE_CMEM_SIZE;
        cSections++;
    }
    // If DSP region is not used for the DSP, give it to the OS (V)
    if ((cSections < cMemSections) && (dwCMemDSPEnabled != 1))
    {
        pMemSections[cSections].dwFlags = 0;
        pMemSections[cSections].dwStart = IMAGE_DSP_CA;
        pMemSections[cSections].dwLen = IMAGE_DSP_SIZE;
        cSections++;
    }    
    
    return cSections;
}
