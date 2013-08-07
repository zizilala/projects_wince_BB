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
//  File:  opp_map.c
//
#include "bsp.h"
#include "oalex.h"
#include "oal_prcm.h"
#include "oal_sr.h"
#include "omap_led.h"
#include "omap3530_dvfs.h"
#include "bsp_opp_map.h"

#define MAX_VOLT_DOMAINS        (2)

#define VP_VOLTS(x) (((x) * 125 + 6000) / 10000)
#define VP_MILLIVOLTS(x) (((x) * 125 + 6000) % 10000)

#pragma warning(push)
#pragma warning(disable: 6385)
#pragma warning(disable: 6386)

//-----------------------------------------------------------------------------
static SmartReflexStateInfo_t   _rgSmartReflexInfo[2] = {
    {
        FALSE, FALSE
    }, {
        FALSE, FALSE
    }
};

//-----------------------------------------------------------------------------
static UINT _rgOppVdd[2];
static UINT _Max_Vdd1_OPP=0;

//-----------------------------------------------------------------------------
static UINT _rgInitialRetentionVoltages[2] = {
    VC_CMD_0_VOLT_RET >> SMPS_RET_SHIFT,
    VC_CMD_1_VOLT_RET >> SMPS_RET_SHIFT
};

//-----------------------------------------------------------------------------
// Error Gain and Error Offset configuration 
static VoltageProcessorErrConfig_t _rgVp1ErrConfig_v1[5] = {

                                 /* v1 settings for 35xx */
                                 { VDD1_OPP1_ERRORGAIN, VDD_NO_ERROFFSET },
                                 { VDD1_OPP2_ERRORGAIN, VDD_NO_ERROFFSET },
                                 { VDD1_OPP3_ERRORGAIN, VDD_NO_ERROFFSET },
                                 { VDD1_OPP4_ERRORGAIN, VDD_NO_ERROFFSET },
                                 { VDD1_OPP5_ERRORGAIN, VDD_NO_ERROFFSET },
                                 };

static VoltageProcessorErrConfig_t _rgVp1ErrConfig_v2[4] = {

                                 /* v2 settings for 37xx */
                                 { VDD1_OPP50_ERRORGAIN, VDD_NO_ERROFFSET },
                                 { VDD1_OPP100_ERRORGAIN, VDD_NO_ERROFFSET },
                                 { VDD1_OPP130_ERRORGAIN, VDD_NO_ERROFFSET },
                                 { VDD1_OPP1G_ERRORGAIN, VDD_NO_ERROFFSET },

                                 };

static VoltageProcessorErrConfig_t _rgVp2ErrConfig_v1 [2] = {
                                 /* v1 settings for 35xx */
                                 { VDD2_OPP1_ERRORGAIN, VDD_NO_ERROFFSET },
                                 { VDD2_OPP2_ERRORGAIN, VDD_NO_ERROFFSET },
                                 };

static VoltageProcessorErrConfig_t _rgVp2ErrConfig_v2 [2] = {
                                 /* v2 settings for 37xx */
                                 { VDD2_OPP50_ERRORGAIN, VDD_NO_ERROFFSET },
                                 { VDD2_OPP100_ERRORGAIN, VDD_NO_ERROFFSET },
                                 };

SmartreflexConfiguration_t      _rgSr1ErrConfig_v1 [5] = {
                                    {     SR1_OPP1_ERRMINLIMIT, 
                                          BSP_SR1_ERRMAXLIMIT_INIT, 
                                          BSP_SR1_ERRWEIGHT_INIT
                                    },
                                    {     SR1_OPP2_ERRMINLIMIT, 
                                          BSP_SR1_ERRMAXLIMIT_INIT, 
                                          BSP_SR1_ERRWEIGHT_INIT
                                    },
                                    {     SR1_OPP3_ERRMINLIMIT, 
                                          BSP_SR1_ERRMAXLIMIT_INIT, 
                                          BSP_SR1_ERRWEIGHT_INIT
                                    },
                                    {     SR1_OPP4_ERRMINLIMIT, 
                                          BSP_SR1_ERRMAXLIMIT_INIT, 
                                          BSP_SR1_ERRWEIGHT_INIT
                                    },
                                    {     SR1_OPP5_ERRMINLIMIT, 
                                          BSP_SR1_ERRMAXLIMIT_INIT, 
                                          BSP_SR1_ERRWEIGHT_INIT
                                    }
                                };

SmartreflexConfiguration_t      _rgSr1ErrConfig_v2 [4] = {
                                    {     SR1_OPP50_ERRMINLIMIT, 
                                          BSP_SR1_ERRMAXLIMIT_INIT, 
                                          BSP_SR1_ERRWEIGHT_INIT
                                    },
                                    {     SR1_OPP100_ERRMINLIMIT, 
                                          BSP_SR1_ERRMAXLIMIT_INIT, 
                                          BSP_SR1_ERRWEIGHT_INIT
                                    },
                                    {     SR1_OPP130_ERRMINLIMIT, 
                                          BSP_SR1_ERRMAXLIMIT_INIT, 
                                          BSP_SR1_ERRWEIGHT_INIT
                                    },
                                    {     SR1_OPP1G_ERRMINLIMIT, 
                                          BSP_SR1_ERRMAXLIMIT_INIT, 
                                          BSP_SR1_ERRWEIGHT_INIT
                                    },
                                };

SmartreflexConfiguration_t      _rgSr2ErrConfig_v1 [2] = {
                                    {     SR2_OPP1_ERRMINLIMIT, 
                                          BSP_SR1_ERRMAXLIMIT_INIT, 
                                          BSP_SR1_ERRWEIGHT_INIT
                                    },
                                    {     SR2_OPP2_ERRMINLIMIT, 
                                          BSP_SR1_ERRMAXLIMIT_INIT, 
                                          BSP_SR1_ERRWEIGHT_INIT
                                    }
                                };

SmartreflexConfiguration_t      _rgSr2ErrConfig_v2 [2] = {
                                    {     SR2_OPP50_ERRMINLIMIT, 
                                          BSP_SR1_ERRMAXLIMIT_INIT, 
                                          BSP_SR1_ERRWEIGHT_INIT
                                    },
                                    {     SR2_OPP100_ERRMINLIMIT, 
                                          BSP_SR1_ERRMAXLIMIT_INIT, 
                                          BSP_SR1_ERRWEIGHT_INIT
                                    }
                                };
//-----------------------------------------------------------------------------
//  nValues for testing purpose
static UINT32 omap37xx_sr1_test_nvalues[] = {
	0x898beb, 0x999b83, 0xaac5a8, 0xaab197,
};
static UINT32 omap37xx_sr2_test_nvalues[] = {
	0x898beb,0x898beb, 0x9a8cee,
};

//-----------------------------------------------------------------------------
//  SmartReflex_driver_Data_t
//      
//      Smart Reflex driver context data
// 
typedef struct {
    DWORD SmartReflex_rev;
    BOOL Use_test_value;
    SmartreflexConfiguration_t  *Sr1ErrCfg;
    SmartreflexConfiguration_t  *Sr2ErrCfg;
    VddOppSetting_t   **ppVoltDomain1;
    VddOppSetting_t   **ppVoltDomain2;
    VoltageProcessorErrConfig_t *Vp1ErrConfig;
    VoltageProcessorErrConfig_t *Vp2ErrConfig;	
}SmartReflex_driver_Data_t;

//-----------------------------------------------------------------------------
// _SR_driver_data_35xx
//      
// SmartReflex driver data for 35xx
// 
SmartReflex_driver_Data_t _SR_driver_data_35xx=
{
    1, 
    FALSE,
    _rgSr1ErrConfig_v1,
    _rgSr2ErrConfig_v1,
    _rgVdd1OppMap,
    _rgVdd2OppMap,
    _rgVp1ErrConfig_v1,
    _rgVp2ErrConfig_v1    
};

//-----------------------------------------------------------------------------
// _SR_driver_data_37xx
//      
// SmartReflex driver data for 37xx
// 
SmartReflex_driver_Data_t _SR_driver_data_37xx=
{
    2, 
    FALSE,
    _rgSr1ErrConfig_v2,
    _rgSr2ErrConfig_v2,
    _rgVdd1OppMap_37x,
    _rgVdd2OppMap_37x,
    _rgVp1ErrConfig_v2,
    _rgVp2ErrConfig_v2    
};

//-----------------------------------------------------------------------------
// static : _sensorEnData
//      
//      Saves the sensor enable data from EFuse
// 
static SmartReflexSenorEnData_t     _sensorEnData;

//-----------------------------------------------------------------------------
// static : _bSmartreflexCapable
//      
//      Saves the hardware SmartReflex capability state
// 
static BOOL _bSmartreflexCapable;

//-----------------------------------------------------------------------------
// SmartReflex_funcs
//      
// SmartReflex API function table pointer
// 
SmartReflex_Functions_t *SmartReflex_funcs;

//-----------------------------------------------------------------------------
// _SR_driver_data
//      
// Pointer to Smart Reflex driver data, it is assigned at run time based on the chip
// 
static SmartReflex_driver_Data_t *_SR_driver_data;


//-----------------------------------------------------------------------------
// Prototype
//
void SmartReflex_VoltageFlush(VoltageProcessor_e vp);
void Opp_init(void);


//-----------------------------------------------------------------------------
__inline
void 
SmartReflexLock(UINT channel)
{
    if (_rgSmartReflexInfo[channel].bInitialized && INTERRUPTS_STATUS()) 
        {
        EnterCriticalSection(&_rgSmartReflexInfo[channel].cs);
        }
}

//-----------------------------------------------------------------------------
__inline
void 
SmartReflexUnlock(UINT channel)
{
    if (_rgSmartReflexInfo[channel].bInitialized && INTERRUPTS_STATUS()) 
        {
        LeaveCriticalSection(&_rgSmartReflexInfo[channel].cs);
        }
}

//-----------------------------------------------------------------------------
BOOL 
IsSmartReflexMonitoringEnabled(UINT channel)
{   
    return _rgSmartReflexInfo[channel].bEnabled;
}

//-----------------------------------------------------------------------------
void 
SmartReflex_dump_sensor_data(Efuse_SensorData_t *pdata)
{
#ifndef SHIP_BUILD
    UINT8 idx;

    OALMSG(1, (L"SmartReflex_dump_sensor_data : VDD1\r\n"));

    for(idx=0;idx<kOpp6;idx++)
    {
        OALMSG(1, (L"idx=%d\t opp=%x \r\n", idx, pdata->Efuse_Vdd1_Opp[idx]));
    }
    OALMSG(1, (L"\r\nSmartReflex_dump_sensor_data : VDD2\r\n"));

    for(idx = 0; idx<= kOpp3; idx++)
    {
        OALMSG(1, (L"idx=%d\t opp=%x \r\n", idx, pdata->Efuse_Vdd2_Opp[idx]));
    }
#else
	UNREFERENCED_PARAMETER(pdata);
#endif
}

/* Voltage Margin in mV */
#define OMAP37XX_HIGH_OPP4_VMARGIN	100
#define OMAP37XX_HIGH_OPP_VMARGIN	75
#define OMAP37XX_LOW_OPP_VMARGIN	63

#define GAIN_MAXLIMIT	16
#define R_MAXLIMIT	256


static void cal_reciprocal(UINT32 sensor, UINT32 *sengain, UINT32 *rnsen)
{
	UINT32 gn, rn, mul;

	for (gn = 0; gn < GAIN_MAXLIMIT; gn++) {
		mul = 1 << (gn + 8);
		rn = mul / sensor;
		if (rn < R_MAXLIMIT) {
			*sengain = gn;
			*rnsen = rn;
		}
	}
}

static UINT32 cal_test_nvalue(UINT32 sennval, UINT32 senpval)
{
	UINT32 senpgain, senngain;
	UINT32 rnsenp, rnsenn;

	/* Calculating the gain and reciprocal of the SenN and SenP values */
	cal_reciprocal(senpval, &senpgain, &rnsenp);
	cal_reciprocal(sennval, &senngain, &rnsenn);

	return (senpgain << NVALUERECIPROCAL_SENPGAIN_SHIFT) |
		(senngain << NVALUERECIPROCAL_SENNGAIN_SHIFT) |
		(rnsenp << NVALUERECIPROCAL_RNSENP_SHIFT) |
		(rnsenn << NVALUERECIPROCAL_RNSENN_SHIFT);
}

static unsigned int SmartReflex_Adjust_EFUSE_nvalue(unsigned int opp_no,
						unsigned int orig_opp_nvalue,
						unsigned int mv_delta)
{
	unsigned int new_opp_nvalue = 0;
	unsigned int senp_gain, senn_gain, rnsenp, rnsenn, pnt_delta, nnt_delta;
	unsigned int new_senn, new_senp, senn, senp;

    UNREFERENCED_PARAMETER(opp_no);

	if (orig_opp_nvalue != 0)
	{
	/* calculate SenN and SenP from the efuse value */
	senp_gain = ((orig_opp_nvalue >> 20) & 0xf);
	senn_gain = ((orig_opp_nvalue >> 16) & 0xf);
	rnsenp = ((orig_opp_nvalue >> 8) & 0xff);
	rnsenn = (orig_opp_nvalue & 0xff);

	senp = ((1<<(senp_gain+8))/(rnsenp));
	senn = ((1<<(senn_gain+8))/(rnsenn));

	/* calculate the voltage delta */
	pnt_delta = (26 * mv_delta)/10;
	nnt_delta = (3 * mv_delta);

	/* now lets add the voltage delta to the sensor values */
	new_senn = senn + nnt_delta;
	new_senp = senp + pnt_delta;

	new_opp_nvalue = cal_test_nvalue(new_senn, new_senp);

	OALMSG( TRUE, (L"Compensating OPP%d for %dmV Orig nvalue:0x%x New nvalue:0x%x \r\n",
			opp_no, mv_delta, orig_opp_nvalue, new_opp_nvalue));
	}

	return new_opp_nvalue;
}



//-----------------------------------------------------------------------------
void
SmartReflex_Read_EFUSE(void)
{
    SmartReflexSensorData_t *pSRInfo;
    Efuse_SensorData_t      sensorData;
    Efuse_SensorData_field_t 	*pSenData;
    OMAP_SYSC_GENERAL_REGS  *pSysc = OALPAtoVA(OMAP_SYSC_GENERAL_REGS_PA, FALSE);
    UINT idx;

    memset(&sensorData, 0, sizeof(sensorData));

    if(_SR_driver_data->Use_test_value == TRUE)
    {
        /* VDD1 */ 
	 sensorData.Efuse_Vdd1_Opp_1 = omap37xx_sr1_test_nvalues[0];
	 sensorData.Efuse_Vdd1_Opp_2 = omap37xx_sr1_test_nvalues[1];
	 sensorData.Efuse_Vdd1_Opp_3 = omap37xx_sr1_test_nvalues[2];
	 sensorData.Efuse_Vdd1_Opp_4 = omap37xx_sr1_test_nvalues[3];

        /* VDD2 */
        for(idx=0;idx<3;idx++)
            sensorData.Efuse_Vdd2_Opp[idx]= omap37xx_sr2_test_nvalues[idx];

        /* when use test value, enable both senN and senP by default */
        _sensorEnData.Efuse_SR       = 0x0f0f;

        _bSmartreflexCapable = TRUE;    
    }	
    else
    {
        if(g_dwCpuFamily == CPU_FAMILY_DM37XX)
        {            
            unsigned int opp_nvalue;
            // EFuse data for Vdd1
            /* 37xx has 4 OPPs, and the efuse are in the following order: 
                        offset           OPP        varible   
                        0x11c               
                        0x114         OPP50       OPP1_VDD1
                        0x118         OPP100      OPP2_VDD1
                        0x120         OPP130      OPP3_VDD1
                        0X110         OPP1G       OPP4_VDD1
                    */
            opp_nvalue = INREG32(&pSysc->CONTROL_FUSE_OPP_VDD1[1]);
            sensorData.Efuse_Vdd1_Opp_1 = SmartReflex_Adjust_EFUSE_nvalue(1, opp_nvalue,
                                                      OMAP37XX_LOW_OPP_VMARGIN);
            opp_nvalue = INREG32(&pSysc->CONTROL_FUSE_OPP_VDD1[2]);
            sensorData.Efuse_Vdd1_Opp_2 = SmartReflex_Adjust_EFUSE_nvalue(2, opp_nvalue,
                                                      OMAP37XX_LOW_OPP_VMARGIN);
            opp_nvalue = INREG32(&pSysc->CONTROL_FUSE_OPP_VDD1[4]);
            sensorData.Efuse_Vdd1_Opp_3 = SmartReflex_Adjust_EFUSE_nvalue(3, opp_nvalue,
                                                      OMAP37XX_HIGH_OPP_VMARGIN);
            opp_nvalue = INREG32(&pSysc->CONTROL_FUSE_OPP_VDD1[0]);
            sensorData.Efuse_Vdd1_Opp_4 = SmartReflex_Adjust_EFUSE_nvalue(4, opp_nvalue,
                                                      OMAP37XX_HIGH_OPP4_VMARGIN);
          
            _sensorEnData.Efuse_SR       = INREG32(&pSysc->CONTROL_FUSE_SR);            
            if ((_sensorEnData.Efuse_SR == 0) &&
                (g_dwCpuRevision==CPU_FAMILY_37XX_REVISION_ES_1_2))
            {
                _sensorEnData.Efuse_SR       = 0x0f0f;
            }                        
        }
        else
        {
	        // EFuse data for Vdd1
	        for(idx = 0; idx < 5 && (idx <=_Max_Vdd1_OPP); idx++)
	        {
	            sensorData.Efuse_Vdd1_Opp[idx] = INREG32(&pSysc->CONTROL_FUSE_OPP_VDD1[idx]);
	        }
            _sensorEnData.Efuse_SR       = INREG32(&pSysc->CONTROL_FUSE_SR);
        }

        /* VDD2 */
        for(idx=0;idx<3;idx++)
        {
            sensorData.Efuse_Vdd2_Opp[idx]= INREG32(&pSysc->CONTROL_FUSE_OPP_VDD2[idx]);
        }
        
	
        //SmartReflex_dump_sensor_data(&sensorData);
        _bSmartreflexCapable = FALSE;

       /*for(idx = 0; idx< 5; idx++)
        {
            if(sensorData.Efuse_Vdd1_Opp[idx] != 0) 
            {
                 _bSmartreflexCapable = TRUE;
	          break;
            }
        }
        for(idx = 0; idx<= 2 && !_bSmartreflexCapable; idx++)
        {
            if(sensorData.Efuse_Vdd2_Opp[idx] != 0) 
            {
                 _bSmartreflexCapable = TRUE;
	         break;
            }
        } */
        if((!_bSmartreflexCapable) && (_sensorEnData.Efuse_SR  != 0x00))
            _bSmartreflexCapable = TRUE; 	

    }
	
    /* VDD1 */
    for(idx=kOpp1; idx<= kOpp6;idx++)
    {
        pSRInfo = &_SR_driver_data->ppVoltDomain1[idx]->smartReflexInfo;
  	 /* KOpp6 is duplicate of KOpp5 */
        pSenData = (idx==kOpp6) ? (Efuse_SensorData_field_t *)&sensorData.Efuse_Vdd1_Opp[kOpp5]: 
                                                  (Efuse_SensorData_field_t *)&sensorData.Efuse_Vdd1_Opp[idx];
        pSRInfo->rnsenn = pSenData->rnsenn;
        pSRInfo->rnsenp = pSenData->rnsenp;
        pSRInfo->senngain = pSenData->senngain;
        pSRInfo->senpgain = pSenData->senpgain;

    }

    /* VDD2 */
    // VDD2 OPP1 corresponds to EFUSE VDD2 OPP2
    // VDD2 OPP2 corresponds to EFUSE VDD2 OPP3
    for(idx=kOpp1; idx<= kOpp2;idx++)
    {
        pSRInfo = &_SR_driver_data->ppVoltDomain2[idx]->smartReflexInfo;

        pSenData = (Efuse_SensorData_field_t *)&sensorData.Efuse_Vdd2_Opp[idx+1];
        pSRInfo->rnsenn = pSenData->rnsenn;
        pSRInfo->rnsenp = pSenData->rnsenp;
        pSRInfo->senngain = pSenData->senngain;
        pSRInfo->senpgain = pSenData->senpgain;
    }
    
}

//-----------------------------------------------------------------------------
void 
SmartReflex_Initialize()
{
    if(g_dwCpuFamily == CPU_FAMILY_DM37XX)
    {
        _SR_driver_data = &_SR_driver_data_37xx;
    }
    else
    {
        _SR_driver_data = &_SR_driver_data_35xx;
    }

    // Initialize the voltage processor with the current OPP voltage.
    SmartReflex_VoltageFlush(kVoltageProcessor1);
    SmartReflex_VoltageFlush(kVoltageProcessor2);

    SmartReflex_Read_EFUSE();
    SmartReflex_funcs = SmartReflex_Get_functions_hander(_SR_driver_data->SmartReflex_rev);	

    if(SmartReflex_funcs == NULL)
    {
        OALMSG( OAL_ERROR, (L"\r\nSmartReflex_Initialize: NULL SmartReflex function pointer\r\n"));
        goto Cleanup;
    }
    // initialize smartreflex library
    SmartReflex_funcs->InitializeChannel(kSmartReflex_Channel1, 
        OALPAtoVA(OMAP_SMARTREFLEX1_PA, FALSE)
        );
    SmartReflex_funcs->InitializeChannel(kSmartReflex_Channel2, 
        OALPAtoVA(OMAP_SMARTREFLEX2_PA, FALSE)
        );
Cleanup:
    return;
}

//-----------------------------------------------------------------------------
void 
SmartReflex_PostInitialize()
{
    InitializeCriticalSection(&_rgSmartReflexInfo[kSmartReflex_Channel1].cs);
    InitializeCriticalSection(&_rgSmartReflexInfo[kSmartReflex_Channel2].cs);

    _rgSmartReflexInfo[kSmartReflex_Channel1].bInitialized = TRUE;
    _rgSmartReflexInfo[kSmartReflex_Channel2].bInitialized = TRUE;
}

//-----------------------------------------------------------------------------
void 
SmartReflex_Configure(UINT channel)
{
    if (SmartReflex_funcs == NULL)
    {
        OALMSG(OAL_LOG_ERROR, (L"\r\nInvalid Smart Reflex Functions pointer\r\n"));
        goto CleanUp;
    }
    if (channel == kSmartReflex_Channel1)
        {
        // Configure SR1 Parameters
        SmartReflex_funcs->SetErrorControl(kSmartReflex_Channel1, 
                                    _SR_driver_data->Sr1ErrCfg[_rgOppVdd[channel]].errWeight,
                                    _SR_driver_data->Sr1ErrCfg[_rgOppVdd[channel]].errMaxLimit,
                                    _SR_driver_data->Sr1ErrCfg[_rgOppVdd[channel]].errMinLimit
                                    );

        SmartReflex_funcs->SetSensorMode(kSmartReflex_Channel1, 
                                    _sensorEnData.sr1_sennenable, 
                                    _sensorEnData.sr1_senpenable
                                    );
        
        SmartReflex_funcs->SetAvgWeight(kSmartReflex_Channel1, 
                                    BSP_SR1_SENN_AVGWEIGHT, 
                                    BSP_SR1_SENP_AVGWEIGHT
                                    );
        }
    else
        {
        // Configure SR2 Parameters
        SmartReflex_funcs->SetErrorControl(kSmartReflex_Channel2, 
                                    _SR_driver_data->Sr2ErrCfg[_rgOppVdd[channel]].errWeight,
                                    _SR_driver_data->Sr2ErrCfg[_rgOppVdd[channel]].errMaxLimit,
                                    _SR_driver_data->Sr2ErrCfg[_rgOppVdd[channel]].errMinLimit
                                    );

        SmartReflex_funcs->SetSensorMode(kSmartReflex_Channel2, 
                                    _sensorEnData.sr2_sennenable, 
                                    _sensorEnData.sr2_senpenable
                                    );

        SmartReflex_funcs->SetAvgWeight(kSmartReflex_Channel2, 
                                    BSP_SR2_SENN_AVGWEIGHT,
                                    BSP_SR2_SENP_AVGWEIGHT
                                    );
        }

    // Enable Smartreflex Sensor Block
    SmartReflex_funcs->EnableSensorBlock(channel, TRUE);

    // Enable Smartreflex ErrorGenerator Block
    SmartReflex_funcs->EnableErrorGeneratorBlock(channel, TRUE);
CleanUp:
    return;
}

//------------------------------------------------------------------------------
//
//  Function:  SmartReflex_Deconfigure
//
//  Description : Deconfigures SR module
// 
void 
SmartReflex_Deconfigure(UINT channel)
{
    if (SmartReflex_funcs == NULL)
    {
        OALMSG(OAL_LOG_ERROR, (L"\r\nInvalid Smart Reflex Functions pointer\r\n"));
        goto CleanUp;
    }

    // Disable Smartreflex Sensor Block
    SmartReflex_funcs->EnableSensorBlock(channel, FALSE);

    // Disable Smartreflex ErrorGenerator Block
    SmartReflex_funcs->EnableErrorGeneratorBlock(channel, FALSE);

    // Disable Smartreflex MinMaxAvg Block
    SmartReflex_funcs->EnableMinMaxAvgBlock(channel, FALSE);
	
CleanUp:
    return;	
}

//------------------------------------------------------------------------------
//
//  Function:  SmartReflex_VoltageFlush
//
//  Description : Flushes the Normal voltage for the current OPP
// 
void 
SmartReflex_VoltageFlush(VoltageProcessor_e vp)
{
    UINT32 interruptMask;
    UINT32 tcrr;
    UINT rampDelay = 0;

    if (vp == kVoltageProcessor1)
        {
        interruptMask = PRM_IRQENABLE_VP1_TRANXDONE_EN;
        }
    else
        {
        interruptMask = PRM_IRQENABLE_VP2_TRANXDONE_EN;
        }
    
    PrcmVoltEnableVp(vp, TRUE);
    
    // Get the delay required 
    rampDelay = PrcmVoltGetVoltageRampDelay(vp);

    // clear prcm interrupts
    PrcmInterruptClearStatus(interruptMask);

    // force SMPS voltage update through voltage processor
    PrcmVoltFlushVoltageLevels(vp);
        
    tcrr = OALTimerGetCount();
    while ((PrcmInterruptClearStatus(interruptMask) & interruptMask) == 0 &&
        (OALTimerGetCount() - tcrr) < BSP_ONE_MILLISECOND_TICKS);

    // wait for voltage change complete
    if (rampDelay)
        {
        OALStall(rampDelay);
        }
    
    PrcmVoltEnableVp(vp, FALSE);
}


//------------------------------------------------------------------------------
//
//  Function:  SmartReflex_EnableMonitor
//
//  Description : Enable/Disable Smart reflex
// 
BOOL 
SmartReflex_EnableMonitor(
    UINT                    channel,
    BOOL                    bEnable
    )
{
    VddOppSetting_t   **ppVoltDomain;
    UINT32 tcrr;
    VoltageProcessor_e  vp;
    UINT interruptMask;

    if(!_bSmartreflexCapable) goto cleanUp;

    if (_rgSmartReflexInfo[channel].bEnabled == bEnable ) goto cleanUp;
    
    SmartReflexLock(channel);

    // select correct array
    if (channel == kSmartReflex_Channel1)
        {
        ppVoltDomain = _SR_driver_data->ppVoltDomain1; 
        interruptMask = PRM_IRQENABLE_VP1_TRANXDONE_EN;
        vp = kVoltageProcessor1;
        }
    else
        {
        ppVoltDomain = _SR_driver_data->ppVoltDomain2; 
        interruptMask = PRM_IRQENABLE_VP2_TRANXDONE_EN;
        vp = kVoltageProcessor2;
        }
    
    if (bEnable)
        {
        // Disable Auto Retention and Sleep
        PrcmVoltSetAutoControl(AUTO_SLEEP_DISABLED | AUTO_RET_DISABLED | AUTO_OFF_ENABLED, 
                                AUTO_SLEEP | AUTO_RET | AUTO_OFF );

        PrcmVoltEnableVp(vp, TRUE);
        
        // Enable Smartreflex
        PrcmDeviceEnableClocks(
            channel == kSmartReflex_Channel1 ? OMAP_DEVICE_SR1 : OMAP_DEVICE_SR2, 
            TRUE
            );

        // Set Interface Clk gating on Idle mode
        SmartReflex_funcs->SetIdleMode(channel, SR_CLKACTIVITY_NOIDLE);
        
        // Disable SmartReflex before updating sensor ref values
        SmartReflex_funcs->Enable(channel, FALSE);

        SmartReflex_Configure(channel);

        // enable smartreflex
        SmartReflex_funcs->SetSensorReferenceData(channel, 
            &(ppVoltDomain[_rgOppVdd[channel]]->smartReflexInfo));
        
        SmartReflex_funcs->ClearInterruptStatus(channel,        
                                         ERRCONFIG_VP_BOUNDINT_ST
                                         );

        SmartReflex_funcs->EnableInterrupts(channel,
                                        (UINT) ERRCONFIG_VP_BOUNDINT_EN,
                                        TRUE
                                        );
        
        // Enable Voltage Processor and Smart Reflex
        SmartReflex_funcs->Enable(channel, TRUE);
        }
    else
        {
        // Disable Voltage Processor
        PrcmVoltEnableVp(vp, FALSE);

        // Wait till the Voltage processor is Idle
        tcrr = OALTimerGetCount();
        while(!PrcmVoltIdleCheck(vp) &&
                ((OALTimerGetCount() - tcrr) < BSP_TEN_MILLISECOND_TICKS)); 

        SmartReflex_funcs->EnableInterrupts(channel,
                                        ERRCONFIG_MCU_DISACKINT_EN,
                                        TRUE
                                        );

        // Disable SmartReflex
        SmartReflex_funcs->Enable(channel, FALSE);

        SmartReflex_funcs->EnableInterrupts(channel,
                                        (UINT)ERRCONFIG_VP_BOUNDINT_EN,
                                        FALSE
                                        );

        SmartReflex_funcs->ClearInterruptStatus(channel,        
                                         (UINT)ERRCONFIG_VP_BOUNDINT_ST
                                         );

        SmartReflex_Deconfigure(channel);

        // Wait till SR is disabled
        tcrr = OALTimerGetCount();
        while(((SmartReflex_funcs->ClearInterruptStatus(channel, 
                    ERRCONFIG_MCU_DISACKINT_ST) & ERRCONFIG_MCU_DISACKINT_ST) == 0) &&
                    ((OALTimerGetCount() - tcrr) < BSP_ONE_MILLISECOND_TICKS)); 

        SmartReflex_funcs->EnableInterrupts(channel,
                                        ERRCONFIG_MCU_DISACKINT_EN,
                                        FALSE
                                        );

        // Set Interface and functional Clk gating on Idle mode
        SmartReflex_funcs->SetIdleMode(channel, SR_CLKACTIVITY_IDLE);

        PrcmDeviceEnableClocks(
            channel == kSmartReflex_Channel1 ? OMAP_DEVICE_SR1 : OMAP_DEVICE_SR2, 
            FALSE
            );

        SmartReflex_VoltageFlush(vp);
        }

    _rgSmartReflexInfo[channel].bEnabled = bEnable;

    SmartReflexUnlock(channel);

cleanUp:
    return TRUE;
}

//-----------------------------------------------------------------------------
void 
UpdateRetentionVoltages(IOCTL_RETENTION_VOLTAGES *pData)
{
    BOOL    bEnableSmartReflex = FALSE;
    UINT32  tcrr;
    UINT    vp;
    UINT    interruptMask[kVoltageProcessorCount] = {
                            PRM_IRQENABLE_VP1_TRANXDONE_EN,
                            PRM_IRQENABLE_VP2_TRANXDONE_EN
                            };

    if(!_bSmartreflexCapable) goto cleanUp;

    for (vp = kVoltageProcessor1 ; vp < kVoltageProcessorCount ; vp++)
        {

        OALMSG(1, (L"UpdateRetentionVoltages: VDD%d %d.%04dV \r\n", vp + 1, VP_VOLTS(pData->retentionVoltage[vp]), VP_MILLIVOLTS(pData->retentionVoltage[vp])));

        //check if SmartReflex was enabled
        if (IsSmartReflexMonitoringEnabled(vp))
            {
            bEnableSmartReflex = TRUE;
            SmartReflex_EnableMonitor(vp, FALSE);
            }
    
        // disable vp
        PrcmVoltEnableVp(vp, FALSE);
    
        // enable voltage processor timeout
        PrcmVoltEnableTimeout(vp, TRUE);
    
        // update retention value
        PrcmVoltSetVoltageLevel(
            vp, 
            pData->retentionVoltage[vp], 
            SMPS_RET_MASK
            );
        
        PrcmVoltSetVoltageLevel(
            vp, 
            pData->retentionVoltage[vp], 
            (UINT)SMPS_ON_MASK
            );
    
        PrcmVoltSetVoltageLevel(
            vp, 
            pData->retentionVoltage[vp], 
            SMPS_ONLP_MASK
            );
    
        PrcmVoltSetInitVddLevel(
            vp, 
            pData->retentionVoltage[vp]
            );
    
        // enable voltage processor
        PrcmVoltEnableVp(vp, TRUE);
            
        // force SMPS voltage update through voltage processor
        PrcmVoltFlushVoltageLevels(vp);
    
        // wait for voltage change complete
        tcrr = OALTimerGetCount();
        while ((PrcmInterruptClearStatus(interruptMask[vp]) & interruptMask[vp]) == 0 &&
            (OALTimerGetCount() - tcrr) < BSP_MAX_VOLTTRANSITION_TIME);
    
    
        // disable the voltage processor sub-chip
        PrcmVoltEnableVp(vp, FALSE);
    
        //restore SmartReflex state	
        if (bEnableSmartReflex == TRUE) 
            {
            SmartReflex_EnableMonitor(vp, TRUE);
            }
        }
    
cleanUp:
    return;
}

//-----------------------------------------------------------------------------
BOOL
SetVoltageOppViaVoltageProcessor(
    VddOppSetting_t        *pVddOppSetting,
    UINT                   *retentionVoltages
    )
{
    UINT32 tcrr;
    BOOL rc = FALSE;        
    UINT interruptMask = (pVddOppSetting->vpInfo.vp == kVoltageProcessor1) ?
                            PRM_IRQENABLE_VP1_TRANXDONE_EN :
                            PRM_IRQENABLE_VP2_TRANXDONE_EN;

    // disable vp
    PrcmVoltEnableVp(pVddOppSetting->vpInfo.vp, FALSE);

    // enable voltage processor timeout
    PrcmVoltEnableTimeout(pVddOppSetting->vpInfo.vp, TRUE);

    OALMSG(OAL_INFO, (L"Vdd%d=0x%02X\r\n", pVddOppSetting->vpInfo.vp + 1,
        pVddOppSetting->vpInfo.initVolt)
        );

    // configure voltage processor parameters
    PrcmVoltSetVoltageLevel(
        pVddOppSetting->vpInfo.vp, 
        pVddOppSetting->vpInfo.initVolt, 
        (UINT) SMPS_ON_MASK
        );

    OALMSG(1, (L"VDD%d %d.%04dV \r\n", pVddOppSetting->vpInfo.vp + 1, VP_VOLTS(pVddOppSetting->vpInfo.initVolt), VP_MILLIVOLTS(pVddOppSetting->vpInfo.initVolt)));

    PrcmVoltSetVoltageLevel(
        pVddOppSetting->vpInfo.vp, 
        pVddOppSetting->vpInfo.lpVolt, 
        (UINT) SMPS_ONLP_MASK
        );
    
    PrcmVoltSetVoltageLevel(
        pVddOppSetting->vpInfo.vp, 
        retentionVoltages[pVddOppSetting->vpInfo.vp], 
        SMPS_RET_MASK
        );

    PrcmVoltSetInitVddLevel(
        pVddOppSetting->vpInfo.vp, 
        pVddOppSetting->vpInfo.initVolt
        );

    // clear prcm interrupts
    PrcmInterruptClearStatus(interruptMask);

    // enable voltage processor
    PrcmVoltEnableVp(pVddOppSetting->vpInfo.vp, TRUE);
        
    // force SMPS voltage update through voltage processor
    PrcmVoltFlushVoltageLevels(pVddOppSetting->vpInfo.vp);

    tcrr = OALTimerGetCount();
    while ((PrcmInterruptClearStatus(interruptMask) & interruptMask) == 0 &&
        (OALTimerGetCount() - tcrr) < BSP_ONE_MILLISECOND_TICKS);

    // disable the voltage processor sub-chip
    PrcmVoltEnableVp(pVddOppSetting->vpInfo.vp, FALSE);

    rc = TRUE;

    return rc;
}

//-----------------------------------------------------------------------------
BOOL
SetFrequencyOpp(
    VddOppSetting_t        *pVddOppSetting
    )
{
    int i;
    BOOL rc = FALSE;

    // iterate through and set the dpll frequency settings    
    for (i = 0; i < pVddOppSetting->dpllCount; ++i)
        {
        PrcmClockSetDpllFrequency(
            pVddOppSetting->rgFrequencySettings[i].dpllId,
            pVddOppSetting->rgFrequencySettings[i].m,
            pVddOppSetting->rgFrequencySettings[i].n,
            pVddOppSetting->rgFrequencySettings[i].freqSel,
            pVddOppSetting->rgFrequencySettings[i].outputDivisor
            );
        };

    rc = TRUE;

    return rc;
}

//-----------------------------------------------------------------------------
BOOL
SetVoltageOpp(
    VddOppSetting_t    *pVddOppSetting
    )
{
    return SetVoltageOppViaVoltageProcessor(pVddOppSetting,_rgInitialRetentionVoltages);
}

void Opp_init(void)
{
    if(g_dwCpuFamily == CPU_FAMILY_DM37XX)
    {
        _rgOppVdd[0] = Vdd1_init_val_37xx[BSP_OPM_SELECT_37XX];
        _Max_Vdd1_OPP	 = MAX_VDD1_OPP_37xx;
    }
    else
    {
        _rgOppVdd[0] = Vdd1_init_val_35xx[BSP_OPM_SELECT_35XX];
        _Max_Vdd1_OPP	 = MAX_VDD1_OPP_35xx;
		
    }

    _rgOppVdd[1] = INITIAL_VDD2_OPP;
	
}
//-----------------------------------------------------------------------------
BOOL 
SetOpp(
    DWORD *rgDomains,
    DWORD *rgOpps,    
    DWORD  count
    )
{
    UINT                opp;
    UINT                i;
    int                 vdd = 0;
    VddOppSetting_t   **ppVoltDomain;
    BOOL                bEnableSmartReflex = FALSE;
    VoltageProcessorErrConfig_t *ErrConfig;


    // loop through and update all changing voltage domains
    //
    for (i = 0; i < count; ++i)
        {
        // select the Opp table to use
        switch (rgDomains[i])
            {
            case DVFS_MPU1_OPP:
                vdd = kVDD1;
                if (rgOpps[i] > _Max_Vdd1_OPP) continue;
                ppVoltDomain = _SR_driver_data->ppVoltDomain1; 
                ErrConfig = _SR_driver_data->Vp1ErrConfig;
                break;
                
            case DVFS_CORE1_OPP:
                // validate parameters
                if (rgOpps[i] > MAX_VDD2_OPP) continue;
                
                vdd = kVDD2;
                ppVoltDomain = _SR_driver_data->ppVoltDomain2; 
                ErrConfig = _SR_driver_data->Vp2ErrConfig;
                break;
                
            default:
                continue;
            }

        // check if the operating point is actually changing
        opp = rgOpps[i];
        OALMSG(1, (L"SetOpp to %d \r\n", opp));
        if (_rgOppVdd[vdd] == opp) continue;

        // disable smartreflex if it's enabled
        if (IsSmartReflexMonitoringEnabled(vdd))
            {
            bEnableSmartReflex = TRUE;
            SmartReflex_EnableMonitor(vdd, FALSE);
            }

        // depending on which way the transition is occurring change
        // the frequency and voltage levels in the proper order
        if (opp > _rgOppVdd[vdd])
            {
            // transitioning to higher performance, change voltage first
            SetVoltageOpp(ppVoltDomain[opp]);
                if ((vdd == kVDD1)&& (g_dwCpuFamily == CPU_FAMILY_DM37XX))
                    PrcmVoltScaleVoltageABB(opp);
            SetFrequencyOpp(ppVoltDomain[opp]);         
            }
        else
            {
            // transitioning to lower performance, change frequency first
            SetFrequencyOpp(ppVoltDomain[opp]); 
            SetVoltageOpp(ppVoltDomain[opp]);         
                if ((vdd == kVDD1)&& (g_dwCpuFamily == CPU_FAMILY_DM37XX))
                    PrcmVoltScaleVoltageABB(opp);
            }

        if (vdd == kVDD1)
            {
            PrcmVoltSetErrorConfiguration(kVoltageProcessor1, 
                                          ErrConfig[opp].errOffset, 
                                          ErrConfig[opp].errGain);
            }
        else
            {
            PrcmVoltSetErrorConfiguration(kVoltageProcessor2, 
                                          ErrConfig[opp].errOffset, 
                                          ErrConfig[opp].errGain);
            }
            
        // update opp for voltage domain
        _rgOppVdd[vdd] = opp;

        // re-enable smartreflex if previously enabled
        if (bEnableSmartReflex == TRUE) 
            {
            SmartReflex_EnableMonitor(vdd, TRUE);
            }
        }

    OALLED(LED_IDX_MPU_VDD, (_rgOppVdd[kVDD1] + 1));
    OALLED(LED_IDX_CORE_VDD, (_rgOppVdd[kVDD2] + 1));
                
    // update latency table
    OALWakeupLatency_UpdateOpp(rgDomains, rgOpps, count);
    
    return TRUE;    
}

#pragma warning(pop)

//-----------------------------------------------------------------------------

