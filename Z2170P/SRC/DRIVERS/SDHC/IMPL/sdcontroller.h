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
//  File: sdcontroller.h
//
//

#ifndef __SDCONTROLLER_H
#define __SDCONTROLLER_H

#include "sdhc.h"
#include "omap3530_base_regs.h"
#include "omap3530_config.h"
#include "twl.h"
#include "bsp_cfg.h"

#define MMC_NO_GPIO_CARD_WP            (-1)     // no GPIO write-protect pin
#define MMC_NO_GPIO_CARD_WP_STATE      (-1)     // no GPIO write-protect pin state
#define MMC_NO_GPIO_CARD_DETECT        (-1)     // no GPIO card detect pin
#define MMC_NO_GPIO_CARD_DETECT_STATE  (-1)     // no GPIO card detect pin state

class CSDIOController : public CSDIOControllerBase
{
public:
    CSDIOController();
    ~CSDIOController();
       
protected:
    BOOL IsWriteProtected();
    BOOL SDCardDetect();
    DWORD SDHCCardDetectIstThreadImpl();
    virtual BOOL InitializeHardware();
    virtual void DeinitializeHardware();
    virtual VOID TurnCardPowerOn();
    virtual VOID TurnCardPowerOff();
    virtual VOID PreparePowerChange(CEDEVICE_POWER_STATE curPowerState, BOOL bInPowerHandler);
    virtual VOID PostPowerChange(CEDEVICE_POWER_STATE curPowerState, BOOL bInPowerHandler);

    BOOL InitializeCardDetect();
    BOOL DeInitializeCardDetect();
    BOOL InitializeWPDetect(void );
    BOOL DeInitializeWPDetect(void);
    
    DWORD    m_dwCDIntrId;
    DWORD    m_dwCDSysintr;
};

#endif // __SDCONTROLLER_H

