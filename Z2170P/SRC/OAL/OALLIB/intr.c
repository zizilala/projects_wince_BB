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
//  File:  intr.c
//
//  Interrupt BSP callback functions. OMAP SoC has one external interrupt
//  pin. In general solution it can be used for cascade interrupt controller.
//  However in most cases it will be used as simple interrupt input. To avoid
//  unnecessary penalty not all BSPIntrXXXX functions are called by default
//  interrupt module implementation.
//
//
#include "bsp.h"
#include "bsp_cfg.h"
#include "sdk_gpio.h"

#include "oalex.h"
#include "oal_prcm.h"
#include "oal_sr.h"
#include "omap3530_irq.h"

//------------------------------------------------------------------------------
//
//  External: g_oalPrcmIrq
//
//  This variable is defined in interrupt module and it is used in interrupt
//  handler to distinguish prcm interrupt.
//
extern UINT32                   g_oalPrcmIrq;

//------------------------------------------------------------------------------
//
//  External: g_oalSmartReflex1
//
//  This variable is defined in interrupt module and it is used in interrupt
//  handler to distinguish SmartReflex1 interrupt.
//
extern UINT32                   g_oalSmartReflex1;

//------------------------------------------------------------------------------
//
//  External: g_oalSmartReflex2
//
//  This variable is defined in interrupt module and it is used in interrupt
//  handler to distinguish SmartReflex2 interrupt.
//
extern UINT32                   g_oalSmartReflex2;

//------------------------------------------------------------------------------
//
//  Function:  RegisterPrcmInterruptHandler
//
//  This function registers with the interrupt handler for the prcm interrupt.
//
// Power, Reset, and Clock Management
BOOL
RegisterPrcmInterruptHandler()
{
    UINT32 sysIntr;
    BOOL rc = FALSE;
    
    OALMSG(OAL_INTR, (L"+RegisterPrcmInterruptHandler()\r\n"));
    // enable PRCM iterrupts
    g_oalPrcmIrq = IRQ_PRCM_MPU;

    // clear status
    PrcmInterruptClearStatus(0xFFFFFFFF);

    // Request SYSINTR for timer IRQ, it is done to reserve it...
    sysIntr = OALIntrRequestSysIntr(1, &g_oalPrcmIrq, OAL_INTR_FORCE_STATIC);

    // Enable System Tick interrupt
    if (!OEMInterruptEnable(sysIntr, NULL, 0))
        {
        OALMSG(OAL_ERROR, (
            L"ERROR: RegisterPrcmInterruptHandler: "
            L"Interrupt enable for PRCM failed"
            ));
        goto cleanUp;
        }

    // enable some prcm interrupts, others should be enabled as needed
    PrcmInterruptEnable(
        //PRM_IRQENABLE_WKUP_EN |
        PRM_IRQENABLE_MPU_DPLL_RECAL_EN |
        PRM_IRQENABLE_CORE_DPLL_RECAL_EN |
        //PRM_IRQENABLE_PERIPH_DPLL_RECAL_EN |
        //PRM_IRQENABLE_IVA2_DPLL_RECAL_EN_MPU |
        //PRM_IRQENABLE_SND_PERIPH_DPLL_RECAL_EN |
        PRM_IRQENABLE_VP1_OPPCHANGEDONE_EN |
        PRM_IRQENABLE_VP2_OPPCHANGEDONE_EN |
        PRM_IRQENABLE_VP1_NOSMPSACK_EN |
        PRM_IRQENABLE_VP2_NOSMPSACK_EN |
        PRM_IRQENABLE_VC_SAERR_EN |
        PRM_IRQENABLE_VC_RAERR_EN |
        PRM_IRQENABLE_VC_TIMEOUTERR_EN |
        PRM_IRQENABLE_IO_EN ,
        TRUE
        );
   
    rc = TRUE;

cleanUp:
	OALMSG(OAL_INTR, (L"-RegisterPrcmInterruptHandler(rc = %d)\r\n", rc));
    return rc;
}

//------------------------------------------------------------------------------
//
//  Function:  BSPIntrInit
//
//  This function is called from OALIntrInit to initialize on-board secondary
//  interrupt controller if exists. As long as GPIO interrupt edge registers
//  are initialized in startup.s code function is stub only.
//
BOOL
BSPIntrInit()
{    
    
    // Associate the External LAN Irq with a fixed sysintr.
    // The reason is that the LAN irq is greater than 255 and that is not 
    // suported by NDIS
    //OALIntrStaticTranslate(SYSINTR_LAN9115, BSPGetGpioIrq(LAN9115_IRQ_GPIO));

    return RegisterPrcmInterruptHandler();
}


//------------------------------------------------------------------------------
//
//  Function:  BSPIntrRequestIrqs
//
//  This function is called from OALIntrRequestIrq to obtain IRQ for on-board
//  devices if exists.
//
BOOL
BSPIntrRequestIrqs(
    DEVICE_LOCATION *pDevLoc, 
    UINT32 *pCount, 
    UINT32 *pIrq
    )
{
    BOOL rc = FALSE;
    UNREFERENCED_PARAMETER(pCount);
	UNREFERENCED_PARAMETER(pIrq);
    OALMSG(OAL_INTR&&OAL_FUNC, (
        L"+BSPIntrRequestIrq(0x%08x->%d/%d/0x%08x/%d, 0x%08x)\r\n", pDevLoc, 
        pDevLoc->IfcType, pDevLoc->BusNumber, pDevLoc->LogicalLoc,
        pDevLoc->Pin, pIrq
        ));

    switch (pDevLoc->IfcType)
        {
        case Internal:
            break;
        }

    OALMSG(OAL_INTR&&OAL_FUNC, (L"-BSPIntrRequestIrq(rc = %d)\r\n", rc));

    return rc;
}

//------------------------------------------------------------------------------

