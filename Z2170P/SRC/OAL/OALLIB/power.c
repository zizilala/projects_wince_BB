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

#include "bsp.h"
#include "oal_sr.h"
#include "oal_clock.h"
#include "bsp_cfg.h"
#include "oalex.h"
#include "ceddkex.h"
#include "oal_gptimer.h"

extern void OEMDeinitDebugSerial();
extern void OEMInitDebugSerial();

//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmPrm
//
//  Reference to all PRCM-PRM registers. Initialized in PrcmInit
//
extern OMAP_PRCM_PRM               g_PrcmPrm;

//-----------------------------------------------------------------------------
//
//  Global:  g_PrcmCm
//
//  Reference to all PRCM-CM registers. Initialized in PrcmInit
//
extern OMAP_PRCM_CM                g_PrcmCm;

//------------------------------------------------------------------------------
//
//  External:  g_pSysCtrlGenReg
//
//  reference to system control general register set
//
extern OMAP_SYSC_GENERAL_REGS     *g_pSysCtrlGenReg;

//-----------------------------------------------------------------------------
//
//  External:  g_pTimerRegs
//
//  References the GPTimer1 registers.  Initialized in OALTimerInit().
//
//extern OMAP_GPTIMER_REGS          *g_pTimerRegs;

//-----------------------------------------------------------------------------
//
//  Static :  s_mpuPowerState
//
//  Saves the mpu power state to restore on wakeup
//  
static UINT32 s_mpuPowerState = 0;

//-----------------------------------------------------------------------------
//
//  Static :  s_enableSmartReflex
//
//  Saves smartreflex states 
//  
static BOOL s_enableSmartReflex1;
static BOOL s_enableSmartReflex2;

//-----------------------------------------------------------------------------
// prototypes
//
extern BOOL IsSmartReflexMonitoringEnabled(UINT channel);
extern BOOL SmartReflex_EnableMonitor(UINT channel, BOOL bEnable);
    
//------------------------------------------------------------------------------
// WARNING: This function is called from OEMPowerOff - no system calls, critical 
// sections, OALMSG, etc., may be used by this function or any function that it calls.
//------------------------------------------------------------------------------
VOID BSPPowerOff()
{
    // Disable Smartreflex if enabled.
    if (IsSmartReflexMonitoringEnabled(kSmartReflex_Channel1))
	{
        s_enableSmartReflex1 = TRUE;
        SmartReflex_EnableMonitor(kSmartReflex_Channel1, FALSE);
	}
    else
	{
        s_enableSmartReflex1 = FALSE;
	}

    if (IsSmartReflexMonitoringEnabled(kSmartReflex_Channel2))
	{
        s_enableSmartReflex2 = TRUE;
        SmartReflex_EnableMonitor(kSmartReflex_Channel2, FALSE);
	}
    else
	{
        s_enableSmartReflex2 = FALSE;
	}

    // clear wake-up enable capabilities for gptimer1
    CLRREG32(&g_PrcmPrm.pOMAP_WKUP_PRM->PM_WKEN_WKUP, CM_CLKEN_GPT1);

    // stop GPTIMER1
    //OUTREG32(&g_pTimerRegs->TCLR, INREG32(&g_pTimerRegs->TCLR) & ~(GPTIMER_TCLR_ST));
    OALTimerStop();
    
    if (g_oalRetailMsgEnable)
	{
        OEMDeinitDebugSerial();
        EnableDeviceClocks(BSPGetDebugUARTConfig()->dev,FALSE);
	}
}

//------------------------------------------------------------------------------
// WARNING: This function is called from OEMPowerOff - no system calls, critical 
// sections, OALMSG, etc., may be used by this function or any function that it calls.
//------------------------------------------------------------------------------
VOID BSPPowerOn( )
{
    // reset wake-up enable capabilities for gptimer1
    SETREG32(&g_PrcmPrm.pOMAP_WKUP_PRM->PM_WKEN_WKUP, CM_CLKEN_GPT1);

    if (s_enableSmartReflex1)
        {
        SmartReflex_EnableMonitor(kSmartReflex_Channel1, TRUE);
        }

    if (s_enableSmartReflex2)
        {
        SmartReflex_EnableMonitor(kSmartReflex_Channel2, TRUE);
        }

    if (g_oalRetailMsgEnable)
	    {
        EnableDeviceClocks(BSPGetDebugUARTConfig()->dev,TRUE);
	    OEMInitDebugSerial();
    	}

    g_ResumeRTC = TRUE;
}

//------------------------------------------------------------------------------
