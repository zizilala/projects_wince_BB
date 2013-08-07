// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

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
//  File: oem_pm.c
//
#include "bsp.h"
#include "oalex.h"
#include <nkintr.h>
//#include <pkfuncs.h>
#include "omap3530_dvfs.h"
#include "oal_prcm.h"
#include "oal_sr.h"
#include "oal_i2c.h"
#include "oal_clock.h"
#include "interrupt_struct.h"
#include "omap_vrfb_regs.h"
#include "oal_gptimer.h"

#include "twl.h"
#include "triton.h"
#include "tps659xx.h"
#include "tps659xx_internals.h"
/*
#include <windows.h>
#include <nkintr.h>
#include <pkfuncs.h>
#include <bsp.h>
#include <bus.h>
#include <i2c.h>
#include <dvfs.h>
#include <oal_prcm.h>
#include <oal_i2c.h>
#include <oal_sr.h>
#include <interrupt_struct.h>*/

extern void OEMEnableDebugOutput(BOOL bEnable);


#define DISPLAY_TIMEOUT     1100    // based on 32khz clk (~30fps)

#define OMAP_GPIO_BANK_TO_RESTORE       6

#define T2_WAKEON_COUNT                 4
#define T2_SLEEPOFF_COUNT               4
#define T2_WARMRESET_COUNT              6

#define T2_A2S_SEQ_START_ADDR           0x2B
#define T2_S2A_SEQ_START_ADDR           0x30
#define T2_WARMRESET_SEQ_START_ADDR     0x38

/*
TWL_PMB_ENTRY _rgS2A12[] = {
    {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VPLL1_RES_ID, TWL_RES_ACTIVE), 0x30
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VDD1_RES_ID, TWL_RES_ACTIVE), 0x04
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VDD2_RES_ID, TWL_RES_ACTIVE), 0x02
    }, {
        TWL_PBM(TWL_PROCESSOR_GRP123, TWL_RES_GRP_ALL, 0, 7, TWL_RES_ACTIVE), 0x2
    }, {
        0, 0
    }        
};

TWL_PMB_ENTRY _rgA2S[] = {
    {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VDD1_RES_ID, TWL_RES_SLEEP), 0x4
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VDD2_RES_ID, TWL_RES_SLEEP), 0x2
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VPLL1_RES_ID, TWL_RES_OFF), 0x3
    }, {
        TWL_PBM(TWL_PROCESSOR_GRP123, TWL_RES_GRP_ALL, 0, 7, TWL_RES_SLEEP), 0x2
    }, {
        0, 0
    }
};

TWL_PMB_ENTRY _rgWarmRst[] = {
    {
        TWL_PSM(TWL_PROCESSOR_GRP_NULL, TWL_TRITON_RESET, TWL_RES_OFF), 0x02
    }, {        
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VDD1_RES_ID, TWL_RES_WRST), 0x0E
    }, {        
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VDD2_RES_ID, TWL_RES_WRST), 0x0E
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_VPLL1_RES_ID, TWL_RES_WRST), 0x60
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP123, TWL_CLKEN_ID, TWL_RES_ACTIVE), 0x02
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP1, TWL_HFCLKOUT_ID, TWL_RES_ACTIVE), 0x02
    }, {
        TWL_PSM(TWL_PROCESSOR_GRP_NULL, TWL_TRITON_RESET, TWL_RES_ACTIVE), 0x02
    }, {
        0, 0
    }
};
*/

//brian
static void ShowT2Reg(void* hTwl, DWORD RegAddr, TCHAR * szRegname)
{
    UINT8 Value = 0;
    TWLReadRegs(hTwl, RegAddr, &Value, sizeof(Value));
    OALLog(L"T2 %s = 0x%02x\r\n", szRegname, Value);
}

#define SHOW_T2_REG(hTwl, reg) { \
    ShowT2Reg(hTwl, reg, TEXT(#reg)); \
}

//-----------------------------------------------------------------------------
// prototypes
//
extern void SmartReflex_Initialize();
extern void SmartReflex_PostInitialize();
extern BOOL SmartReflex_EnableMonitor(UINT channel, BOOL bEnable);
extern void UpdateRetentionVoltages(IOCTL_RETENTION_VOLTAGES *pData);
extern BOOL SetOpp(DWORD *rgDomains, DWORD *rgOpps, DWORD count);
VOID OALContextRestoreInit();
extern void Opp_init();
extern SmartReflex_Functions_t *SmartReflex_funcs;

extern OMAP_GPTIMER_REGS   *g_pPerfTimer;


//-----------------------------------------------------------------------------
// Global : g_pIntr
//  pointer to interrupt structure.
//
extern OMAP_INTR_CONTEXT const    *g_pIntr;
//------------------------------------------------------------------------------
//
//  Global:  dwOEMPRCMCLKSSetupTime
//
//  Timethe PRCM waits for system clock stabilization. 
//  Reinitialized in config.bib (FIXUPVAR)
//
extern const volatile DWORD dwOEMPRCMCLKSSetupTime;

//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmDebugSuspendResume
//
//  Enables suspend/resume debug code in platform\common\src\soc tree
//
BOOL                        g_PrcmDebugSuspendResume = TRUE; // brian test

//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmPrm
//
//  Reference to all PRCM-PRM registers. Initialized in PrcmInit
//
OMAP_PRCM_PRM               g_PrcmPrm;

//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmCm
//
//  Reference to all PRCM-CM registers. Initialized in PrcmInit
//
OMAP_PRCM_CM                g_PrcmCm;

//------------------------------------------------------------------------------
//
//  Volatile :  g_pIdCodeReg
//
//  reference to ID CODE register part of system control general register set
//
volatile UINT32            *g_pIdCodeReg;

//------------------------------------------------------------------------------
//
//  External:  g_pSysCtrlGenReg
//
//  reference to system control general register set
//
extern OMAP_SYSC_GENERAL_REGS      *g_pSysCtrlGenReg;

//------------------------------------------------------------------------------
//
//  Global:  dwOEMSRAMSaveAddr
//
//  location to store Secure SRAM context
//
extern DWORD dwOEMSRAMSaveAddr;

//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmPrm
//
//  Reference to all PRCM-PRM registers. Initialized in PrcmInit
//
OMAP_PRCM_PRM               g_PrcmPrm;

//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmCm
//
//  Reference to all PRCM-CM registers. Initialized in PrcmInit
//
OMAP_PRCM_CM                g_PrcmCm;

//------------------------------------------------------------------------------
//
//  Volatile :  g_pIdCodeReg
//
//  reference to ID CODE register part of system control general register set
//
volatile UINT32            *g_pIdCodeReg;

//-----------------------------------------------------------------------------
//
//  External:  g_pTimerRegs
//
//  References the GPTimer1 registers.  Initialized in OALTimerInit().
//
OMAP_GPTIMER_REGS          *g_pTimerRegs = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  g_pGPMCRegs
//
//  References the gpmc registers.  Initialized in OALPowerInit().
//
OMAP_GPMC_REGS             *g_pGPMCRegs = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  g_pSDRCRegs
//
//  References the sdrc registers.  Initialized in OALPowerInit().
//
OMAP_SDRC_REGS             *g_pSDRCRegs = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  g_pSMSRegs
//
//  References the SMS registers.  Initialized in OALPowerInit().
//
OMAP_SMS_REGS              *g_pSMSRegs = NULL;

//------------------------------------------------------------------------------
//
//  Global:  g_pVRFBRegs
//
//  References the VRFB registers.  Initialized in OALPowerInit().
//
OMAP_VRFB_REGS              *g_pVRFBRegs = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  g_pContextRestore
//
//  Reference to context restore registers. Initialized in OALPowerInit()
//
OMAP_CONTEXT_RESTORE_REGS  *g_pContextRestore = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  g_pSdrcRestore
//
//  Reference to Sdrc restore registers. Initialized in OALPowerInit()
//
OMAP_SDRC_RESTORE_REGS  *g_pSdrcRestore = NULL;

//-----------------------------------------------------------------------------
//
//  Global:  g_pPrcmRestore
//
//  Reference to Prcm restore registers. Initialized in OALPowerInit()
//
OMAP_PRCM_RESTORE_REGS  *g_pPrcmRestore = NULL;

//------------------------------------------------------------------------------
//
//  Global:  g_ffContextSaveMask
//
//  determines if the context for a particular register set need to be saved
//
UINT32                      g_ffContextSaveMask = 0;

//-----------------------------------------------------------------------------
//
//  Static:  s_pT2SleepOffSeqWords
//
//  Contains T2 sequence to be programed for Active to Sleep state change
//
static TRITON_PMB_MESSAGE s_pT2SleepOffSeqWords[T2_SLEEPOFF_COUNT];

//-----------------------------------------------------------------------------
//
//  Static:  s_pT2WakeOnSeqWords
//
//  Contains T2 sequence to be programed for Sleep to Active state change
//
static TRITON_PMB_MESSAGE s_pT2WakeOnSeqWords[T2_WAKEON_COUNT];

//-----------------------------------------------------------------------------
//
//  Static:  s_pT2WarmResetSeqWords
//
//  Contains T2 sequence to be programed for Warm Reset
//
static TRITON_PMB_MESSAGE s_pT2WarmResetSeqWords[T2_WARMRESET_COUNT];

//-----------------------------------------------------------------------------
//
//  static: s_rgGpioRegsAddr
//
//  GPIO register address
//
static  OMAP_GPIO_REGS *s_rgGpioRegsAddr[OMAP_GPIO_BANK_TO_RESTORE]; //We have 6 GPIO banks

//-----------------------------------------------------------------------------
//
//  static: s_rgGpioContext
//
//  To save GPIO context
//
static  OMAP_GPIO_REGS s_rgGpioContext[OMAP_GPIO_BANK_TO_RESTORE];   //We have 6 GPIO banks

//-----------------------------------------------------------------------------
//
//  static:
//  PRCM related variables to store the PRCM context.
//
//
static OMAP_PRCM_WKUP_PRM_REGS              s_wkupPrmContext;
static OMAP_PRCM_CORE_PRM_REGS              s_corePrmContext;
static OMAP_PRCM_MPU_PRM_REGS               s_mpuPrmContext;
static OMAP_PRCM_CLOCK_CONTROL_PRM_REGS     s_clkCtrlPrmContext;

static OMAP_PRCM_OCP_SYSTEM_CM_REGS         s_ocpSysCmContext;
static OMAP_PRCM_WKUP_CM_REGS               s_wkupCmContext;
static OMAP_PRCM_CORE_CM_REGS               s_coreCmContext;
static OMAP_PRCM_CLOCK_CONTROL_CM_REGS      s_clkCtrlCmContext;
static OMAP_PRCM_GLOBAL_CM_REGS             s_globalCmContext;
static OMAP_PRCM_MPU_CM_REGS                s_mpuCmContext;
static OMAP_PRCM_EMU_CM_REGS                s_emuCmContext;

//-----------------------------------------------------------------------------
//
//  Global:  s_SMSRegs
//
//  variable to store SMS context.
//
static OMAP_SMS_REGS                        s_smsContext;

//-----------------------------------------------------------------------------
//
//  Global:  s_vrfbRegs
//
//  variable to store SMS context.
//
static OMAP_VRFB_REGS                        s_vrfbContext;

//-----------------------------------------------------------------------------
//
//  Global:  s_perfTimerContext
//
//  variable to store SMS context.
//
static OMAP_GPTIMER_REGS					 s_perfTimerContext;


//-----------------------------------------------------------------------------
//
//  static:  s_sdrcContext
//
//  variable to store SDRC context
//
static OMAP_SDRC_REGS         s_sdrcContext;

//-----------------------------------------------------------------------------
//
//  static:  s_intcContext
//
//  variable to store MPU Interrupt controller context
//
static OMAP_INTC_MPU_REGS        s_intcContext;

//-----------------------------------------------------------------------------
//
//  static:  s_gpmcContext
//
//  variable to store GPMC context
//
static OMAP_GPMC_REGS          s_gpmcContext;

//-----------------------------------------------------------------------------
//
//  static:
//
//  variable to store System control module context
//
static OMAP_SYSC_GENERAL_REGS  s_syscGenContext;
static OMAP_SYSC_INTERFACE_REGS s_syscIntContext;
static OMAP_SYSC_INTERFACE_REGS   *s_pSyscIFContext = NULL;


//-----------------------------------------------------------------------------
//
//  Global :  s_pSyscPadWkupRegs
//
//  Address for SYSC WKUP pad conf registers
//
OMAP_SYSC_PADCONFS_WKUP_REGS               *g_pSyscPadWkupRegs = NULL;

//-----------------------------------------------------------------------------
//
//  Global :  s_pSyscPadWkupRegs
//
//  Address for SYSC WKUP pad conf registers
//
OMAP_SYSC_PADCONFS_REGS                    *g_pSyscPadConfsRegs = NULL;

//-----------------------------------------------------------------------------
//
//  static:
//
//  variable to save DMA context
//
static OMAP_SDMA_REGS                       s_dmaController;
static OMAP_SDMA_REGS                      *s_pDmaController = NULL;

//-----------------------------------------------------------------------------
//
//  static: s_bCoreOffModeSet
//
//  Flag to indicate PER and NEON domains are configured for CORE OFF
//
static BOOL                                 s_bCoreOffModeSet = FALSE;

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveGPIO
//
//  Saves the GPIO Context, clears the IRQ for OFF mode
//
VOID
OALContextSaveGPIO ();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveGPMC
//
//  Stores the GPMC Registers in shadow register
//
VOID
OALContextSaveGPMC ();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveSCM
//
//  Stores the SCM Registers in shadow register
//
VOID
OALContextSaveSCM ();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSavePRCM
//
//  Stores the PRCM Registers in shadow register
//
VOID
OALContextSavePRCM ();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveINTC
//
//  Stores the MPU IC Registers in shadow register
//
VOID
OALContextSaveINTC ();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveDMA
//
//  Saves the DMA Registers
//
VOID
OALContextSaveDMA();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveMux
//
//  Saves pinmux
//
VOID
OALContextSaveMux();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveSMS
//
//  Saves SMS
//
VOID
OALContextSaveSMS();

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveVRFB
//
//  Saves VRFB
//
VOID
OALContextSaveVRFB();

//-----------------------------------------------------------------------------
//
//  Function:  TwlDump - brian
//

VOID TwlDump(HANDLE hTWL)
{
    SHOW_T2_REG(hTWL, TWL_CFG_P1_TRANSITION);
    SHOW_T2_REG(hTWL, TWL_CFG_P2_TRANSITION);
    SHOW_T2_REG(hTWL, TWL_CFG_P3_TRANSITION);
    SHOW_T2_REG(hTWL, TWL_CFG_P123_TRANSITION);
    SHOW_T2_REG(hTWL, TWL_STS_BOOT);
    SHOW_T2_REG(hTWL, TWL_CFG_BOOT);
    SHOW_T2_REG(hTWL, TWL_SHUNDAN);
    SHOW_T2_REG(hTWL, TWL_BOOT_BCI);
    SHOW_T2_REG(hTWL, TWL_CFG_PWRANA1);
    SHOW_T2_REG(hTWL, TWL_CFG_PWRANA2);
    SHOW_T2_REG(hTWL, TWL_BGAP_TRIM);
    SHOW_T2_REG(hTWL, TWL_BACKUP_MISC_STS);
    SHOW_T2_REG(hTWL, TWL_BACKUP_MISC_CFG);
    SHOW_T2_REG(hTWL, TWL_BACKUP_MISC_TST);
    SHOW_T2_REG(hTWL, TWL_PROTECT_KEY);
    SHOW_T2_REG(hTWL, TWL_STS_HW_CONDITIONS);
    SHOW_T2_REG(hTWL, TWL_P1_SW_EVENTS);
    SHOW_T2_REG(hTWL, TWL_P2_SW_EVENTS);
    SHOW_T2_REG(hTWL, TWL_P3_SW_EVENTS);
    SHOW_T2_REG(hTWL, TWL_STS_P123_STATE);
    SHOW_T2_REG(hTWL, TWL_PB_CFG);
    SHOW_T2_REG(hTWL, TWL_PB_WORD_MSB);
    SHOW_T2_REG(hTWL, TWL_PB_WORD_LSB);
    SHOW_T2_REG(hTWL, TWL_RESERVED_A);
    SHOW_T2_REG(hTWL, TWL_RESERVED_B);
    SHOW_T2_REG(hTWL, TWL_RESERVED_C);
    SHOW_T2_REG(hTWL, TWL_RESERVED_D);
    SHOW_T2_REG(hTWL, TWL_RESERVED_E);
    SHOW_T2_REG(hTWL, TWL_SEQ_ADD_W2P);
    SHOW_T2_REG(hTWL, TWL_SEQ_ADD_P2A);
    SHOW_T2_REG(hTWL, TWL_SEQ_ADD_A2W);
    SHOW_T2_REG(hTWL, TWL_SEQ_ADD_A2S);
    SHOW_T2_REG(hTWL, TWL_SEQ_ADD_S2A12);
    SHOW_T2_REG(hTWL, TWL_SEQ_ADD_S2A3);
    SHOW_T2_REG(hTWL, TWL_SEQ_ADD_WARM);
    SHOW_T2_REG(hTWL, TWL_MEMORY_ADDRESS);
    SHOW_T2_REG(hTWL, TWL_MEMORY_DATA);

    // pm receiver (un)secure mode
    SHOW_T2_REG(hTWL, TWL_SC_CONFIG);
    SHOW_T2_REG(hTWL, TWL_SC_DETECT1);
    SHOW_T2_REG(hTWL, TWL_SC_DETECT2);
    SHOW_T2_REG(hTWL, TWL_WATCHDOG_CFG);
    SHOW_T2_REG(hTWL, TWL_IT_CHECK_CFG);
    SHOW_T2_REG(hTWL, TWL_VIBRATOR_CFG);
    SHOW_T2_REG(hTWL, TWL_DCDC_GLOBAL_CFG);
    SHOW_T2_REG(hTWL, TWL_VDD1_TRIM1);
    SHOW_T2_REG(hTWL, TWL_VDD1_TRIM2);
    SHOW_T2_REG(hTWL, TWL_VDD2_TRIM1);
    SHOW_T2_REG(hTWL, TWL_VDD2_TRIM2);
    SHOW_T2_REG(hTWL, TWL_VIO_TRIM1);
    SHOW_T2_REG(hTWL, TWL_VIO_TRIM2);
    SHOW_T2_REG(hTWL, TWL_MISC_CFG);
    SHOW_T2_REG(hTWL, TWL_LS_TST_A);
    SHOW_T2_REG(hTWL, TWL_LS_TST_B);
    SHOW_T2_REG(hTWL, TWL_LS_TST_C);
    SHOW_T2_REG(hTWL, TWL_LS_TST_D);
    SHOW_T2_REG(hTWL, TWL_BB_CFG);
    SHOW_T2_REG(hTWL, TWL_MISC_TST);
    SHOW_T2_REG(hTWL, TWL_TRIM1);
    SHOW_T2_REG(hTWL, TWL_TRIM2);
    SHOW_T2_REG(hTWL, TWL_DCDC_TIMEOUT);
    SHOW_T2_REG(hTWL, TWL_VAUX1_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VAUX1_TYPE);
    SHOW_T2_REG(hTWL, TWL_VAUX1_REMAP);
    SHOW_T2_REG(hTWL, TWL_VAUX1_DEDICATED);
    SHOW_T2_REG(hTWL, TWL_VAUX2_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VAUX2_TYPE);
    SHOW_T2_REG(hTWL, TWL_VAUX2_REMAP);
    SHOW_T2_REG(hTWL, TWL_VAUX2_DEDICATED);
    SHOW_T2_REG(hTWL, TWL_VAUX3_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VAUX3_TYPE);
    SHOW_T2_REG(hTWL, TWL_VAUX3_REMAP);
    SHOW_T2_REG(hTWL, TWL_VAUX3_DEDICATED);
    SHOW_T2_REG(hTWL, TWL_VAUX4_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VAUX4_TYPE);
    SHOW_T2_REG(hTWL, TWL_VAUX4_REMAP);
    SHOW_T2_REG(hTWL, TWL_VAUX4_DEDICATED);
    SHOW_T2_REG(hTWL, TWL_VMMC1_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VMMC1_TYPE);
    SHOW_T2_REG(hTWL, TWL_VMMC1_REMAP);
    SHOW_T2_REG(hTWL, TWL_VMMC1_DEDICATED);
    SHOW_T2_REG(hTWL, TWL_VMMC2_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VMMC2_TYPE);
    SHOW_T2_REG(hTWL, TWL_VMMC2_REMAP);
    SHOW_T2_REG(hTWL, TWL_VMMC2_DEDICATED);
    SHOW_T2_REG(hTWL, TWL_VPLL1_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VPLL1_TYPE);
    SHOW_T2_REG(hTWL, TWL_VPLL1_REMAP);
    SHOW_T2_REG(hTWL, TWL_VPLL1_DEDICATED);
    SHOW_T2_REG(hTWL, TWL_VPLL2_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VPLL2_TYPE);
    SHOW_T2_REG(hTWL, TWL_VPLL2_REMAP);
    SHOW_T2_REG(hTWL, TWL_VPLL2_DEDICATED);
    SHOW_T2_REG(hTWL, TWL_VSIM_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VSIM_TYPE);
    SHOW_T2_REG(hTWL, TWL_VSIM_REMAP);
    SHOW_T2_REG(hTWL, TWL_VSIM_DEDICATED);
    SHOW_T2_REG(hTWL, TWL_VDAC_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VDAC_TYPE);
    SHOW_T2_REG(hTWL, TWL_VDAC_REMAP);
    SHOW_T2_REG(hTWL, TWL_VDAC_DEDICATED);
    SHOW_T2_REG(hTWL, TWL_VINTANA1_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VINTANA1_TYPE);
    SHOW_T2_REG(hTWL, TWL_VINTANA1_REMAP);
    SHOW_T2_REG(hTWL, TWL_VINTANA1_DEDICATED);
    SHOW_T2_REG(hTWL, TWL_VINTANA2_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VINTANA2_TYPE);
    SHOW_T2_REG(hTWL, TWL_VINTANA2_REMAP);
    SHOW_T2_REG(hTWL, TWL_VINTANA2_DEDICATED);
    SHOW_T2_REG(hTWL, TWL_VINTDIG_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VINTDIG_TYPE);
    SHOW_T2_REG(hTWL, TWL_VINTDIG_REMAP);
    SHOW_T2_REG(hTWL, TWL_VINTDIG_DEDICATED);
    SHOW_T2_REG(hTWL, TWL_VIO_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VIO_TYPE);
    SHOW_T2_REG(hTWL, TWL_VIO_REMAP);
    SHOW_T2_REG(hTWL, TWL_VIO_CFG);
    SHOW_T2_REG(hTWL, TWL_VIO_MISC_CFG);
    SHOW_T2_REG(hTWL, TWL_VIO_TEST1);
    SHOW_T2_REG(hTWL, TWL_VIO_TEST2);
    SHOW_T2_REG(hTWL, TWL_VIO_OSC);
    SHOW_T2_REG(hTWL, TWL_VIO_RESERVED);
    SHOW_T2_REG(hTWL, TWL_VIO_VSEL);
    SHOW_T2_REG(hTWL, TWL_VDD1_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VDD1_TYPE);
    SHOW_T2_REG(hTWL, TWL_VDD1_REMAP);
    SHOW_T2_REG(hTWL, TWL_VDD1_CFG);
    SHOW_T2_REG(hTWL, TWL_VDD1_MISC_CFG);
    SHOW_T2_REG(hTWL, TWL_VDD1_TEST1);
    SHOW_T2_REG(hTWL, TWL_VDD1_TEST2);
    SHOW_T2_REG(hTWL, TWL_VDD1_OSC);
    SHOW_T2_REG(hTWL, TWL_VDD1_RESERVED);
    SHOW_T2_REG(hTWL, TWL_VDD1_VSEL);
    SHOW_T2_REG(hTWL, TWL_VDD1_VMODE_CFG);
    SHOW_T2_REG(hTWL, TWL_VDD1_VFLOOR);
    SHOW_T2_REG(hTWL, TWL_VDD1_VROOF);
    SHOW_T2_REG(hTWL, TWL_VDD1_STEP);
    SHOW_T2_REG(hTWL, TWL_VDD2_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VDD2_TYPE);
    SHOW_T2_REG(hTWL, TWL_VDD2_REMAP);
    SHOW_T2_REG(hTWL, TWL_VDD2_CFG);
    SHOW_T2_REG(hTWL, TWL_VDD2_MISC_CFG);
    SHOW_T2_REG(hTWL, TWL_VDD2_TEST1);
    SHOW_T2_REG(hTWL, TWL_VDD2_TEST2);
    SHOW_T2_REG(hTWL, TWL_VDD2_OSC);
    SHOW_T2_REG(hTWL, TWL_VDD2_RESERVED);
    SHOW_T2_REG(hTWL, TWL_VDD2_VSEL);
    SHOW_T2_REG(hTWL, TWL_VDD2_VMODE_CFG);
    SHOW_T2_REG(hTWL, TWL_VDD2_VFLOOR);
    SHOW_T2_REG(hTWL, TWL_VDD2_VROOF);
    SHOW_T2_REG(hTWL, TWL_VDD2_STEP);
    SHOW_T2_REG(hTWL, TWL_VUSB1V5_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VUSB1V5_TYPE);
    SHOW_T2_REG(hTWL, TWL_VUSB1V5_REMAP);
    SHOW_T2_REG(hTWL, TWL_VUSB1V8_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VUSB1V8_TYPE);
    SHOW_T2_REG(hTWL, TWL_VUSB1V8_REMAP);
    SHOW_T2_REG(hTWL, TWL_VUSB3V1_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VUSB3V1_TYPE);
    SHOW_T2_REG(hTWL, TWL_VUSB3V1_REMAP);
    SHOW_T2_REG(hTWL, TWL_VUSBCP_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_VUSBCP_TYPE);
    SHOW_T2_REG(hTWL, TWL_VUSBCP_REMAP);
    SHOW_T2_REG(hTWL, TWL_VUSB_DEDICATED1);
    SHOW_T2_REG(hTWL, TWL_VUSB_DEDICATED2);
    SHOW_T2_REG(hTWL, TWL_REGEN_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_REGEN_TYPE);
    SHOW_T2_REG(hTWL, TWL_REGEN_REMAP);
    SHOW_T2_REG(hTWL, TWL_NRESPWRON_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_NRESPWRON_TYPE);
    SHOW_T2_REG(hTWL, TWL_NRESPWRON_REMAP);
    SHOW_T2_REG(hTWL, TWL_CLKEN_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_CLKEN_TYPE);
    SHOW_T2_REG(hTWL, TWL_CLKEN_REMAP);
    SHOW_T2_REG(hTWL, TWL_SYSEN_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_SYSEN_TYPE);
    SHOW_T2_REG(hTWL, TWL_SYSEN_REMAP);
    SHOW_T2_REG(hTWL, TWL_HFCLKOUT_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_HFCLKOUT_TYPE);
    SHOW_T2_REG(hTWL, TWL_HFCLKOUT_REMAP);
    SHOW_T2_REG(hTWL, TWL_32KCLKOUT_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_32KCLKOUT_TYPE);
    SHOW_T2_REG(hTWL, TWL_32KCLKOUT_REMAP);
    SHOW_T2_REG(hTWL, TWL_TRITON_RESET_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_TRITON_RESET_TYPE);
    SHOW_T2_REG(hTWL, TWL_TRITON_RESET_REMAP);
    SHOW_T2_REG(hTWL, TWL_MAINREF_DEV_GRP);
    SHOW_T2_REG(hTWL, TWL_MAINREF_TYPE);
    SHOW_T2_REG(hTWL, TWL_MAINREF_REMAP);
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmDeviceSetAutoIdleState
//
//  Sets the autoidle state of a device.
//
BOOL OALIoCtlPrcmDeviceSetAutoIdleState(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_DEVICE_ENABLE_IN *pInfo;

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pOutBuffer);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmDeviceSetAutoIdleState\r\n"));
    if (pInBuffer == NULL || inSize < sizeof(IOCTL_PRCM_DEVICE_ENABLE_IN))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pInfo = (IOCTL_PRCM_DEVICE_ENABLE_IN*)(pInBuffer);        
    PrcmDeviceEnableAutoIdle(pInfo->devId, pInfo->bEnable);

    rc = TRUE;

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmDeviceSetAutoIdleState(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmDeviceGetDeviceStatus
//
//  returns the current clock and autoidle status of a device
//
BOOL OALIoCtlPrcmDeviceGetDeviceStatus(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_DEVICE_GET_DEVICESTATUS_OUT *pOut;

    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmDeviceGetDeviceStatus\r\n"));
    
    // validate parameters
    //
    if (pInBuffer == NULL || inSize != sizeof(UINT) ||
        pOutBuffer == NULL || outSize != sizeof(IOCTL_PRCM_DEVICE_GET_DEVICESTATUS_OUT))
        {
        goto cleanUp;
        }

    if (pOutSize != NULL) *pOutSize = 0;
    
    // update function pointers
    //
    pOut = (IOCTL_PRCM_DEVICE_GET_DEVICESTATUS_OUT*)pOutBuffer;
    if (PrcmDeviceGetEnabledState(*(UINT*)pInBuffer, &pOut->bEnabled) == FALSE ||
        PrcmDeviceGetAutoIdleState(*(UINT*)pInBuffer, &pOut->bAutoIdle) == FALSE)
        {
        goto cleanUp;
        }

    rc = TRUE;
cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmDeviceGetDeviceStatus(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmDeviceGetSourceClockInfo
//
//  returns information about a devices clock
//
BOOL OALIoCtlPrcmDeviceGetSourceClockInfo(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO_OUT *pOut;

    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmDeviceGetSourceClockInfo\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(UINT) ||
        pOutBuffer == NULL || outSize != sizeof(IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO_OUT))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pOut = (IOCTL_PRCM_DEVICE_GET_SOURCECLOCKINFO_OUT*)(pOutBuffer);        
    rc = PrcmDeviceGetSourceClockInfo(*(UINT*)pInBuffer, pOut);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmDeviceGetSourceClockInfo(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmClockGetSourceClockInfo
//
//  returns information about a source clock
//
BOOL OALIoCtlPrcmClockGetSourceClockInfo(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    )
{
    BOOL rc = FALSE;
    SourceClockInfo_t info;
    IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_IN *pIn;
    IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_OUT *pOut;

    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmClockGetSourceClockInfo\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_IN) ||
        pOutBuffer == NULL || outSize != sizeof(IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_OUT))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_IN*)pInBuffer;
    pOut = (IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_OUT*)pOutBuffer;

    memset(pOut, 0, sizeof(IOCTL_PRCM_CLOCK_GET_SOURCECLOCKINFO_OUT));
    rc = PrcmClockGetParentClockRefcount(pIn->clockId, pIn->clockLevel, &pOut->refCount);
    if (PrcmClockGetParentClockInfo(pIn->clockId, pIn->clockLevel, &info))
        {        
        pOut->parentId = info.clockId;
        pOut->parentLevel = info.nLevel;
        pOut->parentRefCount = info.refCount;
        }

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmClockGetSourceClockInfo(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmClockSetSourceClock
//
//  sets the source clock for a given functional clock
//
BOOL OALIoCtlPrcmClockSetSourceClock(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_CLOCK_SET_SOURCECLOCK_IN *pIn;

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmClockSetSourceClock\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_CLOCK_SET_SOURCECLOCK_IN))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_CLOCK_SET_SOURCECLOCK_IN*)pInBuffer;
    rc = PrcmClockSetParent(pIn->clkId, pIn->newParentClkId);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmClockSetSourceClock(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmClockSetSourceClockDivisor
//
//  sets the source clock divisor for a given functional clock
//
BOOL OALIoCtlPrcmClockSetSourceClockDivisor(
    UINT32 code, 
    VOID  *pInBuffer,
    UINT32 inSize, 
    VOID  *pOutBuffer, 
    UINT32 outSize, 
    UINT32*pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_CLOCK_SET_SOURCECLOCKDIVISOR_IN *pIn;

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmClockSetSourceClockDivisor\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_CLOCK_SET_SOURCECLOCKDIVISOR_IN))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_CLOCK_SET_SOURCECLOCKDIVISOR_IN*)pInBuffer;
    rc = PrcmClockSetDivisor(pIn->clkId, pIn->parentClkId, pIn->divisor);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmClockSetSourceClockDivisor(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmClockSetDpllState
//
//  updates the current dpll settings
//
BOOL 
OALIoCtlPrcmClockSetDpllState(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_CLOCK_SET_DPLLSTATE_IN *pIn;

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmClockSetDpllState\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_CLOCK_SET_DPLLSTATE_IN))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_CLOCK_SET_DPLLSTATE_IN*)pInBuffer;
    rc = PrcmClockSetDpllState(pIn);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmClockSetDpllState(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmDomainSetWakeupDependency
//
//  updates the wake-up dependency for a power domain
//
BOOL 
OALIoCtlPrcmDomainSetWakeupDependency(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_DOMAIN_SET_WAKEUPDEP_IN *pIn;

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmDomainSetWakeupDependency\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_DOMAIN_SET_WAKEUPDEP_IN))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_DOMAIN_SET_WAKEUPDEP_IN*)pInBuffer;
    rc = PrcmDomainSetWakeupDependency(pIn->powerDomain, pIn->ffWakeDep, pIn->bEnable);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmDomainSetWakeupDependency(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmDomainSetSleepDependency
//
//  updates the sleep dependency for a power domain
//
BOOL 
OALIoCtlPrcmDomainSetSleepDependency(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_PRCM_DOMAIN_SET_SLEEPDEP_IN *pIn;
    
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmDomainSetSleepDependency\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_DOMAIN_SET_SLEEPDEP_IN))
        {
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_DOMAIN_SET_SLEEPDEP_IN*)pInBuffer;
    rc = PrcmDomainSetSleepDependency(pIn->powerDomain, pIn->ffSleepDep, pIn->bEnable);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmDomainSetSleepDependency(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmDomainSetPowerState
//
//  updates the power state for a power domain
//
BOOL 
OALIoCtlPrcmDomainSetPowerState(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = TRUE;
    IOCTL_PRCM_DOMAIN_SET_POWERSTATE_IN *pIn;

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmDomainSetPowerState\r\n"));
    
    if (pInBuffer == NULL || inSize != sizeof(IOCTL_PRCM_DOMAIN_SET_POWERSTATE_IN))
        {
        rc = FALSE;
        goto cleanUp;
        }

    // update info and call appropriate routine
    //
    if (pOutSize != NULL) *pOutSize = 0;
    pIn = (IOCTL_PRCM_DOMAIN_SET_POWERSTATE_IN*)pInBuffer;

    switch (pIn->powerDomain)
        {
        case POWERDOMAIN_MPU:
        case POWERDOMAIN_NEON:
        case POWERDOMAIN_PERIPHERAL:
            if (pIn->powerState == POWERSTATE_OFF)
                {
                PrcmDomainSetPowerState(pIn->powerDomain,
                    POWERSTATE_RETENTION,
                    LOGICRETSTATE_LOGICRET_DOMAINRET
                    );

                // need to notify wakeup latency framework
                // what the actual power domain state will be
                OALWakeupLatency_UpdateDomainState(pIn->powerDomain,
                    POWERSTATE_OFF,
                    pIn->logicState
                    );
                }
            else
                {
                PrcmDomainSetPowerState(pIn->powerDomain,
                    pIn->powerState,
                    pIn->logicState
                    );
                }
            break;

        case POWERDOMAIN_CORE:
            if (pIn->powerState == POWERSTATE_OFF)
                {
                PrcmDomainSetPowerState(pIn->powerDomain,
                    POWERSTATE_RETENTION,
                    LOGICRETSTATE_LOGICOFF_DOMAINRET
                    );

                // need to notify wakeup latency framework
                // what the actual power domain state will be
                OALWakeupLatency_UpdateDomainState(pIn->powerDomain,
                    POWERSTATE_OFF,
                    LOGICRETSTATE_LOGICOFF_DOMAINRET
                    );
                }
            else
                {
                PrcmDomainSetPowerState(pIn->powerDomain,
                    pIn->powerState,
                    pIn->logicState
                    );
                }
            break;

        default:
            PrcmDomainSetPowerState(pIn->powerDomain,
                    pIn->powerState,
                    pIn->logicState
                    );
        }

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmDomainSetPowerState(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlPrcmSetSuspendState
//
//  updates the chip state to enter on suspend
//
BOOL
OALIoCtlPrcmSetSuspendState(
    UINT32 code,
    VOID *pInBuffer,
    UINT32 inSize,
    VOID *pOutBuffer,
    UINT32 outSize,
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlPrcmSetSuspendState\r\n"));

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    if (pInBuffer == NULL || inSize != sizeof(DWORD)) goto cleanUp;

    rc = OALWakeupLatency_SetSuspendState(*(DWORD*)pInBuffer);

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlPrcmSetSuspendState(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlOppRequest
//
//  updates the current operating point
//
BOOL 
OALIoCtlOppRequest(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    IOCTL_OPP_REQUEST_IN *pOppRequest;
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlOppRequest\r\n"));

    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    if (pInBuffer == NULL || inSize != sizeof(IOCTL_OPP_REQUEST_IN)) goto cleanUp;
    
    pOppRequest = (IOCTL_OPP_REQUEST_IN*)pInBuffer;
    if (pOppRequest->dwCount > MAX_DVFS_DOMAINS) goto cleanUp;
    
    rc = SetOpp(pOppRequest->rgDomains, 
            pOppRequest->rgOpps, 
            pOppRequest->dwCount
            );

cleanUp:    
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlOppRequest(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlSmartReflexControl
//
//  enable/disable smartreflex monitoring
//
BOOL 
OALIoCtlSmartReflexControl(
    UINT32 code, 
    VOID *pInBuffer,
    UINT32 inSize, 
    VOID *pOutBuffer, 
    UINT32 outSize, 
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlSmartReflexControl\r\n"));
    
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(pOutBuffer);
    UNREFERENCED_PARAMETER(code);

    if (pInBuffer == NULL || inSize < sizeof(BOOL)) goto cleanUp;

    SmartReflex_EnableMonitor(kSmartReflex_Channel1, *(BOOL*)pInBuffer);
    SmartReflex_EnableMonitor(kSmartReflex_Channel2, *(BOOL*)pInBuffer);

    rc = TRUE;

cleanUp:    
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlSmartReflexControl(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlUpdateRetentionVoltages
//
//  changes VDD1 and VDD2 retention voltages
//
BOOL
OALIoCtlUpdateRetentionVoltages(
    UINT32 code,
    VOID *pInBuffer,
    UINT32 inSize,
    VOID *pOutBuffer,
    UINT32 outSize,
    UINT32 *pOutSize
    )
{
    IOCTL_RETENTION_VOLTAGES *pData;
    BOOL rc = FALSE;

    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(outSize);
    UNREFERENCED_PARAMETER(pOutBuffer);

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlUpdateRetentionVoltages\r\n"));

    if (pInBuffer == NULL || inSize < sizeof(IOCTL_RETENTION_VOLTAGES)) goto cleanUp;

    pData = (IOCTL_RETENTION_VOLTAGES *)pInBuffer;
    UpdateRetentionVoltages(pData);

    rc = TRUE;

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC, (L"-OALIoCtlUpdateRetentionVoltages(rc = %d)\r\n", rc));
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalContextSaveGetBuffer
//
//  returns a pointer to the buffer which holds the context save mask
//  this is a *fast path* to reduce the number of Kernel IOCTL's
//  necessary to indicate a context save is requred
//
BOOL
OALIoCtlHalContextSaveGetBuffer(
    UINT32 code,
    VOID *pInBuffer,
    UINT32 inSize,
    VOID *pOutBuffer,
    UINT32 outSize,
    UINT32 *pOutSize
    )
{
    BOOL rc = FALSE;
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoCtlHalContextSaveGetBuffer\r\n"));

    UNREFERENCED_PARAMETER(inSize);
    UNREFERENCED_PARAMETER(pOutSize);
    UNREFERENCED_PARAMETER(pInBuffer);
    UNREFERENCED_PARAMETER(code);


    if (pOutBuffer == NULL || outSize < sizeof(UINT32**)) goto cleanUp;

    *(UINT32**)pOutBuffer = &g_ffContextSaveMask;

    rc = TRUE;

cleanUp:
    OALMSG(OAL_INTR&&OAL_FUNC,
        (L"-OALIoCtlHalContextSaveGetBuffer(rc = %d)\r\n", rc)
        );
    return rc;
}

//-----------------------------------------------------------------------------
//
//  Function:  ForceStandbyUSB
//
//  Force USB into standby mode
//
void 
ForceStandbyUSB()
{
    // ES1.0 only
    //force_standby_usb(void)
    PrcmDeviceEnableClocks(OMAP_DEVICE_OMAPCTRL, TRUE);
    SETREG32(&g_pSysCtrlGenReg->CONTROL_DEVCONF0, (1 << 15));
    PrcmDeviceEnableClocks(OMAP_DEVICE_OMAPCTRL, FALSE);
}

//-----------------------------------------------------------------------------
//
//  Function:  ReleaseIvaReset()
//
//  release IVA reset
//
void 
ReleaseIvaReset()
{
    // ES 1.0 only
    // workaround for errata 1.27
    //
    PrcmDeviceEnableClocks(OMAP_DEVICE_OMAPCTRL, TRUE);
    
    PrcmClockSetDpllAutoIdleState(kDPLL2, DPLL_AUTOIDLE_DISABLED);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_IVA2_BOOTMOD, 1);
    PrcmDomainResetRelease(POWERDOMAIN_IVA2, RST2_IVA2);
    PrcmDomainResetRelease(POWERDOMAIN_IVA2, RST1_IVA2);
    PrcmClockSetDpllAutoIdleState(kDPLL2, DPLL_AUTOIDLE_LOWPOWERSTOPMODE);

    OUTREG32(&g_pSysCtrlGenReg->CONTROL_IVA2_BOOTMOD, 0);
    OUTREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->RM_RSTCTRL_IVA2, 0);
    OUTREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->RM_RSTST_IVA2, INREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->RM_RSTST_IVA2));    

    PrcmDeviceEnableClocks(OMAP_DEVICE_OMAPCTRL, FALSE);
}

//-----------------------------------------------------------------------------
//
//  Function:  ResetDisplay()
//
//  properly resets the display and turns it off
//
void 
ResetDisplay()
{
    unsigned int        val;
    unsigned int        tcrr;
    OMAP_DISPC_REGS    *pDispRegs = OALPAtoUA(OMAP_DISC1_REGS_PA);

    // enable all the interface and functional clocks
    PrcmDeviceEnableClocks(OMAP_DEVICE_DSS, TRUE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_DSS1, TRUE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_DSS2, TRUE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_TVOUT, TRUE);

    // check if digital output or the lcd output are enabled
    val = INREG16(&pDispRegs->DISPC_CONTROL);
    if(val & (DISPC_CONTROL_DIGITALENABLE | DISPC_CONTROL_LCDENABLE))
    {
        // disable the lcd output and digital output
        val &= ~(DISPC_CONTROL_DIGITALENABLE | DISPC_CONTROL_LCDENABLE);
        OUTREG32(&pDispRegs->DISPC_CONTROL, val);

        // wait until frame is done
        tcrr = OALTimerGetReg(&g_pTimerRegs->TCRR);
        OUTREG32(&pDispRegs->DISPC_IRQSTATUS, DISPC_IRQSTATUS_FRAMEDONE);                
        while ((INREG32(&pDispRegs->DISPC_IRQSTATUS) & DISPC_IRQSTATUS_FRAMEDONE) == 0)
        {
           if ((g_pTimerRegs->TCRR - tcrr) > DISPLAY_TIMEOUT) break;
        }        
    }

    // reset the display controller
    SETREG32(&pDispRegs->DISPC_SYSCONFIG, DISPC_SYSCONFIG_SOFTRESET);
    
    // wait until reset completes OR timeout occurs   
    tcrr = OALTimerGetReg(&g_pTimerRegs->TCRR);
    while ((INREG32(&pDispRegs->DISPC_SYSSTATUS) & DISPC_SYSSTATUS_RESETDONE) == 0)
    {
        // delay
        if ((g_pTimerRegs->TCRR - tcrr) > DISPLAY_TIMEOUT) break;
    }

    // Configure for smart-idle mode
    OUTREG32( &pDispRegs->DISPC_SYSCONFIG,
                  DISPC_SYSCONFIG_AUTOIDLE |
                  SYSCONFIG_SMARTIDLE |
                  SYSCONFIG_ENAWAKEUP |
                  SYSCONFIG_CLOCKACTIVITY_I_ON |
                  SYSCONFIG_SMARTSTANDBY
                  );

    // restore old clock settings
    PrcmDeviceEnableClocks(OMAP_DEVICE_DSS, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_DSS1, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_DSS2, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_TVOUT, FALSE);

}

#define MAX_IVA2_WAIT_COUNT 0x50000
//-----------------------------------------------------------------------------
//
//  Function:  PrcmSetIVA2OffMode()
//
//  put IVA domain back in OFF mode: 
//  Used when device is resumed from OFF mode only, there is no ongoing DSP activity.
//  This is a workaround for setting VDD1/VDD2 to 0V in OFF mode on 3730
//  Code is taken from LPM_off() with some modifications
//
void 
PrcmSetIVA2OffMode(void)
{
    UINT32 data, i;
	
     /* MPU controlled ON + ACTIVE */
    // set next IVA2 power state to ON

    data = INREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->PM_PWSTCTRL_IVA2);
    data |= (3 << 0);
    OUTREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->PM_PWSTCTRL_IVA2, data);

    // start a software supervised wake-up transition
    OUTREG32(&g_PrcmCm.pOMAP_IVA2_CM->CM_CLKSTCTRL_IVA2, 0x2);

    // wait for IVA2 power domain to switch ON
    for (i = 0; i < MAX_IVA2_WAIT_COUNT; i++) {
       if((INREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->PM_PWSTST_IVA2) & 0x3) == 0x3 ) {
       
            break;
        }
    }
    if (i == MAX_IVA2_WAIT_COUNT) {
        RETAILMSG(1, (L"Error: IVA2 power domain not ON in LPM_on\n"));
        goto fail;
    }
    // clear power-on transition request
    OUTREG32(&g_PrcmCm.pOMAP_IVA2_CM->CM_CLKSTCTRL_IVA2, 0);

    // turn on IVA2 domain functional clock
    OUTREG32(&g_PrcmCm.pOMAP_IVA2_CM->CM_FCLKEN_IVA2, 1);

    PrcmDomainResetRelease(POWERDOMAIN_IVA2, RST2_IVA2);
	
    // first wait for IVA2 state to become active
    for (i = 0; i < MAX_IVA2_WAIT_COUNT; i++) {
         if ((INREG32(&g_PrcmCm.pOMAP_IVA2_CM->CM_IDLEST_IVA2) & 0x1) ==0)  {        
            break;
        }
    }
    if (i == MAX_IVA2_WAIT_COUNT) {
        RETAILMSG(1, (L"lpm.ko: Error: IVA2 did not become active\n"));
        goto fail;
    }

    OUTREG32(&g_pSysCtrlGenReg->CONTROL_IVA2_BOOTMOD, 1);

    // enable automatic clock gating
    OUTREG32(&g_PrcmCm.pOMAP_IVA2_CM->CM_CLKSTCTRL_IVA2, 0x3);

    // set next IVA2 power state to OFF
    data = INREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->PM_PWSTCTRL_IVA2);
    data &= ~(0x3);
    OUTREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->PM_PWSTCTRL_IVA2, data);

    // ensure the IVA2_SW_RST1 status bit is cleared
    data = INREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->RM_RSTST_IVA2);
    data |= (1 << 8);
    OUTREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->RM_RSTST_IVA2, data);

    // release DSP from reset
    data = INREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->RM_RSTCTRL_IVA2);
    data &= ~(1 << 0);
    OUTREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->RM_RSTCTRL_IVA2, data);

    // wait for IVA2_RST1 signal to be released
    for (i = 0; i < MAX_IVA2_WAIT_COUNT; i++) {
         if ((INREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->RM_RSTST_IVA2) & (1 << 8)))  {        
            break;
        }
    }
    if (i == MAX_IVA2_WAIT_COUNT) {
        RETAILMSG(1, (L"lpm.ko: Error: IVA2_RST1 signal was not released\n"));
        goto fail;
    }

    // clear the IVA2_SW_RST1 status bit
    data = INREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->RM_RSTST_IVA2);
    data |= (1 << 8);
    OUTREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->RM_RSTST_IVA2, data);

    // wait for IVA2 power domain to switch OFF (~7 loops @ 312MHz)
    for (i = 0; i < MAX_IVA2_WAIT_COUNT; i++) {
       if((INREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->PM_PWSTST_IVA2) & 0x3) == 0 ) {
            break;
        }
    }
    if (i == MAX_IVA2_WAIT_COUNT) {
        RETAILMSG(1, (L"Error: IVA2 power domain not OFF (2)\n"));
        goto fail;
    }
	
    // assert reset on DSP, SEQ
    OUTREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->RM_RSTCTRL_IVA2, 0x7);
	
    // disable automatic clock gating
    OUTREG32(&g_PrcmCm.pOMAP_IVA2_CM->CM_CLKSTCTRL_IVA2, 0);

    // turn off IVA2 domain functional clock
    OUTREG32(&g_PrcmCm.pOMAP_IVA2_CM->CM_FCLKEN_IVA2, 0);
	
fail:
    RETAILMSG(1, (L"Error: IVA2 power domain funcation failed \r\n"));
    return;
   
}
 
//-----------------------------------------------------------------------------
//
//  Function:  ForceIdleMMC()
//
//  puts the mmc in force idle.  If mmc is not in force idle core will not
//  enter retention.
//
void 
ForceIdleMMC()
{
    OMAP_MMCHS_REGS    *pMmcRegs;
    
    PrcmDeviceEnableClocks(OMAP_DEVICE_MMC1, TRUE);
    pMmcRegs = OALPAtoUA(OMAP_MMCHS1_REGS_PA);
    OUTREG32(&pMmcRegs->MMCHS_SYSCONFIG, SYSCONFIG_FORCEIDLE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_MMC1, FALSE);

    PrcmDeviceEnableClocks(OMAP_DEVICE_MMC2, TRUE);
    pMmcRegs = OALPAtoUA(OMAP_MMCHS2_REGS_PA);
    OUTREG32(&pMmcRegs->MMCHS_SYSCONFIG, SYSCONFIG_FORCEIDLE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_MMC2, FALSE);
}
/* TODO DELETE
//-----------------------------------------------------------------------------
//
//  Function:  SetTwlPowerSequence
//
//  sets the twl power sequence
//
static 
void
SetTwlPowerSequence(
    HANDLE hTwl,
    UINT8 addr, 
    TWL_PMB_ENTRY *rgPwrMsg
    )
{
    int i;

    OALTritonWrite(hTwl, TWL_MEMORY_ADDRESS, (addr << 2) );

    i = 0;
    while (rgPwrMsg[i + 1].msg != 0)
        {
        OALTritonWrite(hTwl, TWL_MEMORY_DATA, (rgPwrMsg[i].msg >> 8) & 0xFF); 
        OALTritonWrite(hTwl, TWL_MEMORY_DATA, rgPwrMsg[i].msg & 0xFF);
        OALTritonWrite(hTwl, TWL_MEMORY_DATA, rgPwrMsg[i].delay);        

        // next item
        ++i;
        OALTritonWrite(hTwl, TWL_MEMORY_DATA, addr + i);
        }

    // write last entries
    OALTritonWrite(hTwl, TWL_MEMORY_DATA, rgPwrMsg[i].msg >> 8); 
    OALTritonWrite(hTwl, TWL_MEMORY_DATA, rgPwrMsg[i].msg & 0xFF);
    OALTritonWrite(hTwl, TWL_MEMORY_DATA, rgPwrMsg[i].delay);
    OALTritonWrite(hTwl, TWL_MEMORY_DATA, 0x3F);
}
*/
//-----------------------------------------------------------------------------
//
//  Function:  TWLconfigOFFmode()
//
//  Configuration of TWL to be able to put VDD1/VDD2 in OFF mode 
//
static void
TWLconfigOFFmode()
{
    void *hTwl;
    UINT8 data;
   
    hTwl = TWLOpen();
    if (hTwl == NULL) return;

    // unsecure registers
    TWLWriteByteReg(hTwl, TWL_PROTECT_KEY, 0xCE);
    TWLWriteByteReg(hTwl, TWL_PROTECT_KEY, 0xEC);

    TWLReadRegs(hTwl, TWL_CFG_P1_TRANSITION, &data, sizeof(data));          
    data &= ~STARTON_CHG;                                       
    TWLWriteByteReg(hTwl, TWL_CFG_P1_TRANSITION, data);          

     // secure registers
    TWLWriteByteReg(hTwl, TWL_PROTECT_KEY, 0x00);

    TWLWriteByteReg(hTwl, TWL_VDD1_REMAP, 0);

    TWLWriteByteReg(hTwl, TWL_VDD2_REMAP, 0);
	 
    TWLClose(hTwl);
}

//-----------------------------------------------------------------------------
//
//  Function:  InitT2PowerSequence
//
//  Configures T2 power sequencing for sleep/off and active
//
static void 
InitT2PowerSequence()
{    
    int i, j;
    void *hTwl;
	UCHAR addr = 0;
   
	OALMSG(1, (L"+OAL: InitT2PowerSequence\r\n"));
    hTwl = TWLOpen();
    if (hTwl == NULL) return;
    
    //  Initialize interrupts of T2 to all off
    TWLWriteByteReg(hTwl, TWL_PWR_ISR1, 0xFF); // Interrupt Status Register
    TWLWriteByteReg(hTwl, TWL_PWR_IMR1, 0xFF); // Interrupt Mask Register
    
    TWLWriteByteReg(hTwl, TWL_PWR_ISR2, 0xFF); // INT2
    TWLWriteByteReg(hTwl, TWL_PWR_IMR2, 0xFF);

    TWLWriteByteReg(hTwl, TWL_BCIIMR1A, 0xFF);
    TWLWriteByteReg(hTwl, TWL_BCIIMR2A, 0xFF);
    TWLWriteByteReg(hTwl, TWL_BCIIMR1B, 0xFF);
    TWLWriteByteReg(hTwl, TWL_BCIIMR2B, 0xFF);

    TWLWriteByteReg(hTwl, TWL_MADC_ISR1, 0xFF);
    TWLWriteByteReg(hTwl, TWL_MADC_IMR1, 0xFF);
    TWLWriteByteReg(hTwl, TWL_MADC_ISR2, 0xFF);
    TWLWriteByteReg(hTwl, TWL_MADC_IMR2, 0xFF);

    TWLWriteByteReg(hTwl, TWL_KEYP_IMR1, 0xFF);
    TWLWriteByteReg(hTwl, TWL_KEYP_IMR2, 0xFF);

    TWLWriteByteReg(hTwl, TWL_GPIO_IMR1A, 0xFF);
    TWLWriteByteReg(hTwl, TWL_GPIO_IMR2A, 0xFF);
    TWLWriteByteReg(hTwl, TWL_GPIO_IMR3A, 0xFF);
    TWLWriteByteReg(hTwl, TWL_GPIO_IMR1B, 0xFF);
    TWLWriteByteReg(hTwl, TWL_GPIO_IMR2B, 0xFF);
    TWLWriteByteReg(hTwl, TWL_GPIO_IMR3B, 0xFF);

    //  Ensure that that there are no pending interrupts
    //  on power and MADC
    TWLWriteByteReg(hTwl, TWL_PWR_ISR1, 0xFF);
    TWLWriteByteReg(hTwl, TWL_PWR_ISR2, 0xFF);
    TWLWriteByteReg(hTwl, TWL_MADC_ISR1, 0xFF);
    TWLWriteByteReg(hTwl, TWL_MADC_ISR2, 0xFF);
    
    // Resources in T2 are allocated to one or more of three groups.
    // Each group has a control input that tells causes the resources
    // in that group to transition between Sleep and Active states.
    // Groups and control signals are as follows:
    // P1 (Controlled by nSLEEP1) Intended for resources needed by the processor itself
    // P2 (Controlled by nSLEEP2) Intended for resources needed by the external modem (not used)
    // P3 (Controlled by CLKREQ) Intended for resources used by peripherals
    
    // EVM3530 has sys_clkreq connected to nSLEEP1, nSLEEP2 is floating, CLKREQ is pulled high and goes 
    // only to expansion connector.  So any resource allocated in group P2/P3 will never go to sleep 
    // mode when the processor drops sys_clkreq.  
    // Therefore all resources needed by the BSP need to be allocated to P1, only.
    // Change HFCLKOUT and CLKEN to P1 only instead of default P1/P2/P3 so it shuts off in suspend
	TWLWriteByteReg(hTwl, TWL_HFCLKOUT_DEV_GRP, TWL_DEV_GROUP_P1);

    // unsecure registers
    TWLWriteByteReg(hTwl, TWL_PROTECT_KEY, 0xCE);
    TWLWriteByteReg(hTwl, TWL_PROTECT_KEY, 0xEC);

    //Set ACTIVE to SLEEP SEQ address
    TWLWriteByteReg(hTwl, TWL_SEQ_ADD_A2S, T2_A2S_SEQ_START_ADDR);

    //Set SLEEP to ACTIVE SEQ address for P1 and P2
    TWLWriteByteReg(hTwl, TWL_SEQ_ADD_S2A12, T2_S2A_SEQ_START_ADDR);

    //Set SLEEP to ACTIVE SEQ address for P3
    TWLWriteByteReg(hTwl, TWL_SEQ_ADD_S2A3, T2_S2A_SEQ_START_ADDR);

    // P1 LVL_WAKEUP should be on LEVEL
    TWLWriteByteReg(hTwl, TWL_P1_SW_EVENTS, 0x08);

    // P2 LVL_WAKEUP should be on LEVEL
    TWLWriteByteReg(hTwl, TWL_P2_SW_EVENTS, 0x00); // 0x08
    
    // P3 LVL_WAKEUP should be on LEVEL 
    TWLWriteByteReg(hTwl, TWL_P3_SW_EVENTS, 0x00); // 0x08

    // Should generate correct sequence for EVM, using new method
    // Program Active to Sleep sequence
    s_pT2SleepOffSeqWords[0].msgWord = TwlSingularMsgSequence(TWL_PROCESSOR_GRP1,
														 TWL_VDD1_RES_ID,
                                                         TWL_RES_OFF,
                                                         4,
                                                         (T2_A2S_SEQ_START_ADDR+1));

    s_pT2SleepOffSeqWords[1].msgWord = TwlSingularMsgSequence(TWL_PROCESSOR_GRP1,
														 TWL_VDD2_RES_ID,
                                                         TWL_RES_OFF,
                                                         2,
                                                         (T2_A2S_SEQ_START_ADDR+2));

    s_pT2SleepOffSeqWords[2].msgWord = TwlSingularMsgSequence(TWL_PROCESSOR_GRP1,
														 TWL_VPLL1_RES_ID,
                                                         TWL_RES_OFF,
                                                         3,
                                                         (T2_A2S_SEQ_START_ADDR+3));

    s_pT2SleepOffSeqWords[3].msgWord = TwlSingularMsgSequence(TWL_PROCESSOR_GRP1,
														 TWL_HFCLKOUT_RES_ID,
                                                         TWL_RES_OFF,
                                                         3,
                                                         0x3F);

	// Set sequence start memory address for Active to Sleep
	addr = T2_A2S_SEQ_START_ADDR << 2;

	// write Active to Sleep sequence to memory
	OALMSG(1, (L"+OAL: rite Active to Sleep sequence to memory 0x%x\r\n",addr));
	for (i = 0; i < T2_SLEEPOFF_COUNT; ++i)
	{
		OALMSG(1, (L"+OAL: %d - 0x%x\r\n",i,s_pT2SleepOffSeqWords[i].msgWord));
		for (j = 0; j < 4; ++j)
		{			
			TWLWriteByteReg(hTwl, TWL_MEMORY_ADDRESS,  addr++);
			TWLWriteByteReg(hTwl, TWL_MEMORY_DATA, s_pT2SleepOffSeqWords[i].msgByte[3-j]);
		}
	}

    s_pT2WakeOnSeqWords[0].msgWord = TwlSingularMsgSequence(TWL_PROCESSOR_GRP1,
														 TWL_VPLL1_RES_ID,
                                                         TWL_RES_ACTIVE,
                                                         0x30,
                                                         (T2_S2A_SEQ_START_ADDR+1));

    s_pT2WakeOnSeqWords[1].msgWord = TwlSingularMsgSequence(TWL_PROCESSOR_GRP1,
														 TWL_VDD1_RES_ID,
                                                         TWL_RES_ACTIVE,
                                                         0x30,
                                                         (T2_S2A_SEQ_START_ADDR+2));

    s_pT2WakeOnSeqWords[2].msgWord = TwlSingularMsgSequence(TWL_PROCESSOR_GRP1,
														 TWL_VDD2_RES_ID,
                                                         TWL_RES_ACTIVE,
                                                         0x37,
                                                         (T2_S2A_SEQ_START_ADDR+3));

    s_pT2WakeOnSeqWords[3].msgWord = TwlSingularMsgSequence(TWL_PROCESSOR_GRP1,
														 TWL_HFCLKOUT_RES_ID,
                                                         TWL_RES_ACTIVE,
                                                         3,
                                                         0x3F);

    // Set sequence start memory address for Sleep to Active
	addr = T2_S2A_SEQ_START_ADDR << 2;

	// write Active to Sleep sequence to memory
	OALMSG(1, (L"+OAL: write Sleep to Active sequence to memory 0x%x\r\n",addr));
	for (i = 0; i < T2_WAKEON_COUNT; ++i)
	{
		OALMSG(1, (L"+OAL: %d - 0x%x\r\n",i,s_pT2WakeOnSeqWords[i].msgWord));
		for (j = 0; j < 4; ++j)
		{
			TWLWriteByteReg(hTwl, TWL_MEMORY_ADDRESS,  addr++);
			TWLWriteByteReg(hTwl, TWL_MEMORY_DATA, s_pT2WakeOnSeqWords[i].msgByte[3-j]);
		}
	}

    // Program Warm Reset Sequence
    //Set WARM RESET SEQ address for WARM RESET
    TWLWriteByteReg(hTwl, TWL_SEQ_ADD_WARM, T2_WARMRESET_SEQ_START_ADDR);

    s_pT2WarmResetSeqWords[0].msgWord = TwlSingularMsgSequence(TWL_PROCESSOR_GRP_NULL,
                                                            TWL_TRITON_RESET_RES_ID,
                                                            TWL_RES_OFF,
                                                            0x02,
                                                            (T2_WARMRESET_SEQ_START_ADDR+1));

    s_pT2WarmResetSeqWords[1].msgWord = TwlSingularMsgSequence(TWL_PROCESSOR_GRP1,
                                                            TWL_VDD1_RES_ID,
                                                            TWL_RES_WRST,
                                                            0x0E,
                                                            (T2_WARMRESET_SEQ_START_ADDR+2));

    s_pT2WarmResetSeqWords[2].msgWord = TwlSingularMsgSequence(TWL_PROCESSOR_GRP1,
                                                            TWL_VDD2_RES_ID,
                                                            TWL_RES_WRST,
                                                            0x0E,
                                                            (T2_WARMRESET_SEQ_START_ADDR+3));

    s_pT2WarmResetSeqWords[3].msgWord = TwlSingularMsgSequence(TWL_PROCESSOR_GRP1,
                                                            TWL_VPLL1_RES_ID,
                                                            TWL_RES_WRST,
                                                            0x60,
                                                            (T2_WARMRESET_SEQ_START_ADDR+4));

    s_pT2WarmResetSeqWords[4].msgWord = TwlSingularMsgSequence(TWL_PROCESSOR_GRP1,
                                                            TWL_32KCLK_OUT_RES_ID,
                                                            TWL_RES_ACTIVE,
                                                            0x02,
                                                            (T2_WARMRESET_SEQ_START_ADDR+5));                                                            	                                                           

    s_pT2WarmResetSeqWords[5].msgWord = TwlSingularMsgSequence(TWL_PROCESSOR_GRP_NULL,
                                                            TWL_TRITON_RESET_RES_ID,
                                                            TWL_RES_ACTIVE,
                                                            0x02,
                                                            0x3F);

    // Set sequence start memory address for WARM RESET
	addr = T2_WARMRESET_SEQ_START_ADDR << 2;

    // write WARM RESET SEQ to memory
    OALMSG(1, (L"+OAL: write WARM RESET SEQ to memory 0x%x\r\n",addr));
	for (i = 0; i < T2_WARMRESET_COUNT; ++i)
	{
		OALMSG(1, (L"+OAL: %d - 0x%x\r\n",i,s_pT2WarmResetSeqWords[i].msgWord));
		for (j = 0; j < 4; ++j)
		{
			TWLWriteByteReg(hTwl, TWL_MEMORY_ADDRESS,  addr++);
			TWLWriteByteReg(hTwl, TWL_MEMORY_DATA, s_pT2WarmResetSeqWords[i].msgByte[3-j]);
		}
	}

    // secure registers
    TWLWriteByteReg(hTwl, TWL_PROTECT_KEY, 0x00);
    
    TwlDump(hTwl);
    
    TWLClose(hTwl);
    OALMSG(1, (L"-OAL: InitT2PowerSequence\r\n"));
}

//-----------------------------------------------------------------------------
//
//  Function:  OALPowerInit
//
//  configures power and autoidle settings specific to a board
//
void 
OALPowerInit()
{ 
    PrcmInitInfo info;
    OMAP_SYSC_INTERFACE_REGS   *pSyscIF;
    OALMSG(1, (L"+OALPowerInit()\r\n"));

    pSyscIF                             = OALPAtoUA(OMAP_SYSC_INTERFACE_REGS_PA);
    g_pGPMCRegs                         = OALPAtoUA(OMAP_GPMC_REGS_PA);
    g_pSDRCRegs                         = OALPAtoUA(OMAP_SDRC_REGS_PA);
    g_pSMSRegs                          = OALPAtoUA(OMAP_SMS_REGS_PA);
    g_pVRFBRegs                         = OALPAtoUA(OMAP_VRFB_REGS_PA);
    g_pSysCtrlGenReg                    = OALPAtoUA(OMAP_SYSC_GENERAL_REGS_PA);

    g_PrcmPrm.pOMAP_GLOBAL_PRM          = OALPAtoUA(OMAP_PRCM_GLOBAL_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_OCP_SYSTEM_PRM      = OALPAtoUA(OMAP_PRCM_OCP_SYSTEM_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_CLOCK_CONTROL_PRM   = OALPAtoUA(OMAP_PRCM_CLOCK_CONTROL_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_WKUP_PRM            = OALPAtoUA(OMAP_PRCM_WKUP_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_PER_PRM             = OALPAtoUA(OMAP_PRCM_PER_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_CORE_PRM            = OALPAtoUA(OMAP_PRCM_CORE_PRM_REGS_PA);    
    g_PrcmPrm.pOMAP_MPU_PRM             = OALPAtoUA(OMAP_PRCM_MPU_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_IVA2_PRM            = OALPAtoUA(OMAP_PRCM_IVA2_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_DSS_PRM             = OALPAtoUA(OMAP_PRCM_DSS_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_SGX_PRM             = OALPAtoUA(OMAP_PRCM_SGX_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_CAM_PRM             = OALPAtoUA(OMAP_PRCM_CAM_PRM_REGS_PA);    
    g_PrcmPrm.pOMAP_NEON_PRM            = OALPAtoUA(OMAP_PRCM_NEON_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_EMU_PRM             = OALPAtoUA(OMAP_PRCM_EMU_PRM_REGS_PA);
    g_PrcmPrm.pOMAP_USBHOST_PRM         = OALPAtoUA(OMAP_PRCM_USBHOST_PRM_REGS_PA);
    
    g_PrcmCm.pOMAP_GLOBAL_CM            = OALPAtoUA(OMAP_PRCM_GLOBAL_CM_REGS_PA);
    g_PrcmCm.pOMAP_OCP_SYSTEM_CM        = OALPAtoUA(OMAP_PRCM_OCP_SYSTEM_CM_REGS_PA);
    g_PrcmCm.pOMAP_CLOCK_CONTROL_CM     = OALPAtoUA(OMAP_PRCM_CLOCK_CONTROL_CM_REGS_PA);
    g_PrcmCm.pOMAP_WKUP_CM              = OALPAtoUA(OMAP_PRCM_WKUP_CM_REGS_PA);
    g_PrcmCm.pOMAP_PER_CM               = OALPAtoUA(OMAP_PRCM_PER_CM_REGS_PA);
    g_PrcmCm.pOMAP_CORE_CM              = OALPAtoUA(OMAP_PRCM_CORE_CM_REGS_PA);
    g_PrcmCm.pOMAP_MPU_CM               = OALPAtoUA(OMAP_PRCM_MPU_CM_REGS_PA);
    g_PrcmCm.pOMAP_DSS_CM               = OALPAtoUA(OMAP_PRCM_DSS_CM_REGS_PA);
    g_PrcmCm.pOMAP_SGX_CM               = OALPAtoUA(OMAP_PRCM_SGX_CM_REGS_PA);
    g_PrcmCm.pOMAP_CAM_CM               = OALPAtoUA(OMAP_PRCM_CAM_CM_REGS_PA);    
    g_PrcmCm.pOMAP_NEON_CM              = OALPAtoUA(OMAP_PRCM_NEON_CM_REGS_PA);
    g_PrcmCm.pOMAP_EMU_CM               = OALPAtoUA(OMAP_PRCM_EMU_CM_REGS_PA); 
    g_PrcmCm.pOMAP_IVA2_CM              = OALPAtoUA(OMAP_PRCM_IVA2_CM_REGS_PA);
    g_PrcmCm.pOMAP_USBHOST_CM           = OALPAtoUA(OMAP_PRCM_USBHOST_CM_REGS_PA);

    g_pContextRestore                   = OALPAtoUA(OMAP_CONTEXT_RESTORE_REGS_PA);
    g_pPrcmRestore                      = OALPAtoUA(OMAP_PRCM_RESTORE_REGS_PA);
    g_pSdrcRestore                      = OALPAtoUA(OMAP_SDRC_RESTORE_REGS_PA);
    g_pSyscPadWkupRegs                  = OALPAtoUA(OMAP_SYSC_PADCONFS_WKUP_REGS_PA);
    g_pSyscPadConfsRegs                 = OALPAtoUA(OMAP_SYSC_PADCONFS_REGS_PA);
    g_pIdCodeReg                        = OALPAtoUA(OMAP_IDCORE_REGS_PA);
    info.pPrcmPrm                       = &g_PrcmPrm;
    info.pPrcmCm                        = &g_PrcmCm;

     // initialize the context restore module
    OALContextRestoreInit();
    SETSYSREG32(OMAP_PRCM_CORE_CM_REGS, CM_ICLKEN1_CORE, CM_CLKEN_OMAPCTRL); // SCM clock enable
        
    // initialize all devices to autoidle
    OUTSYSREG32(OMAP_PRCM_CORE_CM_REGS, CM_AUTOIDLE1_CORE, CM_AUTOIDLE1_CORE_INIT);
    OUTSYSREG32(OMAP_PRCM_CORE_CM_REGS, CM_AUTOIDLE2_CORE, CM_AUTOIDLE2_CORE_INIT);
    OUTSYSREG32(OMAP_PRCM_CORE_CM_REGS, CM_AUTOIDLE3_CORE, CM_AUTOIDLE3_CORE_INIT);

    OUTSYSREG32(OMAP_PRCM_WKUP_CM_REGS, CM_AUTOIDLE_WKUP, CM_AUTOIDLE_WKUP_INIT);

    OUTREG32(&g_PrcmCm.pOMAP_PER_CM->CM_AUTOIDLE_PER, CM_AUTOIDLE_PER_INIT);
    OUTREG32(&g_PrcmCm.pOMAP_CAM_CM->CM_AUTOIDLE_CAM, CM_AUTOIDLE_CAM_INIT);
    OUTREG32(&g_PrcmCm.pOMAP_DSS_CM->CM_AUTOIDLE_DSS, CM_AUTOIDLE_DSS_INIT);
    OUTREG32(&g_PrcmCm.pOMAP_USBHOST_CM->CM_AUTOIDLE_USBHOST, CM_AUTOIDLE_USBHOST_INIT);

    // clear all sleep dependencies.
    OUTREG32(&g_PrcmCm.pOMAP_SGX_CM->CM_SLEEPDEP_SGX, CM_SLEEPDEP_SGX_INIT);
    OUTREG32(&g_PrcmCm.pOMAP_DSS_CM->CM_SLEEPDEP_DSS, CM_SLEEPDEP_DSS_INIT);
    OUTREG32(&g_PrcmCm.pOMAP_CAM_CM->CM_SLEEPDEP_CAM, CM_SLEEPDEP_CAM_INIT);
    OUTREG32(&g_PrcmCm.pOMAP_PER_CM->CM_SLEEPDEP_PER, CM_SLEEPDEP_PER_INIT);
    OUTREG32(&g_PrcmCm.pOMAP_USBHOST_CM->CM_SLEEPDEP_USBHOST, CM_SLEEPDEP_USBHOST_INIT);

    // clear all wake dependencies.
    OUTREG32(&g_PrcmPrm.pOMAP_IVA2_PRM->PM_WKDEP_IVA2, CM_WKDEP_IVA2_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_MPU_PRM->PM_WKDEP_MPU, CM_WKDEP_MPU_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_SGX_PRM->PM_WKDEP_SGX, CM_WKDEP_SGX_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_DSS_PRM->PM_WKDEP_DSS, CM_WKDEP_DSS_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_CAM_PRM->PM_WKDEP_CAM, CM_WKDEP_CAM_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_PER_PRM->PM_WKDEP_PER, CM_WKDEP_PER_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_NEON_PRM->PM_WKDEP_NEON, CM_WKDEP_NEON_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_USBHOST_PRM->PM_WKDEP_USBHOST, CM_WKDEP_USBHOST_INIT);

    // clear all wake ability
    OUTREG32(&g_PrcmPrm.pOMAP_WKUP_PRM->PM_WKEN_WKUP, CM_WKEN_WKUP_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_CORE_PRM->PM_WKEN1_CORE, CM_WKEN1_CORE_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_PER_PRM->PM_WKEN_PER, CM_WKEN_PER_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_DSS_PRM->PM_WKEN_DSS, CM_WKEN_DSS_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_USBHOST_PRM->PM_WKEN_USBHOST, CM_WKEN_USBHOST_INIT);

    // UNDONE:
    // For now have allow all devices mpu wakeup cap.
    OUTREG32(OALPAtoVA(0x48306CA4, FALSE), 0x3CB);          // PM_MPUGRPSEL_WKUP
    OUTREG32(OALPAtoVA(0x48306AA4, FALSE), 0xC33FFE18);     // PM_MPUGRPSEL1_CORE
    OUTREG32(OALPAtoVA(0x48306AF8, FALSE), 0x00000004);     // PM_MPUGRPSEL3_CORE
    OUTREG32(OALPAtoVA(0x483070A4, FALSE), 0x0003EFFF);     // PM_MPUGRPSEL_PER
    OUTREG32(OALPAtoVA(0x483074A4, FALSE), 0x00000001);     // PM_MPUGRPSEL_USBHOST

    // If a UART is used during the basic intialization, the system will lock up.
    // The problem appears to be cause by enabling domain power management as soon 
    // as the first device in the domain is initialized, instead of initializing all
    // devices in domain before enabling domain autoidle. Since the system is single
    // threaded when this function is called, the UART use during init is the only problem.
    OALMSG(OAL_FUNC, (L" Disable serial debug messages during PRCM Device Initialize\r\n"));
    //brian OEMEnableDebugOutput(FALSE);

    OUTREG32(&g_PrcmPrm.pOMAP_GLOBAL_PRM->PRM_RSTST,
        INREG32(&g_PrcmPrm.pOMAP_GLOBAL_PRM->PRM_RSTST)); // PRM_RSTST - clear to 0

    // ERRATA WORKAROUND: 1.107
    // clear an EMU reserved bit so the EMU power domain
    // won't lock-up on wake-up
    OUTREG32(&g_PrcmPrm.pOMAP_EMU_PRM->PM_PWSTST_EMU, 0);

    // set SDRC_POWER_REG register
    OUTREG32(&g_pSDRCRegs->SDRC_POWER, BSP_SDRC_POWER_REG);

    // configure setup times for voltage and src clk
    OUTREG32(&g_PrcmPrm.pOMAP_GLOBAL_PRM->PRM_CLKSETUP, BSP_PRM_CLKSETUP);
    OUTREG32(&g_PrcmPrm.pOMAP_GLOBAL_PRM->PRM_VOLTSETUP1, BSP_PRM_VOLTSETUP1_INIT);
    OUTREG32(&g_PrcmPrm.pOMAP_GLOBAL_PRM->PRM_VOLTSETUP2, BSP_PRM_VOLTSETUP2);
    OUTREG32(&g_PrcmPrm.pOMAP_GLOBAL_PRM->PRM_VOLTOFFSET, BSP_PRM_VOLTOFFSET);

    // initialize prcm library
    PrcmInit(&info);
    
    //----------------------------------------------------------------------
    // Initialize I2C devices
    //----------------------------------------------------------------------
    OALI2CInit(OMAP_DEVICE_I2C1); // TPS65950
    OALI2CInit(OMAP_DEVICE_I2C2); // G-Sensor
    //OALI2CInit(OMAP_DEVICE_I2C3); // DVI

    //----------------------------------------------------------------------
    // clock GPIO banks
    //----------------------------------------------------------------------
    EnableDeviceClocks(OMAP_DEVICE_GPIO1,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO2,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO3,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO4,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO5,TRUE);
    EnableDeviceClocks(OMAP_DEVICE_GPIO6,TRUE);

    // initialize pin mux table
    OALMux_InitMuxTable(); // nothing
   
    // set clock transition delay
    PrcmClockSetSystemClockSetupTime((USHORT)dwOEMPRCMCLKSSetupTime);

    // - Automatically send command for off, retention, sleep
    PrcmVoltSetAutoControl(
        AUTO_OFF_ENABLED | AUTO_RET_DISABLED | AUTO_SLEEP_DISABLED,
        AUTO_OFF | AUTO_RET | AUTO_SLEEP);

    // - set polarity modes
    // - SYS_CLKREQ is active high
    PrcmVoltSetControlPolarity(
        CLKREQ_POL_ACTIVEHIGH | OFFMODE_POL_ACTIVELOW | CLKOUT_POL_INACTIVELOW | EXTVOL_POL_ACTIVELOW,
        OFFMODE_POL | CLKOUT_POL | CLKREQ_POL | EXTVOL_POL);

    // de-assert external clk req on retention and off
    PrcmClockSetExternalClockRequestMode(AUTOEXTCLKMODE_INRETENTION);

    InitT2PowerSequence();

    // we need to write FORCEIDLE to the GPMC register or else it will prevent
    // the core from entering retention.  After the write smartidle can be
    // selected
    OUTREG32(&g_pGPMCRegs->GPMC_SYSCONFIG, SYSCONFIG_FORCEIDLE | SYSCONFIG_AUTOIDLE);

    // configure SCM, SDRC, SMS, and GPMC to smartidle/autoidle
    OUTREG32(&s_pSyscIFContext->CONTROL_SYSCONFIG, SYSCONFIG_SMARTIDLE | SYSCONFIG_AUTOIDLE);
    OUTREG32(&g_pSMSRegs->SMS_SYSCONFIG, SYSCONFIG_AUTOIDLE | SYSCONFIG_SMARTIDLE);
    OUTREG32(&g_pSDRCRegs->SDRC_SYSCONFIG, SYSCONFIG_SMARTIDLE);
    OUTREG32(&g_pGPMCRegs->GPMC_SYSCONFIG, SYSCONFIG_SMARTIDLE | SYSCONFIG_AUTOIDLE);

    #ifdef DEBUG_PRCM_SUSPEND_RESUME
        g_PrcmDebugSuspendResume = TRUE;
    #endif
		
    // re-enable serial debug output
    OEMEnableDebugOutput(TRUE);
    OALMSG(OAL_FUNC, (L" Serial debug messages renabled\r\n"));

    OALMSG(1, (L"-OALPowerInit()\r\n"));
}

//-----------------------------------------------------------------------------
//
//  Function:  OALPowerPostInit
//
//  Called by kernel to allow system to continue initialization with
//  a full featured kernel.
//
VOID
OALPowerPostInit()
{
    IOCTL_PRCM_CLOCK_SET_DPLLSTATE_IN dpllInfo;
    
    DWORD rgDomain = DVFS_MPU1_OPP;
    DWORD * rgOpp = OALArgsQuery(OAL_ARGS_QUERY_OPP_MODE);    

    OALMSG(1, (L"+OALPowerPostInit\r\n"));

    // Allow power compontents to initialize with a full featured kernel
    // before threads are scheduled
    PrcmPostInit();
    OALI2CPostInit();
    PrcmContextRestoreInit();
    Opp_init();
    SmartReflex_Initialize();
    SmartReflex_PostInitialize();

    SetOpp(&rgDomain,rgOpp,1);

    if(g_dwCpuFamily == CPU_FAMILY_DM37XX)
    {
        OALSDRCRefreshCounter(BSP_HYNIX_ARCV, BSP_HYNIX_ARCV_LOW);
    }
    else if (g_dwCpuFamily == CPU_FAMILY_OMAP35XX)
    {
        OALSDRCRefreshCounter(BSP_MICRON_ARCV, BSP_MICRON_ARCV_LOW);
    }
    else
    {
        OALMSG(OAL_ERROR, (L"CPU family(%d) is not Supported.\r\n", g_dwCpuFamily));
    }

    // ES 1.0
    // Need to force USB into standby mode to allow OMAP to go
    // into full retention
    ForceStandbyUSB();

    // Force MMC into idle
    ForceIdleMMC();

    // Disable clocks enabled by bootloader, except display
    PrcmDeviceEnableClocks(OMAP_DEVICE_MMC1, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_DSS2, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_TVOUT, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_WDT2, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_GPTIMER1, FALSE);
    PrcmDeviceEnableClocks(OMAP_DEVICE_HSOTGUSB, FALSE);

    // - PowerIC controlled via I2C
    // - T2 doesn't support OFF command via I2C
    PrcmVoltSetControlMode(
        SEL_VMODE_I2C | SEL_OFF_SIGNALLINE,
        SEL_VMODE | SEL_OFF
        );

    // configure memory on/retention/off levels
    PrcmDomainSetMemoryState(POWERDOMAIN_MPU,
        L2CACHEONSTATE_MEMORYON_DOMAINON | L2CACHERETSTATE_MEMORYRET_DOMAINRET |
        LOGICRETSTATE_LOGICRET_DOMAINRET,
        L2CACHEONSTATE | L2CACHERETSTATE |
        LOGICRETSTATE
        );

    PrcmDomainSetMemoryState(POWERDOMAIN_IVA2,
        L2FLATMEMONSTATE_MEMORYON_DOMAINON | SHAREDL2CACHEFLATONSTATE_MEMORYON_DOMAINON |
        L1FLATMEMONSTATE_MEMORYON_DOMAINON | SHAREDL1CACHEFLATONSTATE_MEMORYON_DOMAINON |
        L2FLATMEMRETSTATE_MEMORYRET_DOMAINRET | SHAREDL2CACHEFLATRETSTATE_MEMORYRET_DOMAINRET |
        L1FLATMEMRETSTATE_MEMORYRET_DOMAINRET | SHAREDL1CACHEFLATRETSTATE_MEMORYRET_DOMAINRET |
        LOGICRETSTATE_LOGICRET_DOMAINRET,
        L2FLATMEMONSTATE | SHAREDL2CACHEFLATONSTATE |
        L1FLATMEMONSTATE | SHAREDL1CACHEFLATONSTATE |
        L2FLATMEMRETSTATE | SHAREDL2CACHEFLATRETSTATE |
        L1FLATMEMRETSTATE | SHAREDL1CACHEFLATRETSTATE |
        LOGICRETSTATE
        );

    PrcmDomainSetMemoryState(POWERDOMAIN_CORE,
        MEM2ONSTATE_MEMORYON_DOMAINON | MEM1ONSTATE_MEMORYON_DOMAINON |
        MEM2RETSTATE_MEMORYRET_DOMAINRET | MEM2RETSTATE_MEMORYRET_DOMAINRET |
        LOGICRETSTATE_LOGICRET_DOMAINRET,
        MEM2ONSTATE | MEM1ONSTATE |
        MEM2RETSTATE | MEM2RETSTATE |
        LOGICRETSTATE
        );

    PrcmDomainSetMemoryState(POWERDOMAIN_PERIPHERAL,
        LOGICRETSTATE_LOGICRET_DOMAINRET,
        LOGICRETSTATE
        );


    // ES3.1 fix
    if (g_dwCpuRevision == CPU_FAMILY_35XX_REVISION_ES_3_1)
        {
        // enable USBTLL SAR
        PrcmDomainSetMemoryState(POWERDOMAIN_CORE, SAVEANDRESTORE, SAVEANDRESTORE);

        // configure USBHOST for memory on when domain is on, logic retained when domain is in retention
        PrcmDomainSetMemoryState(POWERDOMAIN_USBHOST,
            LOGICRETSTATE_LOGICRET_DOMAINRET | MEMONSTATE_MEMORYON_DOMAINON,
            LOGICRETSTATE | MEMONSTATE
            );
        }

    // update dpll settings    
    dpllInfo.size = sizeof(IOCTL_PRCM_CLOCK_SET_DPLLSTATE_IN);
    dpllInfo.lowPowerEnabled = FALSE;
    dpllInfo.driftGuardEnabled = FALSE;
    dpllInfo.dpllMode = DPLL_MODE_LOCK; 
    dpllInfo.ffMask = DPLL_UPDATE_ALL;
    dpllInfo.rampTime = DPLL_RAMPTIME_20;
    dpllInfo.dpllAutoidleState = DPLL_AUTOIDLE_LOWPOWERSTOPMODE;
   
    // dpll1
    dpllInfo.dpllId = kDPLL1;
    PrcmClockSetDpllState(&dpllInfo);

    // dpll2
    dpllInfo.dpllId = kDPLL2;  
    PrcmClockSetDpllState(&dpllInfo);

    // dpll3
    dpllInfo.rampTime = DPLL_RAMPTIME_DISABLE;
    dpllInfo.dpllId = kDPLL3;
    PrcmClockSetDpllState(&dpllInfo);

    // dpll4
    dpllInfo.dpllId = kDPLL4;
    PrcmClockSetDpllState(&dpllInfo);

    // dpll5
    dpllInfo.dpllId = kDPLL5;
    PrcmClockSetDpllState(&dpllInfo);

    OALWakeupLatency_Initialize();

    // set the floor for powerdomains to be retention
    PrcmDomainSetPowerState(POWERDOMAIN_CORE, POWERSTATE_RETENTION, LOGICRETSTATE);
    PrcmDomainSetPowerState(POWERDOMAIN_MPU, POWERSTATE_RETENTION, LOGICRETSTATE);
    PrcmDomainSetPowerState(POWERDOMAIN_CAMERA, POWERSTATE_OFF, LOGICRETSTATE);
    PrcmDomainSetPowerState(POWERDOMAIN_PERIPHERAL, POWERSTATE_RETENTION, LOGICRETSTATE);
    PrcmDomainSetPowerState(POWERDOMAIN_DSS, POWERSTATE_RETENTION, LOGICRETSTATE);
    PrcmDomainSetPowerState(POWERDOMAIN_SGX, POWERSTATE_OFF, LOGICRETSTATE);
    PrcmDomainSetPowerState(POWERDOMAIN_USBHOST, POWERSTATE_OFF, LOGICRETSTATE);
    PrcmDomainSetPowerState(POWERDOMAIN_EMULATION, POWERSTATE_OFF, LOGICRETSTATE);
    PrcmDomainSetPowerState(POWERDOMAIN_NEON, POWERSTATE_RETENTION, LOGICRETSTATE);
    PrcmDomainSetPowerState(POWERDOMAIN_IVA2, POWERSTATE_OFF, LOGICRETSTATE);

    PrcmDomainSetClockState(POWERDOMAIN_CORE, CLOCKDOMAIN_L3, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_CORE, CLOCKDOMAIN_L4, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_CORE, CLOCKDOMAIN_D2D, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_MPU, CLOCKDOMAIN_MPU, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_CAMERA, CLOCKDOMAIN_CAMERA, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_PERIPHERAL, CLOCKDOMAIN_PERIPHERAL, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_DSS, CLOCKDOMAIN_DSS, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_SGX, CLOCKDOMAIN_SGX, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_USBHOST, CLOCKDOMAIN_USBHOST, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_EMULATION, CLOCKDOMAIN_EMULATION, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_NEON, CLOCKDOMAIN_NEON, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_IVA2, CLOCKDOMAIN_IVA2, CLKSTCTRL_AUTOMATIC);

    // Enable IO interrupt
    PrcmInterruptEnable(PRM_IRQENABLE_IO_EN, TRUE);
    TWLconfigOFFmode();
	
    // Enable FBB for higher OPP on 37x
    if (g_dwCpuFamily == CPU_FAMILY_DM37XX)
        PrcmVoltScaleVoltageABB(BSP_OPM_SELECT_37XX);

    // save context for all registers restored by the kernel
    OALContextSaveSMS();
    OALContextSaveGPMC();
    OALContextSaveINTC();
    OALContextSavePRCM();
    OALContextSaveGPIO();
    OALContextSaveSCM();
    OALContextSaveDMA();
    OALContextSaveMux();
    OALContextSaveVRFB();

    OALMSG(1, (L"-OALPowerPostInit\r\n"));
}

//------------------------------------------------------------------------------
//
//  Function:  OALPowerVFP
//
//  Controls the VFP/Neon power domain.
//
BOOL
OALPowerVFP(DWORD dwCommand)
{
#if 1
    UNREFERENCED_PARAMETER(dwCommand);
#else
    switch (dwCommand)
    {
    case VFP_CONTROL_POWER_ON:
        PrcmDomainSetPowerState(POWERDOMAIN_NEON, POWERSTATE_ON, LOGICRETSTATE);
        return TRUE;

    case VFP_CONTROL_POWER_OFF:
        PrcmDomainSetPowerState(POWERDOMAIN_NEON, POWERSTATE_OFF, LOGICRETSTATE);
        return TRUE;

    case VFP_CONTROL_POWER_RETENTION:
        PrcmDomainSetPowerState(POWERDOMAIN_NEON, POWERSTATE_RETENTION, LOGICRETSTATE);
        return TRUE;
    }
#endif
    return FALSE;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALPrcmIntrHandler
//
//  This function implements prcm interrupt handler. This routine is called
//  from the OEMInterruptHandler
//
UINT32
OALPrcmIntrHandler()
{
    const UINT clear_mask = PRM_IRQENABLE_VP1_NOSMPSACK_EN |
                            PRM_IRQENABLE_VP2_NOSMPSACK_EN |
                            PRM_IRQENABLE_VC_SAERR_EN |
                            PRM_IRQENABLE_VC_RAERR_EN |
                            PRM_IRQENABLE_VC_TIMEOUTERR_EN |
                            PRM_IRQENABLE_WKUP_EN |
                            PRM_IRQENABLE_TRANSITION_EN |
                            PRM_IRQENABLE_MPU_DPLL_RECAL_EN |
                            PRM_IRQENABLE_CORE_DPLL_RECAL_EN |
                            PRM_IRQENABLE_VP1_OPPCHANGEDONE_EN |
                            PRM_IRQENABLE_VP2_OPPCHANGEDONE_EN |
                            PRM_IRQENABLE_IO_EN ;
    UINT sysIntr = SYSINTR_NOP;

    OALMSG(OAL_FUNC, (L"+OALPrcmIntrHandler\r\n"));

    // get cause of interrupt
    sysIntr = PrcmInterruptProcess(clear_mask);

    OALMSG(OAL_FUNC, (L"-OALPrcmIntrHandler\r\n"));
    return sysIntr;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALSmartReflex1Intr
//
//  This function implements SmartReflex1 interrupt handler. 
//  This routine is called from the OEMInterruptHandler
//
UINT32 
OALSmartReflex1Intr(
    )
{
    UINT intrStatus;
    OALMSG(OAL_FUNC, (L"+OALSmartReflex1Intr\r\n"));

    // get cause of interrupt
    intrStatus = SmartReflex_funcs->ClearInterruptStatus(kSmartReflex_Channel1,
                    ERRCONFIG_INTR_SR_MASK
                    );
    
    OALMSG(TRUE, (L"OALSmartReflex1Intr intrStatus=0x%08X\r\n", intrStatus));
    OALMSG(OAL_FUNC, (L"-OALSmartReflex1Intr\r\n"));
    return SYSINTR_NOP;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALSmartReflex2Intr
//
//  This function implements SmartReflex2 interrupt handler. 
//  This routine is called from the OEMInterruptHandler
//
UINT32 
OALSmartReflex2Intr(
    )
{
    UINT intrStatus;
    OALMSG(OAL_FUNC, (L"+OALSmartReflex2Intr\r\n"));

    // get cause of interrupt
    intrStatus = SmartReflex_funcs->ClearInterruptStatus(kSmartReflex_Channel2,
                    ERRCONFIG_INTR_SR_MASK
                    );
    
    OALMSG(TRUE, (L"OALSmartReflex2Intr intrStatus=0x%08X\r\n", intrStatus));
    OALMSG(OAL_FUNC, (L"-OALSmartReflex2Intr\r\n"));
    return SYSINTR_NOP;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveGPIO
//
//  Saves the GPIO Context, clears the IRQ for OFF mode
//
VOID
OALContextSaveGPIO ()
{
    UINT32 i;

    for(i=0; i< OMAP_GPIO_BANK_TO_RESTORE; i++)
        {
        // disable gpio clocks
        s_rgGpioContext[i].SYSCONFIG        =   INREG32(&s_rgGpioRegsAddr[i]->SYSCONFIG);
        s_rgGpioContext[i].IRQENABLE1       =   INREG32(&s_rgGpioRegsAddr[i]->IRQENABLE1);
        s_rgGpioContext[i].WAKEUPENABLE     =   INREG32(&s_rgGpioRegsAddr[i]->WAKEUPENABLE);
        s_rgGpioContext[i].IRQENABLE2       =   INREG32(&s_rgGpioRegsAddr[i]->IRQENABLE2);
        s_rgGpioContext[i].CTRL             =   INREG32(&s_rgGpioRegsAddr[i]->CTRL);
        s_rgGpioContext[i].OE               =   INREG32(&s_rgGpioRegsAddr[i]->OE);
        s_rgGpioContext[i].LEVELDETECT0     =   INREG32(&s_rgGpioRegsAddr[i]->LEVELDETECT0);
        s_rgGpioContext[i].LEVELDETECT1     =   INREG32(&s_rgGpioRegsAddr[i]->LEVELDETECT1);
        s_rgGpioContext[i].RISINGDETECT     =   INREG32(&s_rgGpioRegsAddr[i]->RISINGDETECT);
        s_rgGpioContext[i].FALLINGDETECT    =   INREG32(&s_rgGpioRegsAddr[i]->FALLINGDETECT);
        s_rgGpioContext[i].DEBOUNCENABLE    =   INREG32(&s_rgGpioRegsAddr[i]->DEBOUNCENABLE);
        s_rgGpioContext[i].DEBOUNCINGTIME   =   INREG32(&s_rgGpioRegsAddr[i]->DEBOUNCINGTIME);
        s_rgGpioContext[i].DATAOUT          =   INREG32(&s_rgGpioRegsAddr[i]->DATAOUT);
        }

    // clear dirty bit for gpio
    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_GPIO;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveGPMC
//
//  Stores the GPMC Registers in shadow register
//
VOID
OALContextSaveGPMC ()
{
    // Read the GPMC registers value and store it in shadow variable.
    s_gpmcContext.GPMC_SYSCONFIG = INREG32(&g_pGPMCRegs->GPMC_SYSCONFIG);
    s_gpmcContext.GPMC_IRQENABLE = INREG32(&g_pGPMCRegs->GPMC_IRQENABLE);
    s_gpmcContext.GPMC_TIMEOUT_CONTROL = INREG32(&g_pGPMCRegs->GPMC_TIMEOUT_CONTROL);
    s_gpmcContext.GPMC_CONFIG = INREG32(&g_pGPMCRegs->GPMC_CONFIG);
    s_gpmcContext.GPMC_PREFETCH_CONFIG1 = INREG32(&g_pGPMCRegs->GPMC_PREFETCH_CONFIG1);
    s_gpmcContext.GPMC_PREFETCH_CONFIG2 = INREG32(&g_pGPMCRegs->GPMC_PREFETCH_CONFIG2);
    s_gpmcContext.GPMC_PREFETCH_CONTROL = INREG32(&g_pGPMCRegs->GPMC_PREFETCH_CONTROL);

    // Store the GPMC CS0 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_0) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_0 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_0);
        s_gpmcContext.GPMC_CONFIG2_0 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_0);
        s_gpmcContext.GPMC_CONFIG3_0 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_0);
        s_gpmcContext.GPMC_CONFIG4_0 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_0);
        s_gpmcContext.GPMC_CONFIG5_0 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_0);
        s_gpmcContext.GPMC_CONFIG6_0 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_0);
        s_gpmcContext.GPMC_CONFIG7_0 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_0);
    }

    // Store the GPMC CS1 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_1) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_1 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_1);
        s_gpmcContext.GPMC_CONFIG2_1 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_1);
        s_gpmcContext.GPMC_CONFIG3_1 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_1);
        s_gpmcContext.GPMC_CONFIG4_1 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_1);
        s_gpmcContext.GPMC_CONFIG5_1 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_1);
        s_gpmcContext.GPMC_CONFIG6_1 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_1);
        s_gpmcContext.GPMC_CONFIG7_1 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_1);
    }

    // Store the GPMC CS2 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_2) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_2 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_2);
        s_gpmcContext.GPMC_CONFIG2_2 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_2);
        s_gpmcContext.GPMC_CONFIG3_2 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_2);
        s_gpmcContext.GPMC_CONFIG4_2 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_2);
        s_gpmcContext.GPMC_CONFIG5_2 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_2);
        s_gpmcContext.GPMC_CONFIG6_2 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_2);
        s_gpmcContext.GPMC_CONFIG7_2 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_2);
    }

    // Store the GPMC CS3 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_3) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_3 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_3);
        s_gpmcContext.GPMC_CONFIG2_3 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_3);
        s_gpmcContext.GPMC_CONFIG3_3 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_3);
        s_gpmcContext.GPMC_CONFIG4_3 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_3);
        s_gpmcContext.GPMC_CONFIG5_3 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_3);
        s_gpmcContext.GPMC_CONFIG6_3 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_3);
        s_gpmcContext.GPMC_CONFIG7_3 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_3);
    }

    // Store the GPMC CS4 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_4) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_4 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_4);
        s_gpmcContext.GPMC_CONFIG2_4 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_4);
        s_gpmcContext.GPMC_CONFIG3_4 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_4);
        s_gpmcContext.GPMC_CONFIG4_4 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_4);
        s_gpmcContext.GPMC_CONFIG5_4 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_4);
        s_gpmcContext.GPMC_CONFIG6_4 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_4);
        s_gpmcContext.GPMC_CONFIG7_4 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_4);
    }

    // Store the GPMC CS5 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_5) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_5 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_5);
        s_gpmcContext.GPMC_CONFIG2_5 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_5);
        s_gpmcContext.GPMC_CONFIG3_5 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_5);
        s_gpmcContext.GPMC_CONFIG4_5 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_5);
        s_gpmcContext.GPMC_CONFIG5_5 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_5);
        s_gpmcContext.GPMC_CONFIG6_5 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_5);
        s_gpmcContext.GPMC_CONFIG7_5 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_5);
    }

    // Store the GPMC CS6 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_6) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_6 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_6);
        s_gpmcContext.GPMC_CONFIG2_6 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_6);
        s_gpmcContext.GPMC_CONFIG3_6 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_6);
        s_gpmcContext.GPMC_CONFIG4_6 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_6);
        s_gpmcContext.GPMC_CONFIG5_6 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_6);
        s_gpmcContext.GPMC_CONFIG6_6 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_6);
        s_gpmcContext.GPMC_CONFIG7_6 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_6);
    }

    // Store the GPMC CS7 group only if it is enabled.
    if(INREG32(&g_pGPMCRegs->GPMC_CONFIG7_7) & GPMC_CSVALID)
    {
        s_gpmcContext.GPMC_CONFIG1_7 = INREG32(&g_pGPMCRegs->GPMC_CONFIG1_7);
        s_gpmcContext.GPMC_CONFIG2_7 = INREG32(&g_pGPMCRegs->GPMC_CONFIG2_7);
        s_gpmcContext.GPMC_CONFIG3_7 = INREG32(&g_pGPMCRegs->GPMC_CONFIG3_7);
        s_gpmcContext.GPMC_CONFIG4_7 = INREG32(&g_pGPMCRegs->GPMC_CONFIG4_7);
        s_gpmcContext.GPMC_CONFIG5_7 = INREG32(&g_pGPMCRegs->GPMC_CONFIG5_7);
        s_gpmcContext.GPMC_CONFIG6_7 = INREG32(&g_pGPMCRegs->GPMC_CONFIG6_7);
        s_gpmcContext.GPMC_CONFIG7_7 = INREG32(&g_pGPMCRegs->GPMC_CONFIG7_7);
    }

    // clear dirty bit
    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_GPMC;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveSCM
//
//  Stores the SCM Registers in shadow register
//
VOID
OALContextSaveSCM ()
{
    // Read the SCM registers value and store it in shadow variable.

    s_syscIntContext.CONTROL_SYSCONFIG = INREG32(&s_pSyscIFContext->CONTROL_SYSCONFIG);

    s_syscGenContext.CONTROL_DEVCONF0      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEVCONF0);
    s_syscGenContext.CONTROL_MEM_DFTRW0    = INREG32(&g_pSysCtrlGenReg->CONTROL_MEM_DFTRW0);
    s_syscGenContext.CONTROL_MEM_DFTRW1    = INREG32(&g_pSysCtrlGenReg->CONTROL_MEM_DFTRW1);
    s_syscGenContext.CONTROL_MSUSPENDMUX_0 = INREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_0);
    s_syscGenContext.CONTROL_MSUSPENDMUX_1 = INREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_1);
    s_syscGenContext.CONTROL_MSUSPENDMUX_2 = INREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_2);
    s_syscGenContext.CONTROL_MSUSPENDMUX_3 = INREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_3);
    s_syscGenContext.CONTROL_MSUSPENDMUX_4 = INREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_4);
    s_syscGenContext.CONTROL_MSUSPENDMUX_5 = INREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_5);
    s_syscGenContext.CONTROL_SEC_CTRL      = INREG32(&g_pSysCtrlGenReg->CONTROL_SEC_CTRL);
    s_syscGenContext.CONTROL_DEVCONF1      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEVCONF1);
    s_syscGenContext.CONTROL_CSIRXFE       = INREG32(&g_pSysCtrlGenReg->CONTROL_CSIRXFE);
    s_syscGenContext.CONTROL_IVA2_BOOTADDR = INREG32(&g_pSysCtrlGenReg->CONTROL_IVA2_BOOTADDR);
    s_syscGenContext.CONTROL_IVA2_BOOTMOD  = INREG32(&g_pSysCtrlGenReg->CONTROL_IVA2_BOOTMOD);
    s_syscGenContext.CONTROL_DEBOBS_0      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_0);
    s_syscGenContext.CONTROL_DEBOBS_1      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_1);
    s_syscGenContext.CONTROL_DEBOBS_2      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_2);
    s_syscGenContext.CONTROL_DEBOBS_3      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_3);
    s_syscGenContext.CONTROL_DEBOBS_4      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_4);
    s_syscGenContext.CONTROL_DEBOBS_5      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_5);
    s_syscGenContext.CONTROL_DEBOBS_6      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_6);
    s_syscGenContext.CONTROL_DEBOBS_7      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_7);
    s_syscGenContext.CONTROL_DEBOBS_8      = INREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_8);
    s_syscGenContext.CONTROL_PROG_IO0      = INREG32(&g_pSysCtrlGenReg->CONTROL_PROG_IO0);
    s_syscGenContext.CONTROL_PROG_IO1      = INREG32(&g_pSysCtrlGenReg->CONTROL_PROG_IO1);

    s_syscGenContext.CONTROL_DSS_DPLL_SPREADING     = INREG32(&g_pSysCtrlGenReg->CONTROL_DSS_DPLL_SPREADING);
    s_syscGenContext.CONTROL_CORE_DPLL_SPREADING    = INREG32(&g_pSysCtrlGenReg->CONTROL_CORE_DPLL_SPREADING);
    s_syscGenContext.CONTROL_PER_DPLL_SPREADING     = INREG32(&g_pSysCtrlGenReg->CONTROL_PER_DPLL_SPREADING);
    s_syscGenContext.CONTROL_USBHOST_DPLL_SPREADING = INREG32(&g_pSysCtrlGenReg->CONTROL_USBHOST_DPLL_SPREADING);
    s_syscGenContext.CONTROL_PBIAS_LITE             = INREG32(&g_pSysCtrlGenReg->CONTROL_PBIAS_LITE);
    s_syscGenContext.CONTROL_TEMP_SENSOR            = INREG32(&g_pSysCtrlGenReg->CONTROL_TEMP_SENSOR);
    s_syscGenContext.CONTROL_SRAMLDO4               = INREG32(&g_pSysCtrlGenReg->CONTROL_SRAMLDO4);
    s_syscGenContext.CONTROL_SRAMLDO5               = INREG32(&g_pSysCtrlGenReg->CONTROL_SRAMLDO5);
    s_syscGenContext.CONTROL_CSI                    = INREG32(&g_pSysCtrlGenReg->CONTROL_CSI);

    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_SCM;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSavePRCM
//
//  Stores the PRCM Registers in shadow register
//
VOID
OALContextSavePRCM ()
{
    s_ocpSysCmContext.CM_SYSCONFIG  = INREG32(&g_PrcmCm.pOMAP_OCP_SYSTEM_CM->CM_SYSCONFIG);

    s_coreCmContext.CM_AUTOIDLE1_CORE = INREG32(&g_PrcmCm.pOMAP_CORE_CM->CM_AUTOIDLE1_CORE);
    s_coreCmContext.CM_AUTOIDLE2_CORE = INREG32(&g_PrcmCm.pOMAP_CORE_CM->CM_AUTOIDLE2_CORE);
    s_coreCmContext.CM_AUTOIDLE3_CORE = INREG32(&g_PrcmCm.pOMAP_CORE_CM->CM_AUTOIDLE3_CORE);

    s_wkupCmContext.CM_FCLKEN_WKUP    = INREG32(&g_PrcmCm.pOMAP_WKUP_CM->CM_FCLKEN_WKUP);
    s_wkupCmContext.CM_ICLKEN_WKUP    = INREG32(&g_PrcmCm.pOMAP_WKUP_CM->CM_ICLKEN_WKUP);
    s_wkupCmContext.CM_AUTOIDLE_WKUP  = INREG32(&g_PrcmCm.pOMAP_WKUP_CM->CM_AUTOIDLE_WKUP);

    s_clkCtrlCmContext.CM_CLKEN2_PLL = INREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKEN2_PLL);
    s_clkCtrlCmContext.CM_CLKSEL4_PLL = INREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKSEL4_PLL);
    s_clkCtrlCmContext.CM_CLKSEL5_PLL = INREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKSEL5_PLL);
    s_clkCtrlCmContext.CM_AUTOIDLE2_PLL = INREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_AUTOIDLE2_PLL);

    s_globalCmContext.CM_POLCTRL = INREG32(&g_PrcmCm.pOMAP_GLOBAL_CM->CM_POLCTRL);

    s_clkCtrlCmContext.CM_CLKOUT_CTRL = INREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKOUT_CTRL);
    s_clkCtrlCmContext.CM_AUTOIDLE_PLL = INREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_AUTOIDLE_PLL);

    s_emuCmContext.CM_CLKSEL1_EMU = INREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSEL1_EMU);
    s_emuCmContext.CM_CLKSTCTRL_EMU = INREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSTCTRL_EMU);
    s_emuCmContext.CM_CLKSEL2_EMU = INREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSEL2_EMU);
    s_emuCmContext.CM_CLKSEL3_EMU = INREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSEL3_EMU);

    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_PRCM;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveINTC
//
//  Stores the MPU IC Registers in shadow register
//
VOID
OALContextSaveINTC ()
{
    UINT32 i;

    s_intcContext.INTC_SYSCONFIG = INREG32(&g_pIntr->pICLRegs->INTC_SYSCONFIG);
    s_intcContext.INTC_PROTECTION = INREG32(&g_pIntr->pICLRegs->INTC_PROTECTION);
    s_intcContext.INTC_IDLE = INREG32(&g_pIntr->pICLRegs->INTC_IDLE);
    s_intcContext.INTC_THRESHOLD = INREG32(&g_pIntr->pICLRegs->INTC_THRESHOLD);

    for (i = 0; i < dimof(g_pIntr->pICLRegs->INTC_ILR); i++)
        s_intcContext.INTC_ILR[i] = INREG32(&g_pIntr->pICLRegs->INTC_ILR[i]);

    s_intcContext.INTC_MIR0 = INREG32(&g_pIntr->pICLRegs->INTC_MIR0);
    s_intcContext.INTC_MIR1 = INREG32(&g_pIntr->pICLRegs->INTC_MIR1);
    s_intcContext.INTC_MIR2 = INREG32(&g_pIntr->pICLRegs->INTC_MIR2);

    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_INTC;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveDMA
//
//  Saves the DMA Registers
//
VOID
OALContextSaveDMA()
{
    s_dmaController.DMA4_GCR           = INREG32(&s_pDmaController->DMA4_GCR);
    s_dmaController.DMA4_OCP_SYSCONFIG = INREG32(&s_pDmaController->DMA4_OCP_SYSCONFIG);
    s_dmaController.DMA4_IRQENABLE_L0  = INREG32(&s_pDmaController->DMA4_IRQENABLE_L0);
    s_dmaController.DMA4_IRQENABLE_L1  = INREG32(&s_pDmaController->DMA4_IRQENABLE_L1);
    s_dmaController.DMA4_IRQENABLE_L2  = INREG32(&s_pDmaController->DMA4_IRQENABLE_L2);
    s_dmaController.DMA4_IRQENABLE_L3  = INREG32(&s_pDmaController->DMA4_IRQENABLE_L3);

    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_DMA;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveSMS
//
//  Saves the SMS Registers
//
VOID
OALContextSaveSMS()
{
    s_smsContext.SMS_SYSCONFIG         = INREG32(&g_pSMSRegs->SMS_SYSCONFIG);

    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_SMS;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveVRFB
//
//  Saves the VRFB Registers
//
VOID
OALContextSaveVRFB()
{
    int i;

    for (i = 0; i < VRFB_ROTATION_CONTEXTS; ++i)
        {
        s_vrfbContext.aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_CONTROL =
            INREG32(&g_pVRFBRegs->aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_CONTROL);

        s_vrfbContext.aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_SIZE =
            INREG32(&g_pVRFBRegs->aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_SIZE);

        s_vrfbContext.aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_PHYSICAL_BA =
            INREG32(&g_pVRFBRegs->aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_PHYSICAL_BA);
        }

    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_VRFB;
}


//-----------------------------------------------------------------------------
//
//  Function:  OALContextSavePerfTimer
//
//  Saves the PerfTimer Registers
//
VOID
OALContextSavePerfTimer()
{	
	s_perfTimerContext.TIDR = INREG32(&g_pPerfTimer->TIDR);		// 0000
	s_perfTimerContext.TIOCP = INREG32(&g_pPerfTimer->TIOCP);	// 0010
	s_perfTimerContext.TISTAT = INREG32(&g_pPerfTimer->TISTAT);	// 0014
	s_perfTimerContext.TISR = INREG32(&g_pPerfTimer->TISR);		// 0018
	s_perfTimerContext.TIER = INREG32(&g_pPerfTimer->TIER);		// 001C
	s_perfTimerContext.TWER = INREG32(&g_pPerfTimer->TWER);		// 0020
	s_perfTimerContext.TCLR = INREG32(&g_pPerfTimer->TCLR);		// 0024
	s_perfTimerContext.TCRR = INREG32(&g_pPerfTimer->TCRR);		// 0028
	s_perfTimerContext.TLDR = INREG32(&g_pPerfTimer->TLDR);		// 002C
	s_perfTimerContext.TTGR = INREG32(&g_pPerfTimer->TTGR);		// 0030
	s_perfTimerContext.TWPS = INREG32(&g_pPerfTimer->TWPS);		// 0034
	s_perfTimerContext.TMAR = INREG32(&g_pPerfTimer->TMAR);		// 0038
	s_perfTimerContext.TCAR1 = INREG32(&g_pPerfTimer->TCAR1);	// 003C
	s_perfTimerContext.TSICR = INREG32(&g_pPerfTimer->TSICR);	// 0040
	s_perfTimerContext.TCAR2 = INREG32(&g_pPerfTimer->TCAR2);	// 0044
	s_perfTimerContext.TPIR = INREG32(&g_pPerfTimer->TPIR);		// 0x48
	s_perfTimerContext.TNIR = INREG32(&g_pPerfTimer->TNIR);		// 0x4C
	s_perfTimerContext.TCVR = INREG32(&g_pPerfTimer->TCVR);		// 0x50
	s_perfTimerContext.TOCR = INREG32(&g_pPerfTimer->TOCR);		// 0x54
	s_perfTimerContext.TOWR = INREG32(&g_pPerfTimer->TOWR);		// 0x58
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSaveMux
//
//  Saves pinmux
//
VOID
OALContextSaveMux()
{
    // Save all the PADCONF so the values are retained on wakeup from CORE OFF
    SETREG32(&g_pSysCtrlGenReg->CONTROL_PADCONF_OFF, STARTSAVE);
    while ((INREG32(&g_pSysCtrlGenReg->CONTROL_GENERAL_PURPOSE_STATUS) & SAVEDONE) == 0);

    g_ffContextSaveMask &= ~HAL_CONTEXTSAVE_PINMUX;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreGPIO
//
//  Restores the GPIO Registers
//
VOID
OALContextRestoreGPIO()
{
    UINT32 i;

    for(i=0; i< OMAP_GPIO_BANK_TO_RESTORE; i++)
	{
        OUTREG32(&s_rgGpioRegsAddr[i]->SYSCONFIG, s_rgGpioContext[i].SYSCONFIG );
        OUTREG32(&s_rgGpioRegsAddr[i]->CTRL, s_rgGpioContext[i].CTRL);
        OUTREG32(&s_rgGpioRegsAddr[i]->DATAOUT, s_rgGpioContext[i].DATAOUT);
        OUTREG32(&s_rgGpioRegsAddr[i]->OE, s_rgGpioContext[i].OE);
        OUTREG32(&s_rgGpioRegsAddr[i]->LEVELDETECT0, s_rgGpioContext[i].LEVELDETECT0);
        OUTREG32(&s_rgGpioRegsAddr[i]->LEVELDETECT1, s_rgGpioContext[i].LEVELDETECT1);
        OUTREG32(&s_rgGpioRegsAddr[i]->RISINGDETECT, s_rgGpioContext[i].RISINGDETECT);
        OUTREG32(&s_rgGpioRegsAddr[i]->FALLINGDETECT, s_rgGpioContext[i].FALLINGDETECT);

        // Note : Context restore of debouncing register is removed since no modules are 
        // using h/w debouncing. If debounce registers are restored, gpio fclk should be 
        // enabled and enough delay should be provided before disabling the gpio fclk
        // so that debouncing logic sync in h/w and Per domain acks idle request.

        OUTREG32(&s_rgGpioRegsAddr[i]->DEBOUNCENABLE, s_rgGpioContext[i].DEBOUNCENABLE);
        OUTREG32(&s_rgGpioRegsAddr[i]->DEBOUNCINGTIME, s_rgGpioContext[i].DEBOUNCINGTIME);

        OUTREG32(&s_rgGpioRegsAddr[i]->IRQENABLE1, s_rgGpioContext[i].IRQENABLE1);
        OUTREG32(&s_rgGpioRegsAddr[i]->WAKEUPENABLE, s_rgGpioContext[i].WAKEUPENABLE);
        OUTREG32(&s_rgGpioRegsAddr[i]->IRQENABLE2, s_rgGpioContext[i].IRQENABLE2);
	}
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreINTC
//
//  Restores the MPU IC Registers from shadow register
//
VOID
OALContextRestoreINTC()
{
    UINT32 i;

    OUTREG32(&g_pIntr->pICLRegs->INTC_SYSCONFIG, s_intcContext.INTC_SYSCONFIG);
    OUTREG32(&g_pIntr->pICLRegs->INTC_PROTECTION, s_intcContext.INTC_PROTECTION);
    OUTREG32(&g_pIntr->pICLRegs->INTC_IDLE, s_intcContext.INTC_IDLE);
    OUTREG32(&g_pIntr->pICLRegs->INTC_THRESHOLD, s_intcContext.INTC_THRESHOLD);

    for (i = 0; i < dimof(g_pIntr->pICLRegs->INTC_ILR); i++)
        OUTREG32(&g_pIntr->pICLRegs->INTC_ILR[i], s_intcContext.INTC_ILR[i]);

    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR_CLEAR0, ~s_intcContext.INTC_MIR0);
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR_CLEAR1, ~s_intcContext.INTC_MIR1);
    OUTREG32(&g_pIntr->pICLRegs->INTC_MIR_CLEAR2, ~s_intcContext.INTC_MIR2);
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestorePRCM
//
//  Restores the PRCM Registers from shadow register
//
VOID
OALContextRestorePRCM ()
{
    PrcmContextRestore();
    
    OUTREG32(&g_PrcmCm.pOMAP_OCP_SYSTEM_CM->CM_SYSCONFIG, s_ocpSysCmContext.CM_SYSCONFIG);

    OUTREG32(&g_PrcmCm.pOMAP_CORE_CM->CM_AUTOIDLE1_CORE, s_coreCmContext.CM_AUTOIDLE1_CORE);
    OUTREG32(&g_PrcmCm.pOMAP_CORE_CM->CM_AUTOIDLE2_CORE, s_coreCmContext.CM_AUTOIDLE2_CORE);
    OUTREG32(&g_PrcmCm.pOMAP_CORE_CM->CM_AUTOIDLE3_CORE, s_coreCmContext.CM_AUTOIDLE3_CORE);

    OUTREG32(&g_PrcmCm.pOMAP_WKUP_CM->CM_FCLKEN_WKUP, s_wkupCmContext.CM_FCLKEN_WKUP);
    OUTREG32(&g_PrcmCm.pOMAP_WKUP_CM->CM_ICLKEN_WKUP, s_wkupCmContext.CM_ICLKEN_WKUP);
    OUTREG32(&g_PrcmCm.pOMAP_WKUP_CM->CM_AUTOIDLE_WKUP, s_wkupCmContext.CM_AUTOIDLE_WKUP);

    OUTREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_AUTOIDLE2_PLL, DPLL_AUTOIDLE_DISABLED);
    OUTREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKSEL4_PLL, s_clkCtrlCmContext.CM_CLKSEL4_PLL);
    OUTREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKSEL5_PLL, s_clkCtrlCmContext.CM_CLKSEL5_PLL);
    OUTREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKEN2_PLL, s_clkCtrlCmContext.CM_CLKEN2_PLL);
    OUTREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_AUTOIDLE2_PLL, s_clkCtrlCmContext.CM_AUTOIDLE2_PLL);

    OUTREG32(&g_PrcmCm.pOMAP_GLOBAL_CM->CM_POLCTRL, s_globalCmContext.CM_POLCTRL);

    OUTREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_CLKOUT_CTRL, s_clkCtrlCmContext.CM_CLKOUT_CTRL);

    // Restore EMU CM Registers
    OUTREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSEL1_EMU,  s_emuCmContext.CM_CLKSEL1_EMU);
    OUTREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSTCTRL_EMU, s_emuCmContext.CM_CLKSTCTRL_EMU);
    OUTREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSEL2_EMU, s_emuCmContext.CM_CLKSEL2_EMU);
    OUTREG32(&g_PrcmCm.pOMAP_EMU_CM->CM_CLKSEL3_EMU, s_emuCmContext.CM_CLKSEL3_EMU);

    PrcmDomainSetClockState(POWERDOMAIN_EMULATION, CLOCKDOMAIN_EMULATION, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_NEON, CLOCKDOMAIN_NEON, CLKSTCTRL_AUTOMATIC);
    PrcmDomainSetClockState(POWERDOMAIN_IVA2, CLOCKDOMAIN_IVA2, CLKSTCTRL_AUTOMATIC);
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreGPMC
//
//  Restores the GPMC Registers from shadow register
//
VOID
OALContextRestoreGPMC()
{

    OUTREG32(&g_pGPMCRegs->GPMC_SYSCONFIG, s_gpmcContext.GPMC_SYSCONFIG);
    OUTREG32(&g_pGPMCRegs->GPMC_IRQENABLE, s_gpmcContext.GPMC_IRQENABLE);
    OUTREG32(&g_pGPMCRegs->GPMC_TIMEOUT_CONTROL, s_gpmcContext.GPMC_TIMEOUT_CONTROL);
    OUTREG32(&g_pGPMCRegs->GPMC_CONFIG, s_gpmcContext.GPMC_CONFIG);
    OUTREG32(&g_pGPMCRegs->GPMC_PREFETCH_CONFIG1, s_gpmcContext.GPMC_PREFETCH_CONFIG1);
    OUTREG32(&g_pGPMCRegs->GPMC_PREFETCH_CONFIG2, s_gpmcContext.GPMC_PREFETCH_CONFIG2);
    OUTREG32(&g_pGPMCRegs->GPMC_PREFETCH_CONTROL, s_gpmcContext.GPMC_PREFETCH_CONTROL);
    if(s_gpmcContext.GPMC_CONFIG7_0 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_0, s_gpmcContext.GPMC_CONFIG1_0);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_0, s_gpmcContext.GPMC_CONFIG2_0);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_0, s_gpmcContext.GPMC_CONFIG3_0);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_0, s_gpmcContext.GPMC_CONFIG4_0);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_0, s_gpmcContext.GPMC_CONFIG5_0);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_0, s_gpmcContext.GPMC_CONFIG6_0);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_0, s_gpmcContext.GPMC_CONFIG7_0);
    }
    if(s_gpmcContext.GPMC_CONFIG7_1 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_1, s_gpmcContext.GPMC_CONFIG1_1);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_1, s_gpmcContext.GPMC_CONFIG2_1);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_1, s_gpmcContext.GPMC_CONFIG3_1);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_1, s_gpmcContext.GPMC_CONFIG4_1);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_1, s_gpmcContext.GPMC_CONFIG5_1);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_1, s_gpmcContext.GPMC_CONFIG6_1);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_1, s_gpmcContext.GPMC_CONFIG7_1);
    }
    if(s_gpmcContext.GPMC_CONFIG7_2 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_2, s_gpmcContext.GPMC_CONFIG1_2);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_2, s_gpmcContext.GPMC_CONFIG2_2);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_2, s_gpmcContext.GPMC_CONFIG3_2);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_2, s_gpmcContext.GPMC_CONFIG4_2);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_2, s_gpmcContext.GPMC_CONFIG5_2);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_2, s_gpmcContext.GPMC_CONFIG6_2);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_2, s_gpmcContext.GPMC_CONFIG7_2);
    }
    if(s_gpmcContext.GPMC_CONFIG7_3 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_3, s_gpmcContext.GPMC_CONFIG1_3);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_3, s_gpmcContext.GPMC_CONFIG2_3);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_3, s_gpmcContext.GPMC_CONFIG3_3);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_3, s_gpmcContext.GPMC_CONFIG4_3);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_3, s_gpmcContext.GPMC_CONFIG5_3);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_3, s_gpmcContext.GPMC_CONFIG6_3);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_3, s_gpmcContext.GPMC_CONFIG7_3);
    }
    if(s_gpmcContext.GPMC_CONFIG7_4 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_4, s_gpmcContext.GPMC_CONFIG1_4);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_4, s_gpmcContext.GPMC_CONFIG2_4);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_4, s_gpmcContext.GPMC_CONFIG3_4);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_4, s_gpmcContext.GPMC_CONFIG4_4);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_4, s_gpmcContext.GPMC_CONFIG5_4);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_4, s_gpmcContext.GPMC_CONFIG6_4);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_4, s_gpmcContext.GPMC_CONFIG7_4);
    }
    if(s_gpmcContext.GPMC_CONFIG7_5 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_5, s_gpmcContext.GPMC_CONFIG1_5);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_5, s_gpmcContext.GPMC_CONFIG2_5);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_5, s_gpmcContext.GPMC_CONFIG3_5);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_5, s_gpmcContext.GPMC_CONFIG4_5);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_5, s_gpmcContext.GPMC_CONFIG5_5);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_5, s_gpmcContext.GPMC_CONFIG6_5);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_5, s_gpmcContext.GPMC_CONFIG7_5);
    }
    if(s_gpmcContext.GPMC_CONFIG7_6 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_6, s_gpmcContext.GPMC_CONFIG1_6);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_6, s_gpmcContext.GPMC_CONFIG2_6);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_6, s_gpmcContext.GPMC_CONFIG3_6);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_6, s_gpmcContext.GPMC_CONFIG4_6);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_6, s_gpmcContext.GPMC_CONFIG5_6);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_6, s_gpmcContext.GPMC_CONFIG6_6);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_6, s_gpmcContext.GPMC_CONFIG7_6);
    }
    if(s_gpmcContext.GPMC_CONFIG7_7 & GPMC_CSVALID)
    {
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG1_7, s_gpmcContext.GPMC_CONFIG1_7);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG2_7, s_gpmcContext.GPMC_CONFIG2_7);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG3_7, s_gpmcContext.GPMC_CONFIG3_7);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG4_7, s_gpmcContext.GPMC_CONFIG4_7);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG5_7, s_gpmcContext.GPMC_CONFIG5_7);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG6_7, s_gpmcContext.GPMC_CONFIG6_7);
        OUTREG32(&g_pGPMCRegs->GPMC_CONFIG7_7, s_gpmcContext.GPMC_CONFIG7_7);
    }
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreSCM
//
//  Restores the SCM Registers from shadow register
//
VOID
OALContextRestoreSCM ()
{
    OUTREG32(&s_pSyscIFContext->CONTROL_SYSCONFIG, s_syscIntContext.CONTROL_SYSCONFIG);

    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEVCONF0, s_syscGenContext.CONTROL_DEVCONF0);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MEM_DFTRW0, s_syscGenContext.CONTROL_MEM_DFTRW0);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MEM_DFTRW1, s_syscGenContext.CONTROL_MEM_DFTRW1);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_0, s_syscGenContext.CONTROL_MSUSPENDMUX_0);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_1, s_syscGenContext.CONTROL_MSUSPENDMUX_1);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_2, s_syscGenContext.CONTROL_MSUSPENDMUX_2);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_3, s_syscGenContext.CONTROL_MSUSPENDMUX_3);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_4, s_syscGenContext.CONTROL_MSUSPENDMUX_4);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_MSUSPENDMUX_5, s_syscGenContext.CONTROL_MSUSPENDMUX_5);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_SEC_CTRL, s_syscGenContext.CONTROL_SEC_CTRL);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEVCONF1, s_syscGenContext.CONTROL_DEVCONF1);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_CSIRXFE, s_syscGenContext.CONTROL_CSIRXFE);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_IVA2_BOOTADDR, s_syscGenContext.CONTROL_IVA2_BOOTADDR);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_IVA2_BOOTMOD, s_syscGenContext.CONTROL_IVA2_BOOTMOD);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_0, s_syscGenContext.CONTROL_DEBOBS_0);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_1, s_syscGenContext.CONTROL_DEBOBS_1);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_2, s_syscGenContext.CONTROL_DEBOBS_2);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_3, s_syscGenContext.CONTROL_DEBOBS_3);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_4, s_syscGenContext.CONTROL_DEBOBS_4);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_5, s_syscGenContext.CONTROL_DEBOBS_5);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_6, s_syscGenContext.CONTROL_DEBOBS_6);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_7, s_syscGenContext.CONTROL_DEBOBS_7);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DEBOBS_8, s_syscGenContext.CONTROL_DEBOBS_8);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_PROG_IO0, s_syscGenContext.CONTROL_PROG_IO0);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_PROG_IO1, s_syscGenContext.CONTROL_PROG_IO1);

    OUTREG32(&g_pSysCtrlGenReg->CONTROL_DSS_DPLL_SPREADING, s_syscGenContext.CONTROL_DSS_DPLL_SPREADING);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_CORE_DPLL_SPREADING, s_syscGenContext.CONTROL_CORE_DPLL_SPREADING);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_PER_DPLL_SPREADING, s_syscGenContext.CONTROL_PER_DPLL_SPREADING);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_USBHOST_DPLL_SPREADING, s_syscGenContext.CONTROL_USBHOST_DPLL_SPREADING);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_PBIAS_LITE, s_syscGenContext.CONTROL_PBIAS_LITE);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_TEMP_SENSOR, s_syscGenContext.CONTROL_TEMP_SENSOR);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_SRAMLDO4, s_syscGenContext.CONTROL_SRAMLDO4);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_SRAMLDO5, s_syscGenContext.CONTROL_SRAMLDO5);
    OUTREG32(&g_pSysCtrlGenReg->CONTROL_CSI, s_syscGenContext.CONTROL_CSI);
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreDMA
//
//  Restores the DMA Registers from shadow register
//
VOID
OALContextRestoreDMA()
{
    OUTREG32(&s_pDmaController->DMA4_GCR, s_dmaController.DMA4_GCR);
    OUTREG32(&s_pDmaController->DMA4_OCP_SYSCONFIG, s_dmaController.DMA4_OCP_SYSCONFIG);
    OUTREG32(&s_pDmaController->DMA4_IRQENABLE_L0, s_dmaController.DMA4_IRQENABLE_L0);
    OUTREG32(&s_pDmaController->DMA4_IRQENABLE_L1, s_dmaController.DMA4_IRQENABLE_L1);
    OUTREG32(&s_pDmaController->DMA4_IRQENABLE_L2, s_dmaController.DMA4_IRQENABLE_L2);
    OUTREG32(&s_pDmaController->DMA4_IRQENABLE_L3, s_dmaController.DMA4_IRQENABLE_L3);
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreSMS
//  
//  Restores the SMS Registers from shadow register
//
VOID
OALContextRestoreSMS()
{
    OUTREG32(&g_pSMSRegs->SMS_SYSCONFIG, s_smsContext.SMS_SYSCONFIG);
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreVRFB
//
//  Restores the VRFB Registers from shadow register
//
VOID
OALContextRestoreVRFB()
{
    int i;

    for (i = 0; i < VRFB_ROTATION_CONTEXTS; ++i)
        {
        OUTREG32(&g_pVRFBRegs->aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_CONTROL,
            s_vrfbContext.aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_CONTROL
            );

        OUTREG32(&g_pVRFBRegs->aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_SIZE,
            s_vrfbContext.aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_SIZE
            );

        OUTREG32(&g_pVRFBRegs->aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_PHYSICAL_BA,
            s_vrfbContext.aVRFB_SMS_ROT_CTRL[i].VRFB_SMS_ROT_PHYSICAL_BA
            );
        }
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestorePerfTimer
//
//  Restores the PerfTimer Registers from shadow register
//
VOID
OALContextRestorePerfTimer()
{	
	OUTREG32(&g_pPerfTimer->TIDR,s_perfTimerContext.TIDR);		// 0000
	OUTREG32(&g_pPerfTimer->TIOCP,s_perfTimerContext.TIOCP);	// 0010
	OUTREG32(&g_pPerfTimer->TISTAT,s_perfTimerContext.TISTAT);	// 0014
	OUTREG32(&g_pPerfTimer->TISR,s_perfTimerContext.TISR);		// 0018
	OUTREG32(&g_pPerfTimer->TIER,s_perfTimerContext.TIER);		// 001C
	OUTREG32(&g_pPerfTimer->TWER,s_perfTimerContext.TWER);		// 0020
	OUTREG32(&g_pPerfTimer->TCLR,s_perfTimerContext.TCLR);		// 0024
	OUTREG32(&g_pPerfTimer->TCRR,s_perfTimerContext.TCRR);		// 0028
	OUTREG32(&g_pPerfTimer->TLDR,s_perfTimerContext.TLDR);		// 002C
	OUTREG32(&g_pPerfTimer->TTGR,s_perfTimerContext.TTGR);		// 0030
	OUTREG32(&g_pPerfTimer->TWPS,s_perfTimerContext.TWPS);		// 0034
	OUTREG32(&g_pPerfTimer->TMAR,s_perfTimerContext.TMAR);		// 0038
	OUTREG32(&g_pPerfTimer->TCAR1,s_perfTimerContext.TCAR1);	// 003C
	OUTREG32(&g_pPerfTimer->TSICR,s_perfTimerContext.TSICR);	// 0040
	OUTREG32(&g_pPerfTimer->TCAR2,s_perfTimerContext.TCAR2);	// 0044
	OUTREG32(&g_pPerfTimer->TPIR,s_perfTimerContext.TPIR);		// 0x48
	OUTREG32(&g_pPerfTimer->TNIR,s_perfTimerContext.TNIR);		// 0x4C
	OUTREG32(&g_pPerfTimer->TCVR,s_perfTimerContext.TCVR);		// 0x50
	OUTREG32(&g_pPerfTimer->TOCR,s_perfTimerContext.TOCR);		// 0x54
	OUTREG32(&g_pPerfTimer->TOWR,s_perfTimerContext.TOWR);		// 0x58	
}


//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestoreInit
//
//  Initialize the context Restore.
//
VOID
OALContextRestoreInit()
{
    // Initalize
    s_pSyscIFContext = OALPAtoUA(OMAP_SYSC_INTERFACE_REGS_PA);
    s_pDmaController = OALPAtoUA(OMAP_SDMA_REGS_PA);

    s_rgGpioRegsAddr[0] = OALPAtoUA(OMAP_GPIO1_REGS_PA);
    s_rgGpioRegsAddr[1] = OALPAtoUA(OMAP_GPIO2_REGS_PA);
    s_rgGpioRegsAddr[2] = OALPAtoUA(OMAP_GPIO3_REGS_PA);
    s_rgGpioRegsAddr[3] = OALPAtoUA(OMAP_GPIO4_REGS_PA);
    s_rgGpioRegsAddr[4] = OALPAtoUA(OMAP_GPIO5_REGS_PA);
    s_rgGpioRegsAddr[5] = OALPAtoUA(OMAP_GPIO6_REGS_PA);
    
    // Configure OMAP to send Sys OFF for OFF mode and I2C Command for RET. - Page 622
    OUTREG32(&g_PrcmPrm.pOMAP_GLOBAL_PRM->PRM_VOLTCTRL, 
        (AUTO_OFF_ENABLED|SEL_OFF_SIGNALLINE|AUTO_RET_DISABLED));

    // Configure the OFFMODE values for SYS_NIRQ to get wakeup from T2
    OUTREG16(&g_pSyscPadConfsRegs->CONTROL_PADCONF_SYS_NIRQ,
                       (OFF_WAKE_ENABLE | OFF_INPUT_PULL_UP | INPUT_ENABLE |
                        PULL_UP | MUX_MODE_4));    	

}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextUpdateDirtyRegister
//
void
OALContextUpdateDirtyRegister(
    UINT32  ffRegisterSet
    )
{
    g_ffContextSaveMask |= ffRegisterSet;
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextUpdateCleanRegister
//
void
OALContextUpdateCleanRegister(
    UINT32  ffRegisterSet
    )
{
    switch (ffRegisterSet)
        {
        case HAL_CONTEXTSAVE_GPIO:
            OALContextSaveGPIO();
            break;

        case HAL_CONTEXTSAVE_SCM:
            OALContextSaveSCM();
            break;

        case HAL_CONTEXTSAVE_GPMC:
            OALContextSaveGPMC();
            break;

        case HAL_CONTEXTSAVE_DMA:
            OALContextSaveDMA();
            break;

        case HAL_CONTEXTSAVE_PINMUX:
            OALContextSaveDMA();
            break;

        case HAL_CONTEXTSAVE_SMS:
            OALContextSaveSMS();
            break;

        case HAL_CONTEXTSAVE_VRFB:
            OALContextSaveVRFB();
            break;
}
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextSave
//  
//  Saves the SCM, CM, GPMC context to shadow registers
//
BOOL
OALContextSave()
{
    BOOL    rc = TRUE;
    
    // save dirty registers
    if (g_ffContextSaveMask != 0)
        {
           
        //if ((g_ffContextSaveMask & HAL_CONTEXTSAVE_GPIO) != 0)
            {
            OALContextSaveGPIO();
            }

        if ((g_ffContextSaveMask & HAL_CONTEXTSAVE_SCM) != 0)
            {
            OALContextSaveSCM ();
            }
        // always save GPMC before suspend
        //if ((g_ffContextSaveMask & HAL_CONTEXTSAVE_GPMC) != 0)
            {
            OALContextSaveGPMC();
            }

        if ((g_ffContextSaveMask & HAL_CONTEXTSAVE_DMA) != 0)
            {
            OALContextSaveDMA();
            }

        if ((g_ffContextSaveMask & HAL_CONTEXTSAVE_PINMUX) != 0)
            {
            OALContextSaveMux();
            }

        if ((g_ffContextSaveMask & HAL_CONTEXTSAVE_SMS) != 0)
            {
            OALContextSaveSMS();
            }

        if ((g_ffContextSaveMask & HAL_CONTEXTSAVE_VRFB) != 0)
            {
            OALContextSaveVRFB();
            }
        }	

    // Save the MPU Interrupt controller MIR registers
    s_intcContext.INTC_MIR0 = INREG32(&g_pIntr->pICLRegs->INTC_MIR0);
    s_intcContext.INTC_MIR1 = INREG32(&g_pIntr->pICLRegs->INTC_MIR1);
    s_intcContext.INTC_MIR2 = INREG32(&g_pIntr->pICLRegs->INTC_MIR2);

    // need to clear all gpio interface clocks for per to enter OFF
    CLRREG32(&g_PrcmCm.pOMAP_PER_CM->CM_ICLKEN_PER, 0x3E000);
	
    // need to clear all gpio functional clocks for per to enter OFF
    CLRREG32(&g_PrcmCm.pOMAP_PER_CM->CM_FCLKEN_PER, 0x3E000);

    // Errata 2.23. GPIO is driving random values when device is coming 
    // back from OFF mode.
    // Set PER domain sleep dependency with Wkup to avoid GPIO glitch
    SETREG32(&g_PrcmPrm.pOMAP_PER_PRM->PM_WKDEP_PER, SLEEPDEP_EN_WKUP);

    s_bCoreOffModeSet = TRUE;

    return rc;    
}

//-----------------------------------------------------------------------------
//
//  Function:  OALContextRestore
//
//  Restores the SCM, CM, GPMC context from shadow registers
//
VOID
OALContextRestore(
    UINT32 prevMpuState,
    UINT32 prevCoreState,
    UINT32 prevPerState
    )
{
    if(((prevMpuState & POWERSTATE_MASK) == POWERSTATE_OFF) ||
      (((prevMpuState & POWERSTATE_MASK) == POWERSTATE_RETENTION) &&
       ((prevMpuState & LOGICRETSTATE_MASK) == LOGICRETSTATE_LOGICOFF_DOMAINRET)))
	{
	    // ROMCODE restores the CM_AUTOIDLE_PLL register with value in scratchpad when
	    // MPU is OFF. Since scratch pad value is set to 0 to reduce the wakeup
	    // latency for off mode, restore CM_AUTOIDLE_PLL register with actual value.
	    OUTREG32(&g_PrcmCm.pOMAP_CLOCK_CONTROL_CM->CM_AUTOIDLE_PLL,
	                                   s_clkCtrlCmContext.CM_AUTOIDLE_PLL);
	}
	/* WorkAround for EHCI/USBHost: Somehow touching the ICLKEN_WKUP register is causing USBHost 
	     power domain to be up reliably */
    if(((prevCoreState & POWERSTATE_MASK) == POWERSTATE_OFF) ||
      (((prevCoreState & POWERSTATE_MASK) == POWERSTATE_RETENTION) &&
       ((prevCoreState & LOGICRETSTATE_MASK) == LOGICRETSTATE_LOGICOFF_DOMAINRET)))
        {
        	//UNDONE: should we need to do this all the time ?
            PrcmDomainClearReset();
        
            // Restore the PRCM Registers from shadow variable
            OALContextRestorePRCM();
        
            // Handle CORE OFF case
            if((prevCoreState & POWERSTATE_MASK) == POWERSTATE_OFF)
        	{
                // Restore the SMS Registers from shadow variable
                OALContextRestoreSMS ();
        
                // Restore the VRFB Registers from shadow variable
                OALContextRestoreVRFB ();
        
                // Restore the GPMC Registers from shadow variable
                OALContextRestoreGPMC ();
        
                // Restore the SCM Registers from shadow variable
                OALContextRestoreSCM ();
        
                // Restore the MPU IC Registers from shadow variable
                OALContextRestoreINTC ();
        
                // Restore DMA context
                OALContextRestoreDMA ();		
        
                // restore CM registers for all power domains
                PrcmRestoreDomain(POWERDOMAIN_PERIPHERAL);
                PrcmRestoreDomain(POWERDOMAIN_DSS);
                PrcmRestoreDomain(POWERDOMAIN_CAMERA);
                PrcmRestoreDomain(POWERDOMAIN_USBHOST);
                PrcmRestoreDomain(POWERDOMAIN_SGX);
                PrcmRestoreDomain(POWERDOMAIN_NEON);
                PrcmRestoreDomain(POWERDOMAIN_EMULATION);
                PrcmRestoreDomain(POWERDOMAIN_IVA2);
				
		  // Workaround for 3730 VDD1/VDD2 drop to 0V issue, need to be re-visited 
	         if(g_dwCpuFamily == CPU_FAMILY_DM37XX)
                    PrcmSetIVA2OffMode();
        	}
        }

    if (s_bCoreOffModeSet)
	{
        // enable GPIO ICLKs
        SETREG32(&g_PrcmCm.pOMAP_PER_CM->CM_AUTOIDLE_PER, 0x3E000);
        SETREG32(&g_PrcmCm.pOMAP_PER_CM->CM_ICLKEN_PER, 0x3E000);
		SETREG32(&g_PrcmCm.pOMAP_PER_CM->CM_FCLKEN_PER, 0x3E000);

        // Errata 2.23. GPIO is driving random values when device is coming 
        // back from OFF mode.
        // Set PER domain sleep dependency with Wkup to avoid GPIO glitch
        CLRREG32(&g_PrcmPrm.pOMAP_PER_PRM->PM_WKDEP_PER, SLEEPDEP_EN_WKUP);
        
        s_bCoreOffModeSet = FALSE;
	}

    if(((prevPerState & POWERSTATE_MASK) == POWERSTATE_OFF) ||
      (((prevPerState & POWERSTATE_MASK) == POWERSTATE_RETENTION) &&
       ((prevPerState & LOGICRETSTATE_MASK) == LOGICRETSTATE_LOGICOFF_DOMAINRET)))
	{
        // restore GPIO registers
        OALContextRestoreGPIO();
	}
 }

//-----------------------------------------------------------------------------
//
//  Function:  OutShadowReg32
//
// Interface to update the GPMC, SDRC, MPUIC, CM register and corresponding
// shadow registers
//
VOID
OutShadowReg32(
    UINT32 deviceGroup,
    UINT32 offset,
    UINT32 value
    )
{
    UINT32 *pShadowRegBase = NULL;

    // change the byte offset to 4 bytes offset
    offset = offset/4;

    switch(deviceGroup)
        {
        case OMAP_PRCM_OCP_SYSTEM_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_ocpSysCmContext;
            pShadowRegBase += offset;

            // Update the register content
            OUTREG32(((UINT32*)g_PrcmCm.pOMAP_OCP_SYSTEM_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = value;
            break;

        case OMAP_PRCM_WKUP_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_wkupCmContext;
            pShadowRegBase += offset;

            // Update the register content
            OUTREG32(((UINT32*)g_PrcmCm.pOMAP_WKUP_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = value;
            break;

        case OMAP_PRCM_CORE_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_coreCmContext;
            pShadowRegBase += offset;

            // Update the register content
            OUTREG32(((UINT32*)g_PrcmCm.pOMAP_CORE_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = value;
            break;


        case OMAP_PRCM_CLOCK_CONTROL_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_clkCtrlCmContext;
            pShadowRegBase += offset;

            // Update the register content
            OUTREG32(((UINT32*)g_PrcmCm.pOMAP_CLOCK_CONTROL_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = value;
            break;

        case OMAP_PRCM_GLOBAL_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_globalCmContext;
            pShadowRegBase += offset;

            // Update the register content
            OUTREG32(((UINT32*)g_PrcmCm.pOMAP_GLOBAL_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = value;
            break;


        default:
            break;
        }
}

//-----------------------------------------------------------------------------
//
//  Function:  SetShadowReg32
//
// Interface to update the GPMC, SDRC, MPUIC, CM register and corresponding
// shadow registers
//
VOID
SetShadowReg32(
    UINT32 deviceGroup,
    UINT32 offset,
    UINT32 value
    )
{
    UINT32 *pShadowRegBase = NULL;

    // change the byte offset to 4 bytes offset
    offset = offset/4;

    switch(deviceGroup)
        {
        case OMAP_PRCM_OCP_SYSTEM_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_ocpSysCmContext;
            pShadowRegBase += offset;

            // Update the register content
            SETREG32(((UINT32*)g_PrcmCm.pOMAP_OCP_SYSTEM_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = (INREG32((UINT32*)g_PrcmCm.pOMAP_OCP_SYSTEM_CM+offset)|value);
            break;

        case OMAP_PRCM_WKUP_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_wkupCmContext;
            pShadowRegBase += offset;

            // Update the register content
            SETREG32(((UINT32*)g_PrcmCm.pOMAP_WKUP_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = (INREG32((UINT32*)g_PrcmCm.pOMAP_WKUP_CM+offset)|value);
            break;

        case OMAP_PRCM_CORE_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_coreCmContext;
            pShadowRegBase += offset;

            // Update the register content
            SETREG32(((UINT32*)g_PrcmCm.pOMAP_CORE_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = (INREG32((UINT32*)g_PrcmCm.pOMAP_CORE_CM+offset)|value);
            break;


        case OMAP_PRCM_CLOCK_CONTROL_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_clkCtrlCmContext;
            pShadowRegBase += offset;

            // Update the register content
            SETREG32(((UINT32*)g_PrcmCm.pOMAP_CLOCK_CONTROL_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = (INREG32((UINT32*)g_PrcmCm.pOMAP_CLOCK_CONTROL_CM+offset)|value);
            break;

        case OMAP_PRCM_GLOBAL_CM_REGS_PA:
            pShadowRegBase = (UINT32 *)&s_globalCmContext;
            pShadowRegBase += offset;

            // Update the register content
            SETREG32(((UINT32*)g_PrcmCm.pOMAP_GLOBAL_CM+offset), value);

            // Update the shadow content.
            *pShadowRegBase = (INREG32((UINT32*)g_PrcmCm.pOMAP_GLOBAL_CM+offset)|value);
            break;

        default:
            break;
        }
}
//-----------------------------------------------------------------------------

