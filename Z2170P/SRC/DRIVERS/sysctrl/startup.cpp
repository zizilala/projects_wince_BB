#include <windows.h>
//#include <bt_api.h>
//#include <Storemgr.h>
#include <windev.h>
//#include <ndis.h>
//#include <windev.h>
//#include <nuiouser.h>
//#include <msgqueue.h>
//#include <ndispwr.h>
//#include <ras.h>

//#include "bsp.h"

//#include "..\SDK\LIB\SystemCtrl\SystemCtrl.h"
//#include "..\SDK\LIB\PowerCtl\PowerCtl.h"
//#include "SSI.h"
//#include "notify.h"	//20111007 luke
//#include "ZEBEX.h"

//#define ActiveSyncAppName L"repllog.exe"	//20111007 luke

//DWORD PreloadThread(LPVOID pvarg);


//extern "C"  HANDLE BT_LED_Event;


//extern BOOL AddBluetoothTrayIcon(void);

//extern BOOL RemoveBluetoothTrayIcon(void);

//extern BOOL ChkBuiltInRFID(void);
/*
void Set_BT_Power_2_Register(BOOL IsBTOn)
{
	HKEY hBTReg = NULL;
	DWORD dwDisp;

	if(RegCreateKeyEx( HKEY_LOCAL_MACHINE ,TEXT("Drivers\\BuiltIn\\BT_UART"), 0, NULL, REG_OPTION_NON_VOLATILE,
						KEY_ALL_ACCESS, NULL, &hBTReg, &dwDisp)==ERROR_SUCCESS)
	{
		if(RegSetValueEx( hBTReg,TEXT("PowerStatus"),0,REG_DWORD,(LPBYTE)&IsBTOn, sizeof(DWORD))!=ERROR_SUCCESS)
		{  
			RETAILMSG(1, (TEXT("Set_BT_Power_2_Register: Set IsBTOn fail!\r\n")));
		}
		else
			RETAILMSG(1, (TEXT("Set_BT_Power_2_Register: IsBTOn = %d\r\n"),IsBTOn)); 
	}
	else
		RETAILMSG(1, (TEXT("Set_BT_Power_2_Register: RegCreateKeyEx fail!!!\r\n"))); 
	RegCloseKey(hBTReg);
}

BOOL Get_BT_Power_form_Register()
{
	HKEY  hBT = NULL;
	DWORD dwType,dwSize;
	BOOL IsBTOn = FALSE;    
    
	dwType = REG_DWORD;
	dwSize = sizeof(DWORD);
	
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Drivers\\BuiltIn\\BT_UART"), 0,0, &hBT) == ERROR_SUCCESS)  	
	{
		if(RegQueryValueEx( hBT,TEXT("PowerStatus"),NULL,&dwType,(PBYTE)&IsBTOn,&dwSize)!=ERROR_SUCCESS)
		{	
			IsBTOn = FALSE;
		}
	}
	RETAILMSG (1, (TEXT("Get_BT_Power_form_Register:IsBTOn = %d\r\n"),IsBTOn)); 
	RegCloseKey(hBT);  
	return IsBTOn;
}

BOOL Get_WLAN_Power_form_Register()
{
	HKEY  hWLAN = NULL;
	DWORD dwType,dwSize;
	BOOL IsWLANOn = FALSE;    
    
	dwType = REG_DWORD;
	dwSize = sizeof(DWORD);
	
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Drivers\\BuiltIn\\SDHC"), 0,0, &hWLAN) == ERROR_SUCCESS)  	
	{
		if(RegQueryValueEx( hWLAN,TEXT("PowerStatus"),NULL,&dwType,(PBYTE)&IsWLANOn,&dwSize)!=ERROR_SUCCESS)
		{	
			IsWLANOn = FALSE;
		}
	}
	RETAILMSG (1, (TEXT("Get_WLAN_Power_form_Register:IsWLANOn = %d\r\n"),IsWLANOn)); 
	RegCloseKey(hWLAN);  
	return IsWLANOn;
}
*/
void LaunchProgram(LPCTSTR szProgramFile, LPCTSTR szParameter)
{
    //HANDLE hndFile = INVALID_HANDLE_VALUE;
    SHELLEXECUTEINFO shei;

	memset(&shei, 0, sizeof(shei));
	shei.cbSize   = sizeof(shei);
	shei.fMask    = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
	shei.lpVerb   = L"open";
	shei.nShow = SW_SHOW;
	shei.hInstApp = NULL;
	shei.lpFile = szProgramFile;
	shei.lpParameters = szParameter;

	ShellExecuteEx(&shei);
}
/*
BOOL IsProcessRunning(LPTSTR lpszFileName)
{
    HANDLE hSnapshot;
    PROCESSENTRY32 pe;
    wchar_t	*pstr;
    BOOL ret = FALSE;
    BOOL result = FALSE;

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if(hSnapshot == (HANDLE)1)
    {
        return ret;
    }

    memset(&pe, 0, sizeof(pe));
    pe.dwSize = sizeof(pe);
    result = Process32First(hSnapshot, &pe);

    if(!result)
    {
        CloseToolhelp32Snapshot(hSnapshot);
        return ret;
    }

    do
    {
        pstr = wcsstr(pe.szExeFile, lpszFileName);

        if(pstr)
        {
            // RETAILMSG (1,(TEXT("\n\r%s is rinning\n\r"),lpszFileName));
            ret = TRUE;
            break;
        }

        result = Process32Next(hSnapshot, &pe);
    }
    while(result);

    CloseToolhelp32Snapshot(hSnapshot);
    return ret;
}

void InitDefaultTime(void)
{
    const SYSTEMTIME st = {2013, 1, 0, 1, 12, 0, 0, 0};

    DWORD bytesUsed;

    KernelIoControl(IOCTL_HAL_INIT_RTC, (UCHAR *)&st, sizeof(SYSTEMTIME), NULL, 0, &bytesUsed);


    //LaunchProgram(L"ctlpnl.exe",L"\\windows\\cplmain.cpl,13");

}*/
/*
void SetSystemMrmory(void)
{
    DWORD StorePages, RamPages, PageSize;
    STORE_INFORMATION  StoreInfo;

    GetSystemMemoryDivision(&StorePages, &RamPages, &PageSize);

    GetStoreInformation(&StoreInfo);
    StoreInfo.dwStoreSize /= PageSize;
    StoreInfo.dwFreeSize /= PageSize;

	StorePages-= (StoreInfo.dwFreeSize/3);


    SetSystemMemoryDivision(StorePages);
}

extern "C" BOOL SysCtrl_BCRSetHWPower(BOOL dwState);
*/
DWORD StartupThread(LPVOID pvarg)
{
	UNREFERENCED_PARAMETER(pvarg);
//	BYTE EngType;
	
    HANDLE hShellAPI = CreateEvent(NULL, FALSE, FALSE, _T("SYSTEM/ShellAPIReady"));
    WaitForSingleObject(hShellAPI, INFINITE);
    CloseHandle(hShellAPI);  
/*
		
	CeRunAppAtEvent(ActiveSyncAppName,NOTIFICATION_EVENT_NONE);	//20111007 luke
    CeRunAppAtEvent(ActiveSyncAppName,NOTIFICATION_EVENT_RS232_DETECTED);	//20111007 luke
	
	if( (*(volatile WORD *)BOOT_MODE_STORE_ADDR&ARSR_HW_RESET) )
	{
		InitDefaultTime();
		//SetSystemMrmory();  
	}
		
	if( *(volatile BYTE *)BCR_ENGINE_TYPE_STORE_ADDR == BCR_ENGINE_UNKNOW )
	{
		RETAILMSG(1, (TEXT("********************SSI_GetEngType***************************\n\r")));
		SysCtrl_BCRSetHWPower(TRUE);	// GPIO_93 High
		SSI_GetEngType(&EngType);
		if( (EngType == BCR_ENGINE_UNKNOW) )// || (EngType == BCR_ENGINE_ZEBEX_Z5111) )
		{
			RETAILMSG(1, (TEXT("********************ZEBEX_GetEngType***************************\n\r")));
			ZEBEX_GetEngType(&EngType);
		}
	
		if( (EngType == BCR_ENGINE_UNKNOW) )//|| (EngType == RFID_ENGINE_JOGTEK) )
		{
			RETAILMSG(1, (TEXT("********************ChkBuiltInRFID***************************\n\r")));
			if(	ChkBuiltInRFID() )
				EngType = RFID_ENGINE_JOGTEK;
			else
				EngType = BCR_ENGINE_UNKNOW;
			SysCtrl_BCRSetHWPower(FALSE);
		}
		*(volatile BYTE *)BCR_ENGINE_TYPE_STORE_ADDR = EngType;
	}
	else
	{
		SysCtrl_BCRSetHWPower(FALSE);
		EngType = *(volatile BYTE *)BCR_ENGINE_TYPE_STORE_ADDR;
	}
	
	BCRSaveEngineType(EngType);
	
	if( EngType == RFID_ENGINE_JOGTEK )
	{*/
		/*RETAILMSG(1, (TEXT("----------------ChkBuiltInRFID---------------\n\r")));
		SysCtrl_BCRSetHWPower(TRUE);	// GPIO_93 High
		if(	ChkBuiltInRFID() )
				EngType = RFID_ENGINE_JOGTEK;
			else
				EngType = BCR_ENGINE_UNKNOW;*/
	/*}
	else
	{
		TCHAR szRFIDProm[MAX_PATH];
		SHGetSpecialFolderPath(NULL, szRFIDProm, CSIDL_PROGRAMS , FALSE);
		wcscat(szRFIDProm, L"\\PowerPack\\RFID.lnk");
		DeleteFile(szRFIDProm);
	}
	if( EngType != BCR_ENGINE_UNKNOW )
		RETAILMSG(1, (TEXT("Found Engine: %d\n\r"),EngType));
	
    RETAILMSG(1, (TEXT("\n\r*** Start StartupThread ***\n\r")));
  
    //******* wait iNand ready *******
    WIN32_FIND_DATA file_pro;
    HANDLE  hChkFile = INVALID_HANDLE_VALUE;
    BOOL FlashReady = FALSE;
    DWORD dwTickCount = GetTickCount();

    do
    {
        FlashReady = SysFindDisk(L"Built-In Flash");
        if(FlashReady)
        {
            hChkFile = FindFirstFile(TEXT("\\Flash Disk"), &file_pro);
            if(hChkFile != INVALID_HANDLE_VALUE)
            {
                break;
            }
        }
		else
        {
            RETAILMSG(1, (TEXT("\n\n\r*** Waiting Built-In Flash Ready ***\n\r")));
        }
        if((GetTickCount() - dwTickCount) > 5000)
        {
            break;
        }
        Sleep(1000);
    } while(1);


    //******* check "Flash Disk" folder *******

    if(hChkFile == INVALID_HANDLE_VALUE)
    {
        RETAILMSG(1, (TEXT("\n\n\r*** No Flash Disk folder ***\n\r")));
    }
    else
    {
        RETAILMSG(1, (TEXT("\n\n\r*** Found Flash Disk folder ***\n\r")));
        
    }
        
    if((*(volatile WORD *)BOOT_MODE_STORE_ADDR & SYSTEM_CLEAN_BOOT) || (hChkFile == INVALID_HANDLE_VALUE) )
    {
        *(volatile WORD *)BOOT_MODE_STORE_ADDR |= SYSTEM_CLEAN_BOOT;

        if(FlashReady)
        {
            LaunchProgram(L"FlashFormat.exe", NULL);
        }
        while(IsProcessRunning(L"FlashFormat.exe"))
        {
            Sleep(1000);
        }
        *(volatile WORD *)BOOT_MODE_STORE_ADDR &= ~SYSTEM_CLEAN_BOOT;
    }
    FindClose(hChkFile);

    //LaunchProgram(L"rassync.exe",NULL);
    
	//if ( (EngType == BCR_ENGINE_SSI_955) || (EngType == BCR_ENGINE_SSI_4500) || (EngType == BCR_ENGINE_ZEBEX_Z5111) )
	LaunchProgram(L"BCRService.exe", NULL);
*/
    LaunchProgram(L"KeyHook.exe", NULL);
  /*  
    //========== Test Mode ===========
    if(*(volatile WORD *)BOOT_MODE_STORE_ADDR & SYSTEM_TEST_MODE_BOOT)
    {
        RETAILMSG(1, (TEXT("\n\n\r*** Test Mode ***\n\r")));
        LaunchProgram(L"testing.exe", L"-assembly");
		return 0;
    }

    LaunchProgram(L"CalStylus.exe", L"-start");
    while(IsProcessRunning(L"CalStylus.exe"))
    {
        Sleep(500);
    }

    HANDLE hPreload = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) PreloadThread, NULL, 0, NULL);
    CloseHandle(hPreload);
	*/
    return 0;
}

/*
DWORD PreloadThread(LPVOID pvarg)
{
    TCHAR szProgram[MAX_PATH];

    WaitForAPIReady(SH_SHELL, INFINITE);
    Sleep(500);
    
    CreateDirectory(TEXT("\\Flash Disk\\Preload"), NULL);

    int iPrograms = SysGetPreloadNumbers();

    if(iPrograms == 0)
    {
        HANDLE hPreloadFile;

        hPreloadFile = CreateFile(PRELOAD_CONFIG_FILE, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN, NULL);


        if(hPreloadFile != INVALID_HANDLE_VALUE)
        {

            DWORD dwFileSize = GetFileSize(hPreloadFile, NULL) ;

            BOOL State;

            DWORD NumberOfBytesToRead, NumberOfBytesRead ;

            RETAILMSG(1, (TEXT("\n\rPreloadFile %d \r\n"), dwFileSize));

            if(dwFileSize)
            {
                NumberOfBytesToRead = dwFileSize;

                dwFileSize += 1;

                char *DataBuffer = new char[dwFileSize];

                memset(DataBuffer, '\0', dwFileSize);

                State = ReadFile(hPreloadFile, (PBYTE)DataBuffer, NumberOfBytesToRead, &NumberOfBytesRead , NULL);

                CloseHandle(hPreloadFile);

                if(State)
                {
                    int iSet = 1;

                    while(DataBuffer)
                    {
                        if(strchr(DataBuffer, '[') && strchr(DataBuffer, ']'))
                        {
                            DataBuffer = strchr(DataBuffer, '[') + 1;

                            *strchr(DataBuffer, ']') = '\0';

                            RETAILMSG(1, (TEXT("\n\r%S\r\n"), (char *)DataBuffer));

                            wsprintf(szProgram, L"%S", DataBuffer);

                            if(SysSetPreloadProgram(iSet, szProgram) == FALSE)
                            {
                                break;
                            }

                            DataBuffer += strlen(DataBuffer) + 1;

                            iSet++;
                        }

                        DataBuffer = strchr(DataBuffer, '[');
                    }

                    iPrograms = SysGetPreloadNumbers();
                }

                delete DataBuffer;
            }
        }
    }

    for(int i = 1 ; i <= iPrograms; i++)
    {

        if(SysGetPreloadProgram(i, szProgram))
        {
            RETAILMSG(1, (TEXT("\n\rLaunchProgram-> %d %s\r\n"), i, szProgram));

            if(wcsstr(szProgram, L"SD Card") || wcsstr(szProgram, L"Storage Card"))
            {
                DWORD dwTickCount = GetTickCount();

                while(!SysFindDisk(L"SD Memory Card"))
                {
                    Sleep(1000);

                    if((GetTickCount() - dwTickCount) > 35000)
                    {
                        break;
                    }
                }
            }

            LaunchProgram(szProgram, L"");

        }
        else
        {
            break;
        }
	}

    return 1;
}



DWORD BluetoothNotifyThread(LPVOID pvarg)
{
	MSGQUEUEOPTIONS mqOptions;
	HANDLE hMsgQ = NULL;
	HANDLE hBTNotif;
    //DWORD dwCount = 0;
    
    HANDLE hShellAPI = CreateEvent(NULL, FALSE, FALSE, _T("SYSTEM/ShellAPIReady"));
    WaitForSingleObject(hShellAPI, INFINITE);
    CloseHandle(hShellAPI); 
    
    Sleep(5000);

    if(FindBluetooth())
    {
		*(volatile WORD *)BUILD_IN_DEVICE_FLAG_ADDR |= BT_BUILD_IN_MASK;
    }
    else
    {
        *(volatile WORD *)BUILD_IN_DEVICE_FLAG_ADDR &= ~BT_BUILD_IN_MASK;
        return 0;
    }

	if(Get_BT_Power_form_Register())
	{
		*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR |= BT_POWER_MASK;  
	} 
	
	if(((*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR & BT_POWER_MASK) == 0 ||
		(*(volatile WORD *)BOOT_MODE_STORE_ADDR & ARSR_HW_RESET))     &&
		(*(volatile WORD *)BOOT_MODE_STORE_ADDR & SYSTEM_TEST_MODE_BOOT) == 0)
	{
		SysSetBluetoothPower(FALSE);
    }
    else // if(FindBluetooth())
    {
        *(volatile BYTE *)DEVICE_POWER_FLAG_ADDR |= BT_POWER_MASK;

        AddBluetoothTrayIcon();

        if(BT_LED_Event)
        {
            SetEvent(BT_LED_Event);
        }
    }

    memset(&mqOptions, 0, sizeof(mqOptions));

    mqOptions.dwFlags = 0;
    mqOptions.dwSize = sizeof(mqOptions);
    mqOptions.dwMaxMessages = 10;
    mqOptions.cbMaxMessage = sizeof(BTEVENT);
    mqOptions.bReadAccess = TRUE;

    // Create message queue to receive BT events on
    hMsgQ = CreateMsgQueue(NULL, &mqOptions);

    if(! hMsgQ)
    {

        RETAILMSG(1, (TEXT("\n\rError creating message queue.\r\n")));
        goto exit;
    }

    // Listen for all Bluetooth notifications
    hBTNotif = RequestBluetoothNotifications(
                   BTE_CLASS_CONNECTIONS | BTE_CLASS_DEVICE | BTE_CLASS_PAIRING | BTE_CLASS_STACK | BTE_CLASS_AVDTP,
                   hMsgQ);

    if(! hBTNotif)
    {

        RETAILMSG(1, (TEXT("\n\rError in call to RequestBluetoothNotifications.\r\n")));
        goto exit;
    }

    RETAILMSG(1, (TEXT("\n\rWaiting for Bluetooth notifications...\r\n")));

    HANDLE BT_RDY_Event=CreateEvent(NULL, FALSE, FALSE, BT_RDY_NOTIFY);	

    while(1)
    {
        // Wait on the Message Queue handle (not hBtNotif)
        DWORD dwWait = WaitForSingleObject(hMsgQ, INFINITE);

        if(WAIT_OBJECT_0 == dwWait)
        {
            //
            // We have got a Bluetooth event!
            //
            BTEVENT btEvent;
            DWORD dwFlags = 0;
            DWORD dwBytesRead = 0;

            BOOL fRet = ReadMsgQueue(hMsgQ, &btEvent, sizeof(BTEVENT), &dwBytesRead, 10, &dwFlags);
            if(! fRet)
            {
                RETAILMSG(1, (TEXT("\n\rError - Failed to read message from queue!\r\n")));
                goto exit;
            }
            else
            {
                if((btEvent.dwEventId == BTE_STACK_UP) || (btEvent.dwEventId == BTE_STACK_DOWN))
                {
                    if(btEvent.dwEventId == BTE_STACK_UP)
                    {
                        *(volatile BYTE *)DEVICE_POWER_FLAG_ADDR |= BT_POWER_MASK;
                        AddBluetoothTrayIcon();
                        //Set_BT_Power_2_Register(TRUE);
                    }
                    else
                    {
                        *(volatile BYTE *)DEVICE_POWER_FLAG_ADDR &= ~BT_POWER_MASK;
                        RemoveBluetoothTrayIcon();
                        //Set_BT_Power_2_Register(FALSE);
                    }
                    if(BT_RDY_Event)
		                  SetEvent(BT_RDY_Event); 
		                
                    if(BT_LED_Event)
                    {
                        SetEvent(BT_LED_Event);
                    }
                }
            }
        }
        else
        {
            RETAILMSG(1, (TEXT("\n\rError - Unexpected return value from WaitForSingleObject!\r\n")));
            goto exit;
        }
        //dwCount++;
    }

exit:

    // Stop listening for Bluetooth notifications
    if(hBTNotif && (! StopBluetoothNotifications(hBTNotif)))
    {
        RETAILMSG(1, (TEXT("\n\rWarning! StopBluetoothNotifications returned FALSE!\r\n")));
    }
    if(hMsgQ)
    {
        CloseMsgQueue(hMsgQ);
    }

    return 0;
}

//Thread to get the message of NDIS power
CRITICAL_SECTION    g_GlobalCS;
HANDLE              g_hMsgQueue = NULL;
HANDLE              g_hNdisuio  = NULL;
DWORD NpwNotificationThread(LPVOID pUnused);

DWORD
NpwStartNotification(LPVOID pUnused)
{
#define	NOTIFICATION_REQUIRED           \
    NDISUIO_NOTIFICATION_BIND

    //
    //	CE does not have WMI, so NDISUIO's IOCTL_NDISUIO_REQUEST_NOTIFICATION
    //	handles it for us..
    //	Once created, we never shut down, since we don't have the SCM
    //	equivalent..
    //

    MSGQUEUEOPTIONS					sOptions;
    NDISUIO_REQUEST_NOTIFICATION	sRequestNotification;
    HANDLE							hThread;
    BOOL							bStatus = FALSE;


    //
    //  Wait till PM is ready..  or NDIS has registered its
    //  adapters..
    //
*/
    /*while(!IsAPIReady(SH_SHELL))
    {
        Sleep(50);
    }*//*
	WaitForAPIReady(SH_SHELL, INFINITE);
    Sleep(500);	// 20110927-luk 5000
    
	RETAILMSG(1, (TEXT("\n\n\r*** NpwStartNotification  ***\r\n\n")));
     
    do
    {
        //
        //  First stop is to get a handle to NDISUIO..
        //

        DWORD   dwErr = ERROR_SUCCESS;

        char                    Buffer[1024];
        PNDISUIO_QUERY_BINDING  pQueryBinding;
        DWORD                   dwOutSize, i = 0x00;

        //
        //	Note the Desired Access is zero..
        //	This handle can only do Write/Query.
        //

        g_hNdisuio = CreateFile(
                         (PTCHAR)NDISUIO_DEVICE_NAME,					//	Object name.
                         0x00,											//	Desired access.
                         0x00,											//	Share Mode.
                         NULL,											//	Security Attr
                         OPEN_EXISTING,									//	Creation Disposition.
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,	//	Flag and Attributes..
                         (HANDLE)INVALID_HANDLE_VALUE);

        if(g_hNdisuio == INVALID_HANDLE_VALUE)
        {
            dwErr = GetLastError();
            DEBUGMSG(ZONE_ERROR,
                     (TEXT("NdisPower:: Unable to open [%s]\r\n"),
                      NDISUIO_DEVICE_NAME));

            break;
        }


        //
        //  Check out the interfaces that have been instantiated before
        //  we get launched... (IOCTL_NDISUIO_REQUEST_NOTIFICATION will only
        //  give notification for devices arriving in the future).
        //

        do
        {
            PTCHAR  pAdapterName;

            pQueryBinding = (PNDISUIO_QUERY_BINDING)&Buffer[0];

            memset(pQueryBinding, 0x00, sizeof(NDISUIO_QUERY_BINDING));

            pQueryBinding->BindingIndex = i;

            if(!DeviceIoControl(
                   g_hNdisuio,
                   IOCTL_NDISUIO_QUERY_BINDING,
                   pQueryBinding,
                   sizeof(NDISUIO_QUERY_BINDING),
                   pQueryBinding,
                   1024,
                   &dwOutSize,
                   NULL))
            {
                break;
            }

            //
            //  Make sure it's terminated..
            //

			pAdapterName = (PTCHAR)&Buffer[pQueryBinding->DeviceNameOffset];
            pAdapterName[(pQueryBinding->DeviceNameLength / sizeof(TCHAR)) - 1] = 0x00;

            RETAILMSG(1, (TEXT("\n\rFound adapter [%s]\r\n"), pAdapterName));

            if((*(volatile WORD *)BOOT_MODE_STORE_ADDR&ARSR_HW_RESET))
            {
				// RETAILMSG(1, (TEXT("\n\n\r***NpwStartNotification ARSR_HW_RESET***\r\n")));
				if(*(volatile WORD *)BOOT_MODE_STORE_ADDR &SYSTEM_TEST_MODE_BOOT)
				{	// ?
					RETAILMSG(1, (TEXT("\n\n\r***NpwStartNotification SYSTEM_TEST_MODE_BOOT***\r\n")));
					*(volatile WORD *)BUILD_IN_DEVICE_FLAG_ADDR |= WIFI_BUILD_IN_MASK;
				}
				else
				{
					SET_WIFI_NDISPower(pAdapterName,FALSE);
					*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR&=~WIFI_POWER_MASK;
				}
			}
            else //warm reset
            {
				RETAILMSG(1, (TEXT("\n\n\r***NpwStartNotification ARSR_SW_RESET***\r\n")));

				if(0==(*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR&WIFI_POWER_MASK))
				{
					SET_WIFI_NDISPower(pAdapterName,FALSE);
					*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR&=~WIFI_POWER_MASK;
				}
			}  

            *(volatile WORD *)BUILD_IN_DEVICE_FLAG_ADDR |= WIFI_BUILD_IN_MASK;
            
            //20111026-luk
			HANDLE hWIFIReadyHandle = CreateEvent(NULL, FALSE, FALSE, WIFI_RDY_NOTIFY);
			SetEvent(WIFI_RDY_NOTIFY);  		   
			CloseHandle(WIFI_RDY_NOTIFY);            
            
            break;
        }
        while(TRUE);

        //
        //	Then create the msg queue..
        //

        sOptions.dwSize					= sizeof(MSGQUEUEOPTIONS);
        sOptions.dwFlags				= 0;
        sOptions.dwMaxMessages	= 4;
        sOptions.cbMaxMessage		= sizeof(NDISUIO_DEVICE_NOTIFICATION);
        sOptions.bReadAccess		= TRUE;

        g_hMsgQueue = CreateMsgQueue(NULL, &sOptions);

        if(g_hMsgQueue == NULL)
        {
            DEBUGMSG(ZONE_ERROR,
                     (TEXT("NdisPower:: Error CreateMsgQueue()..\r\n")));
            break;
        }


        //
        //	Queue created successfully, tell NDISUIO about it..
        //

        sRequestNotification.hMsgQueue				   = g_hMsgQueue;
        sRequestNotification.dwNotificationTypes = 0xffffffff;

        if(!DeviceIoControl(
               g_hNdisuio,
               IOCTL_NDISUIO_REQUEST_NOTIFICATION,
               &sRequestNotification,
               sizeof(NDISUIO_REQUEST_NOTIFICATION),
               NULL,
               0x00,
               NULL,
               NULL))
        {
            DEBUGMSG(ZONE_ERROR,
                     (TEXT("NdisPower:: Err IOCTL_NDISUIO_REQUEST_NOTIFICATION\r\n")));
            break;
        }

        //
        //	NDISUIO takes it well, now party on it..
        //

        hThread = CreateThread(
                      NULL,
                      0,
                      NpwNotificationThread,
                      NULL,
                      0,
                      NULL);

        CloseHandle(hThread);

        //
        //	Everything is cool..
        //

        DEBUGMSG(ZONE_INIT,
                 (TEXT("NdisPower:: Successfully register for notification!\r\n")));

        bStatus = TRUE;

    }
    while(FALSE);


    if(!bStatus)
    {
        if(g_hMsgQueue)
        {
            CloseMsgQueue(g_hMsgQueue);
        }

        if(g_hNdisuio)
        {
            CloseHandle(g_hNdisuio);
        }
    }

    DeleteCriticalSection(&g_GlobalCS);
    
    if(Get_WLAN_Power_form_Register())
	{
		*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR |= WIFI_POWER_MASK;  
	}
				
	if((*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR&WIFI_POWER_MASK))
	{
		SysSetWLANPower(TRUE);
	}
	
    return 0;

}

DWORD NpwNotificationThread(LPVOID pUnused)
{
    NDISUIO_DEVICE_NOTIFICATION		sDeviceNotification;
    DWORD							dwBytesReturned;
    DWORD							dwFlags;
    TCHAR pszAdapter[MAX_PATH]=L"";

    //
    //  Wait till PM is ready..  or NDIS has registered its
    //  adapters..
    //


    while(WaitForSingleObject(g_hMsgQueue, INFINITE) == WAIT_OBJECT_0)
    {
        while(ReadMsgQueue(
                  g_hMsgQueue,
                  &sDeviceNotification,
                  sizeof(NDISUIO_DEVICE_NOTIFICATION),
                  &dwBytesReturned,
                  1,
                  &dwFlags))
        {
            //
            //	Okay, we have notification..
            //

            RETAILMSG(1,
                      (TEXT("\n\rNdisPower:: Notification:: Event [%s] \t Adapter [%s]\r\n"),
                       (sDeviceNotification.dwNotificationType & NDISUIO_NOTIFICATION_RESET_START)
                       ?	TEXT("RESET_START")			:
                       (sDeviceNotification.dwNotificationType & NDISUIO_NOTIFICATION_RESET_END)
                       ? 	TEXT("RESET_END")			:
                       (sDeviceNotification.dwNotificationType & NDISUIO_NOTIFICATION_MEDIA_CONNECT)
                       ?	TEXT("MEDIA_CONNECT")		:
                       (sDeviceNotification.dwNotificationType & NDISUIO_NOTIFICATION_MEDIA_DISCONNECT)
                       ?	TEXT("MEDIA_DISCONNECT")	:
                       (sDeviceNotification.dwNotificationType & NDISUIO_NOTIFICATION_BIND)
                       ?	TEXT("BIND")					:
                       (sDeviceNotification.dwNotificationType & NDISUIO_NOTIFICATION_UNBIND)
                       ?	TEXT("UNBIND")                  :
    			             (sDeviceNotification.dwNotificationType & NDISUIO_NOTIFICATION_DEVICE_POWER_DOWN)
    				           ?	TEXT("POWER_DOWN")                  :  
    			             (sDeviceNotification.dwNotificationType & NDISUIO_NOTIFICATION_DEVICE_POWER_UP)
    				           ?	TEXT("POWER_UP")                  :                         
                       TEXT("Unknown!"),
                       sDeviceNotification.ptcDeviceName));


            //
            //	This is what we do to the notification:
            //

            if(sDeviceNotification.dwNotificationType == NDISUIO_NOTIFICATION_BIND)
            {

                RETAILMSG(1, (TEXT("\n\r*** BIND notification for adapter [%s]***\r\n"), sDeviceNotification.ptcDeviceName));
                
				//RETAILMSG(1, (TEXT("\n\r+WLAN_FIRST_START_SOTRE_ADDR(%d)BOOT_MODE_STORE_ADDR(%x)\r\n"),*(volatile BYTE *)WLAN_FIRST_START_SOTRE_ADDR,*(volatile WORD *)BOOT_MODE_STORE_ADDR&ARSR_HW_RESET)); 
              *//*  
                if(*(volatile BYTE *)WLAN_FIRST_START_SOTRE_ADDR==TRUE)
                {
     
                  *(volatile WORD *)BUILD_IN_DEVICE_FLAG_ADDR |= WIFI_BUILD_IN_MASK;
                  
                  Sleep(5000);
                  
                  if(*(volatile WORD *)BOOT_MODE_STORE_ADDR&ARSR_HW_RESET)
                  {               
                    wsprintf(pszAdapter,L"%S",SUMMIT_WIFI);
    
                    SET_WIFI_NDISPower(pszAdapter,FALSE);
                 
                    *(volatile BYTE *)DEVICE_POWER_FLAG_ADDR &= ~WIFI_POWER_MASK;
                    
                  }
                  else if(*(volatile WORD *)BOOT_MODE_STORE_ADDR&ARSR_SW_RESET)
                  {
                     if((*(volatile BYTE *)DEVICE_POWER_FLAG_ADDR & WIFI_POWER_MASK)==0)
                      {
                          wsprintf(pszAdapter,L"%S",SUMMIT_WIFI);
    
                          SET_WIFI_NDISPower(pszAdapter,FALSE);
                       
                         *(volatile BYTE *)DEVICE_POWER_FLAG_ADDR &= ~WIFI_POWER_MASK;
                      }
                      else
                      {
                         *(volatile BYTE *)DEVICE_POWER_FLAG_ADDR |= WIFI_POWER_MASK;                     
                        
                      }
                      
                  }

                 *(volatile BYTE *)WLAN_FIRST_START_SOTRE_ADDR = FALSE; 
                 
                }
                else
                { 

                    *(volatile BYTE *)DEVICE_POWER_FLAG_ADDR |= WIFI_POWER_MASK;

                }
                *//*
                
                *(volatile WORD *)DEVICE_POWER_FLAG_ADDR |= WIFI_POWER_MASK;

            }

            if(sDeviceNotification.dwNotificationType == NDISUIO_NOTIFICATION_UNBIND)
            {

                RETAILMSG(1, (TEXT("\n\r*** UNBIND notification for adapter [%s] ***\r\n"), sDeviceNotification.ptcDeviceName));

                *(volatile BYTE *)DEVICE_POWER_FLAG_ADDR &= ~WIFI_POWER_MASK;

           //    PostMessage(FindWindow(TEXT("Summit Client Utility"), NULL), WM_DESTROY, 0, 0);

            }

            //SetInterruptEvent(SYSINTR_POWER_SOURCE);




        }

    }

    return 0x00;

}
*/