#include <windows.h>
#include "bsp.h"
#include "clkmgr.h"
#include "runtime_context.h"
#include "GPIO_SOC.h"
#include "GPIO_Platform.h"
#include "mfp_soc.h"
#include "BCRScanCtrl.h"
#include "MFP_DRV.h"


volatile PXA_GPIOREG_T  *g_pGPIORegisters = NULL;

volatile UINT32 *g_pMFPBase = NULL;

volatile UINT32 *lpPMER = NULL;

#define QUEUE_ENTRIES   3
#define MAX_NAMELEN     200
#define QUEUE_SIZE      (QUEUE_ENTRIES * (sizeof(POWER_BROADCAST) + MAX_NAMELEN))

void DisableUart3_Pin();
void DisableBTUart_Pin();
BOOL BTDrvLoad();
BOOL BTDrvUnload();

void MapResources(void)
{
    if(g_pGPIORegisters == NULL)
    {
        g_pGPIORegisters = (volatile PXA_GPIOREG_T *)PXA_CTX_GetRegAddr(PXA_PERIPHERAL_REGIDX_GPIO);
    }

    if(g_pMFPBase == NULL)
    {
        g_pMFPBase = (volatile UINT32 *)PXA_CTX_GetRegAddr(PXA_PERIPHERAL_REGIDX_MFP);
    }

    if(lpPMER == NULL)
    {
        lpPMER = (volatile UINT32 *)PXA_CTX_GetRegAddr(PXA_PERIPHERAL_REGIDX_PMER);
    }

}

void EnableGPIOReset(void)
{
    if(lpPMER)
    {
        *lpPMER = 0x00;
    }
}

BOOL SysCtrl_BCRGetHWPower(void)
{
    PXA_LEVEL_T GPIOLevel;

    if(!g_pGPIORegisters || !g_pMFPBase)
    {
        MapResources();
    }

    if(!g_pGPIORegisters || !g_pMFPBase)
    {
        return FALSE;
    }

    PXA_GPIOGetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_93, &GPIOLevel);

    return GPIOLevel;
}

BOOL SysCtrl_BCRSetHWPower(BOOL dwState)
{
    if(!g_pGPIORegisters || !g_pMFPBase)
    {
        MapResources();
    }

    if(!g_pGPIORegisters || !g_pMFPBase)
    {
        return FALSE;
    }
    
    if(dwState)
    {	// BC_PWREN
    	RETAILMSG(1, (TEXT("SysCtrl_BCRSetHWPower - ON\r\n")));
		MFP_SetActiveMode(PXA_COMPONENT_UART3_ID);
		PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO93_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
		PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_93, PXA_GPIO_DIRECTION_OUT);
		PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_93, PXA_HI);
		PXA_MFPConfigureLpmOutputLevel(g_pMFPBase, PXA_MFP_PIN_GPIO93_OFFSET, PXA_MFP_LPMO_DRIVE_LOW);  
       
		// BC_nTRIG
		PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO94_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
		PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_94, PXA_GPIO_DIRECTION_OUT);
		PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_94,PXA_HI); 
        
		*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR|=BCR_POWER_MASK;                         
	}
    else
    {
    	RETAILMSG(1, (TEXT("SysCtrl_BCRSetHWPower - OFF\r\n")));
		DisableUart3_Pin();
		*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR&=~BCR_POWER_MASK;
    }
    
    return TRUE;
}


BOOL GetTouchPanelState(void)
{
    RETAILMSG(1, (TEXT("\n\r+GetTouchPanelState (%x) \r\n"), *(volatile BYTE *)TOUCH_PANEL_ENABLE_STATE_STORE_ADDR));

    return (*(volatile BYTE *)TOUCH_PANEL_ENABLE_STATE_STORE_ADDR) ? TRUE : FALSE;

}


BOOL SetTouchPanelState(BOOL dwEnableStatus)
{
    RETAILMSG(1, (TEXT("\n\r+GetTouchPanelState (%x) \r\n"), dwEnableStatus));

    *(volatile BYTE *)TOUCH_PANEL_ENABLE_STATE_STORE_ADDR = dwEnableStatus;

    RETAILMSG(1, (TEXT("\n\r-GetTouchPanelState (%x) \r\n"), *(volatile BYTE *)TOUCH_PANEL_ENABLE_STATE_STORE_ADDR));

    return TRUE;

}


DWORD SYSTEM_POWER_Thread(LPVOID pvarg)
{


    HANDLE hMsgQ;
    MSGQUEUEOPTIONS Options;

    DWORD dwNumberOfBytesRead;
    DWORD dwErr;
    DWORD flags;

    BYTE buf[QUEUE_SIZE];

    HANDLE hbklNotifications =  CreateEvent(NULL, FALSE, FALSE, LCD_BACKLIGHT_ACTIVE_NOTIFY);

    /*while(!IsAPIReady(SH_SHELL))
    {
        Sleep(1000);
    }*/
    WaitForAPIReady(SH_SHELL, INFINITE);

    memset(&Options, 0, sizeof(Options));

    Options.dwSize = sizeof(MSGQUEUEOPTIONS);
    Options.dwFlags = 0;
    Options.dwMaxMessages = QUEUE_ENTRIES;
    Options.cbMaxMessage = sizeof(POWER_BROADCAST) + MAX_NAMELEN;
    Options.bReadAccess = TRUE;


    hMsgQ = CreateMsgQueue(NULL, &Options);

    if(!hMsgQ)
    {
        ERRORMSG(1, (TEXT("pwrtest:CreateMsgQueue failed\r\n")));
        return 0;
    }

    if(!RequestPowerNotifications(hMsgQ, POWER_NOTIFY_ALL))
    {
        ERRORMSG(1, (TEXT("pwrtest:Register Power notification events failed\r\n")));
        return 0;
    }

	RETAILMSG(1, (TEXT("SYSTEM_POWER_Thread Start!!!!\r\n")));
	
    while(1)
    {
        WaitForSingleObject(hMsgQ, INFINITE);

        if(!ReadMsgQueue(hMsgQ,
                         &buf,
                         QUEUE_SIZE,
                         &dwNumberOfBytesRead,
                         INFINITE,
                         &flags))
        {
            ERRORMSG(1, (TEXT("pwrtest:ReadMsgQueue failed\r\n")));
            dwErr = GetLastError();

            switch(dwErr)
            {
                case ERROR_INSUFFICIENT_BUFFER:
                    ERRORMSG(1, (TEXT("pwrtest:ERROR_INSUFFICIENT_BUFFER\r\n")));
                    break;
                case ERROR_PIPE_NOT_CONNECTED:
                    ERRORMSG(1, (TEXT("pwrtest:ERROR_PIPE_NOT_CONNECTED\r\n")));
                    break;
                case ERROR_TIMEOUT:
                    ERRORMSG(1, (TEXT("pwrtest:EEOR_TIMEOUT\r\n")));
                    break;
                default:
                    ERRORMSG(1, (TEXT("pwrtest:ERROR_UNKNOWN\r\n")));
            }
        }
        else if(dwNumberOfBytesRead >= sizeof(POWER_BROADCAST))
        {
            PPOWER_BROADCAST pB = (PPOWER_BROADCAST)&buf;


            if(pB->Message == PBT_RESUME)
            {
                RETAILMSG(1, (TEXT("\n\rPBT_RESUME\r\n")));




                if(hbklNotifications)
                {
                    SetEvent(hbklNotifications);
                }


            }//end of PBT_RESUME



            if((pB->Message == PBT_TRANSITION) &&
               (POWER_STATE(pB->Flags) == POWER_STATE_SUSPEND)
              )
            {
                /*

                BOOL  BTPowerState;

                if(SysGetBluetoothPower(&BTPowerState))
                {
                 if(BTPowerState)
                      SysSetBluetoothPower(FALSE);

                }	*/

            }

        }
    }

    return 0;
}

void DisableBTUart_Pin()
{
	//GPIO112 BT_RXD
	PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO112_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_112, PXA_GPIO_DIRECTION_OUT);
	PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_112,PXA_LO);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_112, PXA_GPIO_DIRECTION_IN);

	//GPIO113 BT_TXD
	PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO113_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_113, PXA_GPIO_DIRECTION_OUT);
	PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_113,PXA_LO);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_113, PXA_GPIO_DIRECTION_IN);
	
	//GPIO114 BT_CTS
	PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO114_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_114, PXA_GPIO_DIRECTION_OUT);
	PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_114,PXA_LO);
	//PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_114, PXA_GPIO_DIRECTION_IN);
  
	//GPIO111 BT_RTS
	PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO111_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_111, PXA_GPIO_DIRECTION_OUT);
	PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_111,PXA_LO);
	//PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_111, PXA_GPIO_DIRECTION_IN);

	//GPIO91 BT_nRST
	PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO91_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_91, PXA_GPIO_DIRECTION_OUT);
	PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_91,PXA_LO);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_91, PXA_GPIO_DIRECTION_IN);    
                         
	//GPIO92 BT_PWREN
	PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO92_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_92, PXA_GPIO_DIRECTION_OUT);
	PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_92,PXA_LO); 
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_92, PXA_GPIO_DIRECTION_IN);      

}

void DisableUart3_Pin()
{
	//GPIO107 ENF_CTS
	PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO107_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_107, PXA_GPIO_DIRECTION_OUT);
	PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_107,PXA_LO);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_107, PXA_GPIO_DIRECTION_IN);

	//GPIO108 ENG_RTS
	PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO108_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_108, PXA_GPIO_DIRECTION_OUT);
	PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_108,PXA_LO);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_108, PXA_GPIO_DIRECTION_IN);
     
	//GPIO109 ENG_TXD
	PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO109_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_109, PXA_GPIO_DIRECTION_OUT);
	PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_109,PXA_LO);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_109, PXA_GPIO_DIRECTION_IN);

	//GPIO110 ENG_RXD
	PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO110_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_110, PXA_GPIO_DIRECTION_OUT);
	PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_110,PXA_LO);               
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_110, PXA_GPIO_DIRECTION_IN);  

	//GPIO93 BC_PWREN
	PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO93_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_93, PXA_GPIO_DIRECTION_OUT);
	PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_93,PXA_LO);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_93, PXA_GPIO_DIRECTION_IN);
                         
	//GPIO94 BC_nTRIG
	PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO94_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_94, PXA_GPIO_DIRECTION_OUT);
	PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_94,PXA_LO);                       
	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_94, PXA_GPIO_DIRECTION_IN);
}

BOOL BTDrvLoad()
{
  BOOL status = FALSE;
  HANDLE hDevice ;
  HKEY hk;
  
  hDevice = ActivateDevice (L"Software\\Microsoft\\Bluetooth\\Driver", 0);

		if (hDevice) {
			
			if (ERROR_SUCCESS == RegOpenKeyEx (HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Bluetooth", 0, KEY_READ, &hk)) {
				RegSetValueEx (hk, L"btd_handle", 0, REG_DWORD, (LPBYTE)&hDevice, sizeof(hDevice));
				RegCloseKey (hk);
				status= TRUE;
			}
		}  
  return status;  
}

BOOL BTDrvUnload()
{
		HKEY hk;
		BOOL status=FALSE;
		HANDLE hDevice = NULL;
		DWORD dwSize,dwType;
		
		if (ERROR_SUCCESS == RegOpenKeyEx (HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Bluetooth", 0, KEY_READ, &hk)) {
			
			dwSize = sizeof (hDevice);

			if ((ERROR_SUCCESS == RegQueryValueEx (hk, L"btd_handle", NULL, &dwType, (LPBYTE)&hDevice, &dwSize)) &&
				(dwType == REG_DWORD) && (dwSize == sizeof (hDevice)) && hDevice) {
				DeactivateDevice(hDevice);
				RegDeleteValue (hk, L"btd_handle");
			}
			RegCloseKey (hk);
			status = TRUE;
		}
		
		return status;  
  
}

void SysCtrl_BCR_HW_Trigger(BOOL OnOff)
{
   
   PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, 
                         PXA_GPIO_94, 
                         PXA_GPIO_DIRECTION_OUT);
   //Low avtice                                     
    PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_94,OnOff?PXA_LO:PXA_HI);   
        
}

BOOL BTSetHWPower(BOOL dwState)
{
    if(!g_pGPIORegisters || !g_pMFPBase)
    {
        MapResources();
    }

    if(!g_pGPIORegisters || !g_pMFPBase)
    {
        return FALSE;
    }
        
    RETAILMSG(1, (TEXT("+BTSetHWPower: %d\r\n"), dwState));

    if(dwState)
    {
		MFP_SetActiveMode(PXA_COMPONENT_BTUART_ID);

		// BT_PWRREN
		PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_92, PXA_GPIO_DIRECTION_OUT);
		PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO92_OFFSET, PXA_MFP_ALT_FN_0, PXA_MFP_DS_03X);
		PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_92, PXA_HI);
		
		// BT_nRST
		PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_91, PXA_GPIO_DIRECTION_OUT);
		PXA_MFPSetAfDs(g_pMFPBase, PXA_MFP_PIN_GPIO91_OFFSET, PXA_MFP_ALT_FN_0,PXA_MFP_DS_03X);     

		PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_91, PXA_LO);
		Sleep(50);
		PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_91, PXA_HI);  
    
		BTDrvLoad();    
	}
    else
    {
		BTDrvUnload();
		DisableBTUart_Pin();
    }
	RETAILMSG(1, (TEXT("-BTSetHWPower.\r\n")));
	
	return TRUE;
}

BOOL RFIDSetHWPower(BOOL dwState)
{
    if(!g_pGPIORegisters || !g_pMFPBase)
    {
        MapResources();
    }

    if(!g_pGPIORegisters || !g_pMFPBase)
    {
        return FALSE;
    }
        
    RETAILMSG(1, (TEXT("+RFIDSetHWPower: %d\r\n"), dwState));

    if(dwState)
    {
    	PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_93, PXA_GPIO_DIRECTION_OUT);
		PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_93, PXA_HI);
		PXA_OST_DelayMilliSeconds(1000);
	}
    else
    {
		PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_93, PXA_GPIO_DIRECTION_OUT);
		PXA_GPIOSetLevel((PXA_GPIOREG_T *)g_pGPIORegisters,PXA_GPIO_93,PXA_LO);
		PXA_GPIOSetDirection((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_93, PXA_GPIO_DIRECTION_IN);
    }
	RETAILMSG(1, (TEXT("-RFIDSetHWPower.\r\n")));
	
	return TRUE;
}

BOOL RFIDGetHWPower(void)
{
    PXA_LEVEL_T GPIOLevel;

    if(!g_pGPIORegisters || !g_pMFPBase)
    {
        MapResources();
    }
    if(!g_pGPIORegisters || !g_pMFPBase)
    {
        return FALSE;
    }

    PXA_GPIOGetLevel((PXA_GPIOREG_T *)g_pGPIORegisters, PXA_GPIO_93, &GPIOLevel);

    return GPIOLevel;
}

BOOL VibratorGetHWPower(void)
{
    return (*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR & VIBRATORI_POWER_MASK) ? TRUE : FALSE;
}

BOOL VibratorSetHWPower(BOOL dwState)
{            
    RETAILMSG(1, (TEXT("+VibratorSetHWPower: %d\r\n"), dwState));
    if(dwState)
    {
    	*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR |= VIBRATORI_POWER_MASK;
	}
    else
    {
		*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR &= ~VIBRATORI_POWER_MASK;
    }
	RETAILMSG(1, (TEXT("-VibratorSetHWPower.\r\n")));

	return TRUE;
}
