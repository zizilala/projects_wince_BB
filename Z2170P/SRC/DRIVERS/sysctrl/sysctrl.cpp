#include <windows.h>
/*#include <bt_api.h>
#include "bsp.h"
#include "sysctrl.h"
#include "runtime_context.h"
#include "GPIO_SOC.h"
*/
HINSTANCE ghInstance;
/*
extern "C" BOOL Init_LED(void);

extern "C" BOOL SysCtrl_BCRGetHWPower(void);

extern "C" BOOL SysCtrl_BCRSetHWPower(BOOL dwState);

extern "C" DWORD SYSTEM_POWER_Thread(LPVOID pvarg);

extern "C" DWORD LEDThread(LPVOID pvarg);

extern "C" BOOL BTSetHWPower(BOOL dwState);

extern "C" BOOL SysCtrl_BCR_HW_Trigger(BOOL OnOff);
*/
extern DWORD StartupThread(LPVOID pvarg);
/*
extern DWORD TrayIconThread(LPVOID pvarg);

extern DWORD BluetoothNotifyThread(LPVOID pvarg);

extern DWORD NpwStartNotification(LPVOID pUnused);

extern BOOL ChkBuiltInRFID(void);

extern "C" BOOL FlashInit(void);
extern "C" BOOL ReadFlashData(DWORD Offset,DWORD DataSize,PBYTE lpTargetData);
extern "C" BOOL WriteFlashData(DWORD Offset,DWORD DataSize,PBYTE lpSourceData);
extern "C" BOOL RFIDSetHWPower(BOOL dwState);
extern "C" BOOL RFIDGetHWPower(void);
extern "C" BOOL VibratorSetHWPower(BOOL dwState);
extern "C" BOOL VibratorGetHWPower(void);

static BOOL NANDInit=FALSE;
*/
BOOL WINAPI
DllMain(
    HANDLE hinstDll,
    DWORD dwReason,
    LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);
	
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            ghInstance = (HINSTANCE) hinstDll;
            break;

        case DLL_PROCESS_DETACH:
            //RETAILMSG(1, (TEXT("IoDispatch_DllEntry: DLL_PROCESS_DETACH\r\n")));
            break;

        case DLL_THREAD_ATTACH:
            //RETAILMSG(1, (TEXT("IoDispatch_DllEntry: DLL_THREAD_ATTACH\r\n")));
            break;

        case DLL_THREAD_DETACH:
            //RETAILMSG(1, (TEXT("IoDispatch_DllEntry: DLL_THREAD_DETACH\r\n")));
            break;
    }

    // return TRUE for success, FALSE for failure
    return TRUE;
}

DWORD SYS_Init(DWORD dwData)
{
	UNREFERENCED_PARAMETER(dwData);
	//Init_LED();

    HANDLE hStartup = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) StartupThread, NULL, 0, NULL);
    CloseHandle(hStartup);

    //HANDLE hPower = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) SYSTEM_POWER_Thread, NULL, 0, NULL);
    //CloseHandle(hPower);

    //HANDLE hTrayIcon = CreateThread(NULL, 0, TrayIconThread, NULL, 0, NULL);
    //CloseHandle(hTrayIcon);

    //HANDLE hLED = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) LEDThread, NULL, 0, NULL);
    //CloseHandle(hLED);

    //HANDLE hBTThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) BluetoothNotifyThread, NULL, 0, NULL);
    //CloseHandle(hBTThread);

    //HANDLE hWiFiThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)NpwStartNotification, NULL, 0, NULL);
    //CloseHandle(hWiFiThread);

    return 1;
}


BOOL SYS_Deinit(DWORD dwData)
{
	UNREFERENCED_PARAMETER(dwData);
    return TRUE;
}



BOOL
SYS_Open(
    DWORD dwData,
    DWORD dwAccess,
    DWORD dwShareMode)
{
	UNREFERENCED_PARAMETER(dwData);
	UNREFERENCED_PARAMETER(dwAccess);
	UNREFERENCED_PARAMETER(dwShareMode);
    return TRUE;
}


BOOL
SYS_Close(DWORD dwData)
{
	UNREFERENCED_PARAMETER(dwData);
    return TRUE;
}



DWORD
SYS_Read(
    DWORD dwData,
    unsigned int *pBuffer,
    DWORD dwLen)
{
	UNREFERENCED_PARAMETER(dwData);
	UNREFERENCED_PARAMETER(pBuffer);
	UNREFERENCED_PARAMETER(dwLen);
    return 0;
}


DWORD
SYS_Write(
    DWORD dwData,
    unsigned int *pBuffer,
    DWORD dwInLen)
{
	UNREFERENCED_PARAMETER(dwData);
	UNREFERENCED_PARAMETER(pBuffer);
	UNREFERENCED_PARAMETER(dwInLen);
    return 0;
}


DWORD
SYS_Seek(
    DWORD dwData,
    long pos,
    DWORD type)
{
	UNREFERENCED_PARAMETER(dwData);
	UNREFERENCED_PARAMETER(pos);
	UNREFERENCED_PARAMETER(type);
    return 0;
}


BOOL
SYS_PowerUp(void)
{
    return TRUE;
}

BOOL
SYS_PowerDown(void)
{
    return TRUE;
}

BOOL
SYS_IOControl(
    DWORD dwInst,
    DWORD dwIoControlCode,
    LPVOID lpInBuf,
    DWORD nInBufSize,
    LPVOID lpOutBuf,
    DWORD nOutBufSize,
    LPDWORD lpBytesReturned)
{
    //BOOL dwState = FALSE;
	//uNandFLASH *lpFPro=NULL;
	//NandRead *lpNRead=NULL;
	UNREFERENCED_PARAMETER(dwInst);
	UNREFERENCED_PARAMETER(lpInBuf);
	UNREFERENCED_PARAMETER(nInBufSize);
	UNREFERENCED_PARAMETER(lpOutBuf);
	UNREFERENCED_PARAMETER(nOutBufSize);
	UNREFERENCED_PARAMETER(lpBytesReturned);
	UNREFERENCED_PARAMETER(dwIoControlCode);
		
 //   switch(dwIoControlCode)
//	{
/*
        case ZSYS_GetBootInformation:

            *(volatile WORD *)lpOutBuf = *(volatile WORD *)BOOT_MODE_STORE_ADDR;

            *lpBytesReturned = sizeof(WORD);

            break;


        case ZSYS_GetSystemInformation:

            memcpy((BYTE *)lpOutBuf, (BYTE *)SYSTEM_INFORMATION_MEMORY_START, sizeof(SysInfor));

            *lpBytesReturned = sizeof(SysInfor);
            break;


        case ZSYS_SetSystemInformation:

            memcpy((BYTE *)SYSTEM_INFORMATION_MEMORY_START, (BYTE *)lpInBuf, sizeof(SysInfor));

            return TRUE;

		case ZSYS_GetSerialNumber:
			//RETAILMSG(1, (TEXT("SYS_IOControl: ZSYS_GetSerialNumber\r\n")));
			memcpy((BYTE *)lpOutBuf, (BYTE *)SERIAL_NUMBER_STORE_ADDR, 24);
			return TRUE;
			
        case ZSYS_GetBarcodePower:

            *(PBOOL)lpOutBuf = SysCtrl_BCRGetHWPower();

            *lpBytesReturned = sizeof(BOOL);

            return TRUE;


        case ZSYS_SetBarcodePower:

            dwState = *(PBOOL)lpInBuf;

            return SysCtrl_BCRSetHWPower(dwState);

		case ZSYS_GetBluetoothPower:
			*(PBOOL)lpOutBuf = (*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR & BT_POWER_MASK) ? TRUE : FALSE;
			*lpBytesReturned = sizeof(BOOL);
			return TRUE;

        case ZSYS_SetBluetoothPower:
            dwState = *(PBOOL)lpInBuf;
            return BTSetHWPower(dwState);
            
		case ZSYS_GetWIFIState:
			*(PBOOL)lpOutBuf= (*(volatile WORD *)BUILD_IN_DEVICE_FLAG_ADDR&WIFI_BUILD_IN_MASK)? TRUE : FALSE;
			*lpBytesReturned=sizeof(BOOL);
			return TRUE;	        

		case ZSYS_ClearWIFIState:
			*(volatile WORD *)BUILD_IN_DEVICE_FLAG_ADDR&=~WIFI_BUILD_IN_MASK;
			return TRUE;

		case ZSYS_GetRFIDPower:
            *(PBOOL)lpOutBuf = RFIDGetHWPower();
            *lpBytesReturned = sizeof(BOOL);
            return TRUE;

        case ZSYS_SetRFIDPower:
            dwState = *(PBOOL)lpInBuf;
            //if( RFIDSetHWPower(dwState) ==  FALSE )
            if( SysCtrl_BCRSetHWPower(dwState) ==  FALSE )
            	return FALSE;
            if( dwState )
            {
            	PXA_OST_DelayMilliSeconds(1000);
            	if(	!ChkBuiltInRFID() )
            	{
            		PXA_OST_DelayMilliSeconds(1000);
            		ChkBuiltInRFID();
            	}
            }
            return TRUE;
            
		case ZSYS_GetVIBRATORPower:
            *(PBOOL)lpOutBuf = VibratorGetHWPower();
            *lpBytesReturned = sizeof(BOOL);
            return TRUE;

        case ZSYS_SetVIBRATORPower:
            dwState = *(PBOOL)lpInBuf;
            if( VibratorSetHWPower(dwState) ==  FALSE )
            	return FALSE;            
            return TRUE;
            
        case ZSYS_Run_SW_Reset:
            KernelIoControl(IOCTL_HAL_REBOOT, NULL, 0, NULL, 0, NULL);
            return FALSE;
        
        case ZSYS_BCR_HW_Trigger:
			dwState = *(PBOOL)lpInBuf;
			SysCtrl_BCR_HW_Trigger(dwState);
            return TRUE;

		case ZSYS_WRITE_FLASH:
			if(NANDInit==FALSE)
				NANDInit=FlashInit();
			if(NANDInit==FALSE)
				return FALSE;
			lpFPro = (uNandFLASH *)lpInBuf;
			if(lpFPro==NULL) 
				return FALSE;  
			//RETAILMSG(1, (TEXT("SYS_WRITE_FLASH : 0x%x - 0x%x\r\n"),lpFPro->Offset,lpFPro->DataSize));
			return WriteFlashData(lpFPro->Offset,lpFPro->DataSize,lpFPro->lpBuffer);

		case ZSYS_READ_FLASH:
			//RETAILMSG(1, (TEXT("SYS_READ_FLASH\r\n")));
			if(NANDInit==FALSE)
				NANDInit = FlashInit();
			if(NANDInit==FALSE)
				return FALSE;
			lpNRead = (NandRead *)lpInBuf;
			if(lpNRead==NULL) 
				return FALSE;
			*lpBytesReturned=lpNRead->DataSize;
			//RETAILMSG(1, (TEXT("SYS_READ_FLASH : 0x%x - 0x%x\r\n"),lpNRead->Offset,lpNRead->DataSize));
			return ReadFlashData(lpNRead->Offset,lpNRead->DataSize,(PBYTE)lpOutBuf);	
*/
//		default:
	//		return FALSE;
    //}

    return TRUE;
}


