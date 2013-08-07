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
//  File:  bsp_opp_map.h
//

#ifndef __BSP_OPP_MAP_H
#define __BSP_OPP_MAP_H

#include "Bsp_cfg.h"

#pragma warning (push)
#pragma warning (disable:4200)
//-----------------------------------------------------------------------------
typedef struct {
    SmartReflexSensorData_t     smartReflexInfo;
    VoltageProcessorSetting_t   vpInfo;
    int                         dpllCount;
    DpllFrequencySetting_t      rgFrequencySettings[];
} VddOppSetting_t;
#pragma warning (pop)

//-----------------------------------------------------------------------------
typedef struct {
    BOOL                        bInitialized;
    BOOL                        bEnabled;
    CRITICAL_SECTION            cs;
} SmartReflexStateInfo_t;

#define VDD1_OPP_COUNT_35xx      OMAP35x_OPP_NUM
#define VDD1_OPP_COUNT_37xx      OMAP37x_OPP_NUM
#define VDD2_OPP_COUNT      2

#define INITIAL_VDD2_OPP            (kOpp2)
#define MAX_VDD2_OPP                  (kOpp2)
#define MAX_VDD1_OPP_37xx        (kOpp4)
#define MAX_VDD1_OPP_35xx        (kOpp6)

Dvfs_OperatingPoint_e Vdd1_init_val_35xx[VDD1_OPP_COUNT_35xx + 1]=
{
    kOppUndefined,
    kOpp1,	
    kOpp2,		
    kOpp3,		
    kOpp4,		
    kOpp5,		
    kOpp6		    
};
Dvfs_OperatingPoint_e Vdd1_init_val_37xx[VDD1_OPP_COUNT_37xx + 1]=
{
    kOppUndefined,
    kOpp1,	
    kOpp2,		
    kOpp3,	
    kOpp4	
};


//-----------------------------------------------------------------------------

// For TPS659XX use the following equations to calculate the value from volt and vice versa.
// val = (volt - .6)/.0125, volt = val * .0125 + .6

// (just a placeholder)

//-----------------------------------------------------------------------------

// For TPS659XX use the following equations to calculate the value from volt and vice versa.
// val = (volt - .6)/.0125, volt = val * .0125 + .6

// (just a placeholder)
static VddOppSetting_t vdd1Opp0Info = {
    {
        kSmartReflex_Channel1, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor1, 0, 0                        // voltage processor info
    }, 
        2,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 0,  (UINT)-1, 0,  0, 1                    // dpll1 (MPU)
       }, {
            kDPLL2, 0,  (UINT)-1, 0,  0, 1                    // dpll2 (DSP)
       }
    }
};

// MPU[125Mhz @ 0.975V], IVA2[ 90Mhz @ 0.975V]
static VddOppSetting_t vdd1Opp1Info = {
    {
        kSmartReflex_Channel1, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor1, 0x1E,   0x1E                // voltage processor info
    }, 
        2,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 125, 250, 12, 7, 4                  // dpll1 (MPU)
       }, {
            kDPLL2,  90, 180, 12, 7, 4                  // dpll2 (DSP)
       }
    }
};

// MPU[250Mhz @ 1.00V], IVA2[180Mhz @ 1.05V]
static VddOppSetting_t vdd1Opp2Info = {
    {
        kSmartReflex_Channel1, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor1, 0x24,   0x24                // voltage processor info
    }, 
        2,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 250, 250, 12, 7, 2                  // dpll1 (MPU)
       }, {
            kDPLL2, 180, 180, 12, 7, 2                  // dpll2 (DSP)
       }
    }
};

// MPU[500Mhz @ 1.20V], IVA2[360Mhz @ 1.20V]
static VddOppSetting_t vdd1Opp3Info = {
    {
        kSmartReflex_Channel1, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor1, 0x30,   0x30                // voltage processor info
    }, 
        2,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 500, 250, 12, 7, 1                  // dpll1 (MPU)
       }, {
            kDPLL2, 360, 180, 12, 7, 1                  // dpll2 (DSP)
       }
    }
};

// MPU[550Mhz @ 1.27V], IVA2[400Mhz @ 1.27V]
static VddOppSetting_t vdd1Opp4Info = {
    {
        kSmartReflex_Channel1, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor1, 0x35,   0x35                // voltage processor info
    }, 
        2,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 550, 275, 12, 7, 1                  // dpll1 (MPU)
       }, {
            kDPLL2, 396, 200, 12, 7, 1                  // dpll2 (DSP)
       }
    }
};

// MPU[600Mhz @ 1.35V], IVA2[430Mhz @ 1.35V]
static VddOppSetting_t vdd1Opp5Info = {
    {
        kSmartReflex_Channel1, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor1, 0x3C,   0x3C                // voltage processor info
    }, 
        2,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 600, 300, 12, 7, 1                  // dpll1 (MPU)
       }, {
            kDPLL2, 430, 215, 12, 7, 1                  // dpll2 (DSP)
       }
    }
};

// MPU[720Mhz @ 1.35V], IVA2[520Mhz @ 1.35V]
static VddOppSetting_t vdd1Opp6Info = {
    {
        kSmartReflex_Channel1, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor1, 0x3C,   0x3C                // voltage processor info
    }, 
        2,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 720, 360, 12, 7, 1                  // dpll1 (MPU)
       }, {
            kDPLL2, 520, 260, 12, 7, 1                  // dpll2 (DSP)
       }
    }
};


//-----------------------------------------------------------------------------
// (just a placeholder)
static VddOppSetting_t vdd2Opp0Info = {
    {
        kSmartReflex_Channel2, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor2, 0, 0                        // voltage processor info
    }, 
        1,
    {                                                   // vdd2 has 1 dpll
       {
            kDPLL3,   0,  (UINT)-1,  0, 0, 0                  // dpll3 (CORE)
       }
    }
};

//CORE[166Mhz @ 1.05V]
static VddOppSetting_t vdd2Opp1Info = {
    {
        kSmartReflex_Channel2, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor2, 0x24,    0x24               // voltage processor info
    }, 
        1,
    {                                                   // vdd2 has 1 dpll
       {
            kDPLL3, 166, 166, 12, 7, 2                  // dpll3 (CORE)
       }
    }
};

// CORE[332Mhz @ 1.15V]
static VddOppSetting_t vdd2Opp2Info = {
    {
        kSmartReflex_Channel2, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor2, 0x2C,    0x2C               // voltage processor info
    }, 
        1,
    {                                                   // vdd2 has 1 dpll
       {
            kDPLL3, 332, 166, 12, 7, 1                  // dpll3 (CORE)
       }
    }
};

//-----------------------------------------------------------------------------
static VddOppSetting_t  *_rgVdd1OppMap[VDD1_OPP_COUNT_35xx] = {
    &vdd1Opp1Info,      // kOpp1
    &vdd1Opp2Info,      // kOpp2
    &vdd1Opp3Info,      // kOpp3
    &vdd1Opp4Info,      // kOpp4
    &vdd1Opp5Info,      // kOpp5
    &vdd1Opp6Info       // kOpp6
};

static VddOppSetting_t  *_rgVdd2OppMap[VDD2_OPP_COUNT] = {
    &vdd2Opp1Info,      // kOpp1
    &vdd2Opp2Info,      // kOpp2
};

// 37xx family CPU Settings
// (just a placeholder)
static VddOppSetting_t vdd1Opp0Info_37x = {
    {
        kSmartReflex_Channel1, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor1, 0, 0                        // voltage processor info
    }, 
        2,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 0,  (UINT)-1, 0,  0, 1                    // dpll1 (MPU)
       }, {
            kDPLL2, 0,  (UINT)-1, 0,  0, 1                    // dpll2 (DSP)
       }
    }
};

// MPU[300Mhz @ 0.xxxV], IVA2[ 260Mhz @ 0.975V]
static VddOppSetting_t vdd1Opp1Info_37x = {
    {
        kSmartReflex_Channel1, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor1, 0x1B,   0x1B                // voltage processor info
    }, 
        2,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 300, 150, 12, 0, 1                  // dpll1 (MPU)
       }, {
            kDPLL2,  260, 130, 12, 0, 1                  // dpll2 (DSP)
       }
    }
};

// MPU[600Mhz @ 1.00V], IVA2[520Mhz @ 1.05V]
static VddOppSetting_t vdd1Opp2Info_37x = {
    {
        kSmartReflex_Channel1, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor1, 0x28,   0x28                // voltage processor info
    }, 
        2,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 600, 300, 12, 0, 1                  // dpll1 (MPU)
       }, {
            kDPLL2, 520, 260, 12, 0, 1                  // dpll2 (DSP)
       }
    }
};

// MPU[800Mhz @ 1.20V], IVA2[660Mhz @ 1.20V]
static VddOppSetting_t vdd1Opp3Info_37x = {
    {
        kSmartReflex_Channel1, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor1, 0x35,   0x35                // voltage processor info
    }, 
        2,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 800, 400, 12, 0, 1                  // dpll1 (MPU)
       }, {
            kDPLL2, 660, 330, 12, 0, 1                  // dpll2 (DSP)
       }
    }
};

// MPU[1000Mhz @ 1.27V], IVA2[875Mhz @ 1.27V]
static VddOppSetting_t vdd1Opp4Info_37x = {
    {
        kSmartReflex_Channel1, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor1, 0x3E,   0x3E                // voltage processor info
    }, 
        2,
    {                                                   // vdd1 has 2 dpll's
       {
            kDPLL1, 1000, 500, 12, 0, 1                  // dpll1 (MPU)
       }, {
            kDPLL2, 800, 400, 12, 0, 1                  // dpll2 (DSP)
       }
    }
};



//-----------------------------------------------------------------------------
// (just a placeholder)
static VddOppSetting_t vdd2Opp0Info_37x = {
    {
        kSmartReflex_Channel2, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor2, 0, 0                        // voltage processor info
    }, 
        1,
    {                                                   // vdd2 has 1 dpll
       {
            kDPLL3,   0,  (UINT)-1,  0, 0, 0                  // dpll3 (CORE)
       }
    }
};

//CORE[200Mhz @ 1.05V]
static VddOppSetting_t vdd2Opp1Info_37x = {
    {
        kSmartReflex_Channel2, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor2, 0x1B,    0x1B               // voltage processor info
    }, 
        1,
    {                                                   // vdd2 has 1 dpll
       {
            kDPLL3, 200, 200, 12, 0, 2                  // dpll3 (CORE)
            
       }
    }
};

// CORE[400Mhz @ 1.15V]
static VddOppSetting_t vdd2Opp2Info_37x = {
    {
        kSmartReflex_Channel2, BSP_SRCLKLEN_INIT        // smartreflex info
    }, {
        kVoltageProcessor2, 0x2B,    0x2B               // voltage processor info
    }, 
        1,
    {                                                   // vdd2 has 1 dpll
       {
            kDPLL3, 400, 200, 12, 0, 1                  // dpll3 (CORE)
            
       }
    }
};
//-----------------------------------------------------------------------------
static VddOppSetting_t  *_rgVdd1OppMap_37x[VDD1_OPP_COUNT_37xx] = {
    &vdd1Opp1Info_37x,      // kOpp1
    &vdd1Opp2Info_37x,      // kOpp2
    &vdd1Opp3Info_37x,      // kOpp3
    &vdd1Opp4Info_37x,      // kOpp4
};

static VddOppSetting_t  *_rgVdd2OppMap_37x[VDD2_OPP_COUNT] = {
    &vdd2Opp1Info_37x,      // kOpp1
    &vdd2Opp2Info_37x,      // kOpp2
};

//-----------------------------------------------------------------------------
#endif // __BSP_OPP_MAP_H

