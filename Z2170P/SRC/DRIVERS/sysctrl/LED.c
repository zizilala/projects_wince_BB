#include <windows.h>
#include "bsp.h"
#include "clkmgr.h"
#include "runtime_context.h"
#include "GPIO_SOC.h"
#include "GPIO_Platform.h"
#include "mfp_soc.h"
#include "..\SDK\LIB\PowerCtl\PowerCtl.h"

#define BT_LED_ON   0x01
#define WIFI_LED_ON 0x02

HANDLE BT_LED_Event = NULL;

HANDLE WIFI_LED_Event = NULL;

extern volatile PXA_GPIOREG_T  *g_pGPIORegisters;

extern volatile UINT32 *g_pMFPBase;

extern void MapResources(void);

BOOL Init_LED(void)
{
    UINT32 led_mfp_list[] =
    {
        PXA_MFP_PIN_GPIO47_OFFSET, // BT LED
        PXA_MFP_PIN_GPIO53_OFFSET, // WIFI LED
        PXA_MFP_PIN_GPIO82_OFFSET, // BAROCDE LED
        PXA_MFP_PIN_GPIO20_OFFSET, // vibrator
        PXA_MFP_PIN_EOLIST_MARKER
    };

    PXA_MFP_ALT_FN_T led_af_list[] =
    {
        PXA_MFP_ALT_FN_0,
        PXA_MFP_ALT_FN_0,
        PXA_MFP_ALT_FN_0,
        PXA_MFP_ALT_FN_0
    };

    PXA_MFP_DRIVE_STRENGTH_T led_ds_list[] =
    {
        PXA_MFP_DS_03X,
        PXA_MFP_DS_03X,
        PXA_MFP_DS_03X,
        PXA_MFP_DS_03X
    };

    PXA_MFP_LPM_OUTPUT_T led_lpm_list[] =
    {
        PXA_MFP_LPMO_DRIVE_HIGH,
        PXA_MFP_LPMO_DRIVE_HIGH,
        PXA_MFP_LPMO_DRIVE_HIGH,
        //PXA_MFP_LPMO_DRIVE_LOW
        PXA_MFP_LPMO_DRIVE_HIGH
    };

    MapResources();

    if(!g_pGPIORegisters || !g_pMFPBase)
    {
        return FALSE;
    }

    PXA_MFPSetAfDsList(g_pMFPBase, led_mfp_list, led_af_list, led_ds_list);

    PXA_MFPConfigureLpmOutputLevelList(g_pMFPBase, led_mfp_list, led_lpm_list);

    PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_47, PXA_HI);

    PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_53, PXA_HI);

    PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_82, PXA_HI);
    

    PXA_MFPConfigurePullUpDown(g_pMFPBase, PXA_MFP_PIN_GPIO20_OFFSET, PXA_MFP_PULL_UP);
    PXA_MFPActivatePullUpDown(g_pMFPBase, PXA_MFP_PIN_GPIO20_OFFSET, PXA_ON);
    PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_20, PXA_GPIO_DIRECTION_OUT);
    PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_20, PXA_HI);

                         
    PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters,
                         PXA_GPIO_47,
                         PXA_GPIO_DIRECTION_OUT);

    PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters,
                         PXA_GPIO_53,
                         PXA_GPIO_DIRECTION_OUT);

    PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters,
                         PXA_GPIO_82,
                         PXA_GPIO_DIRECTION_OUT);




    return TRUE;
}

BOOL SetBluetoothLED(BOOL dwONState)
{
    if(!g_pGPIORegisters)
    {
        return FALSE;
    }

    return PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_47, dwONState ? PXA_LO : PXA_HI);
}

BOOL SetWIFILED(BOOL dwONState)
{
    if(!g_pGPIORegisters)
    {
        return FALSE;
    }

    return PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_53, dwONState ? PXA_LO : PXA_HI);
}

BOOL SetBarcodeLED(BOOL dwONState)
{
    if(!g_pGPIORegisters)
    {
        return FALSE;
    }

    // LED
    PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_82, dwONState ? PXA_LO : PXA_HI);
    // Vibrator
    if( *(volatile BYTE *)DEVICE_POWER_FLAG_ADDR & VIBRATORI_POWER_MASK )
    	PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_20, dwONState ? PXA_LO : PXA_HI);

    return TRUE;
}

DWORD BCRLEDThread(LPVOID pvarg)
{
	HANDLE BCR_LED_Event = CreateEvent(NULL, FALSE, FALSE, BCR_LED_NOTIFY);

    while(BCR_LED_Event)
	{
		WaitForSingleObject(BCR_LED_Event, INFINITE);

        SetBluetoothLED(FALSE);
        SetWIFILED(FALSE);

		SetBarcodeLED(TRUE);
        WaitForSingleObject(BCR_LED_Event, 100);
        SetBarcodeLED(FALSE);
	}

    return 0;
}

DWORD LEDThread(LPVOID pvarg)
{
	DWORD TimeOut = INFINITE;
    DWORD WaitState;
    BOOL BTisON, WIFIisON;
    BYTE LastLED;
    DWORD dwTimeCount = 0;
    HANDLE  WaitEvents[2];
	HANDLE hThread,WIFI_RDY_Event=NULL;

    SetBluetoothLED(FALSE);

    SetWIFILED(FALSE);

    /*while(!IsAPIReady(SH_SHELL))
    {
        Sleep(500);
    }*/
    WaitForAPIReady(SH_SHELL, INFINITE);
	Sleep(500);			

    hThread = CreateThread(NULL, 0, BCRLEDThread, NULL, 0, NULL);
    CloseHandle(hThread);


    BT_LED_Event = CreateEvent(NULL, FALSE, FALSE, BT_LED_NOTIFY);
    WIFI_LED_Event = CreateEvent(NULL, FALSE, FALSE, WIFI_LED_NOTIFY);
	WIFI_RDY_Event=CreateEvent(NULL, FALSE, FALSE, WIFI_RDY_NOTIFY); //20111026-luk
   	  
	if(BT_LED_Event==NULL || WIFI_LED_Event==NULL||WIFI_RDY_Event==NULL)
		return 0;	

    WaitEvents[0]  = BT_LED_Event;
    WaitEvents[1]  = WIFI_LED_Event;

    LastLED = 0;
    BTisON = FALSE;
    WIFIisON = FALSE;

	WaitForSingleObject(WIFI_RDY_Event, 3000);	//20111026-luk
	CloseHandle(WIFI_RDY_Event);
	
    if(SysGetWLANPower(&WIFIisON))
    {
        if(WIFIisON)
        {
            TimeOut = 500;
        }
    }

    if((*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR & BT_POWER_MASK))
    {
        TimeOut = 500;
        BTisON = TRUE;
    }

    while(1)
    {
        WaitState = WaitForMultipleObjects(2, &WaitEvents[0], FALSE, TimeOut);

		switch(WaitState)
        {
            case WAIT_OBJECT_0:
                BTisON = (*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR & BT_POWER_MASK) ? TRUE : FALSE;
                TimeOut = 500;
                break;

            case(WAIT_OBJECT_0+1):
                SysGetWLANPower(&WIFIisON);
                TimeOut = 500;
                break;
                
            default:
                break;
        }

        if(WaitState == WAIT_TIMEOUT)
        {
            switch(TimeOut)
            {
                case 500:
                    SetBluetoothLED(FALSE);
                    SetWIFILED(FALSE);
                    LastLED = 0;
                    if(BTisON || WIFIisON)
                    {
                        TimeOut = 1000;
                        if(BTisON && WIFIisON)
                        {
                            LastLED = BT_LED_ON;
                            SetBluetoothLED(TRUE);
                        }
                        else if(BTisON)
                        {
                            LastLED = 0;
                            SetBluetoothLED(TRUE);
                        }
                        else if(WIFIisON)
                        {
                            LastLED = 0;
                            SetWIFILED(TRUE);
                        }
                    }
                    else
                    {
                        TimeOut = INFINITE;
                    }
                    break;

                case 1000:
                    TimeOut = 3000;
                    if(LastLED)
                    {
                        (LastLED == BT_LED_ON) ? SetBluetoothLED(FALSE) :   SetWIFILED(FALSE);
                    }
                    else if(BTisON)
                    {
                        SetBluetoothLED(FALSE);
                    }
                    else if(WIFIisON)
                    {
                        SetWIFILED(FALSE);
                    }
                    break;

                case 3000:
                    TimeOut = 1000;
                    if(LastLED)
                    {
                        LastLED = (LastLED == BT_LED_ON) ? WIFI_LED_ON : BT_LED_ON;
                        (LastLED == BT_LED_ON) ? SetBluetoothLED(TRUE) :   SetWIFILED(TRUE);
                    }
                    else if(BTisON)
                    {
                        SetBluetoothLED(TRUE);
                    }
                    else if(WIFIisON)
                    {
                        SetWIFILED(TRUE);
                    }
                    break;

                default:
                    break;
            }
        }
    }

    return 0;
}
