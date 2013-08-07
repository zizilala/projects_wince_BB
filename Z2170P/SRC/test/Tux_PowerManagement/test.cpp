//
//  All rights reserved ADENEO EMBEDDED 2010
//
////////////////////////////////////////////////////////////////////////////////
//
//  Tux_PowerManagement TUX DLL
//
//  Module: test.cpp
//          Contains the test functions.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#pragma warning(push)
#pragma warning(disable : 6258 6385)

#include "main.h"
#include "globals.h"
#include <pm.h>
#include <notify.h>

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

#define ARSIZE 100
#define SSTATE_NAME_ON			L"on"
#define SSTATE_NAME_USERIDLE	L"useridle"
#define SSTATE_NAME_SYSTEMIDLE	L"systemidle"
#define SSTATE_NAME_SUSPEND		L"suspend"


#define SREGISTRY_AC_USERIDLE		L"ACUserIdle"
#define SREGISTRY_AC_SYSTEMIDLE		L"ACSystemIdle"
#define SREGISTRY_AC_SUSPEND		L"ACSuspend"

#define SREGISTRY_BATTERY_USERIDLE		L"BattUserIdle"
#define SREGISTRY_BATTERY_SYSTEMIDLE	L"BattSystemIdle"
#define SREGISTRY_BATTERY_SUSPEND		L"BattSuspend"

#define STATE_SEEN				1
#define STATE_NOT_SEEN			2
#define NOT_SET					3

#define TEST_COMPLETE			0x0a
#define TEST_ERROR				0x0d
#define TEST_RUNNING			0x0e
#define TEST_PASS				0x0f
#define TEST_FAIL				0x10

#define FILE_PATH "test.bin"
#define RESUME_EVENT L"ResumeEvent"

struct test_args_t {
	int n_loops;
	int sleep_time;
	int n_bytes;
	BOOL running;
};

HANDLE  g_hPMThread1, g_hPMThread2;
DWORD	dwStateId_on			= 0;
DWORD   dwStateId_useridle		= 1;
DWORD   dwStateId_systemidle	= 2;
DWORD   dwStateId_suspend		= 3;

BOOL    bOnState				= FALSE;
BOOL    bUserIdleState			= FALSE;
BOOL	bSystemIdleState		= FALSE;
BOOL	bSuspendState			= FALSE;

DWORD   g_bTestStatus			= TEST_RUNNING;
DWORD   g_bTestResult			= TEST_PASS;
DWORD	g_dwMaxTimeout			= 0;


//////////////////////////////////////////////////////////////
//    PowerStateTest global variables                     ////
//////////////////////////////////////////////////////////////
DWORD g_dwOnStateSeen			= NOT_SET;
DWORD g_dwUserIdleStateSeen		= NOT_SET;
DWORD g_dwSystemIdleStateSeen	= NOT_SET;
DWORD g_dwSuspendStateSeen		= NOT_SET;

DWORD g_dwSaveACUserIdle		= 0;
DWORD g_dwSaveACSystemIdle		= 0;
DWORD g_dwSaveACSuspend			= 0;
DWORD g_dwSaveBattUserIdle		= 0;
DWORD g_dwSaveBattSystemIdle	= 0;
DWORD g_dwSaveBattSuspend		= 0;

BOOL  g_bTestBegin				= FALSE;

typedef struct StateInfo
{
	TCHAR   szStateName[MAX_PATH];
	TCHAR   szTimeoutName[MAX_PATH];
	DWORD   dwStateTimeout;
	BOOL    bEnabled;
}STATE_INFO,PSTATE_INFO;

STATE_INFO g_StateInfoTable[ARSIZE];
FILETIME  g_CurrentSystemTime;

DWORD g_TestStartTime = 0;
DWORD g_numofstates = 1;

DWORD g_PreviousState = 0;
DWORD g_CurrentState = 0;

static BOOL InitializeStateMachine(void);
static BOOL InitializePMNotifications(void);
static BOOL StartPMNotifications(void);
static DWORD WINAPI PMNotificationThread(void);
static DWORD WINAPI PMNotificationThread2(void);
static DWORD CheckStateMachine(POWER_BROADCAST *ppb, FILETIME *systime);
static BOOL IsLastState(DWORD dwStateId);
static BOOL UpdateTestTicker(DWORD dwStateId, DWORD tcount);
static BOOL DisableStateTimeouts(void);
static BOOL RestoreStateTimeouts(void);

////////////////////////////////////////////////////////////////////////////////
// TestProc
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI TestPowerStates(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	DWORD dwExitCode;
	DWORD dwRet;

    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

	// disable all of the system state timeouts so we don't go into any of
	// the states accidentally.
	g_pKato->Log(LOG_COMMENT,(TEXT("Saving and Disabling System State Timeouts\r\n")));
	if(!DisableStateTimeouts())
	{
		dwRet = TPR_ABORT;
		goto end;
	}

	// start watching for the PM notifications
	g_pKato->Log(LOG_COMMENT,(TEXT("Registering for PM Notifications\r\n")));
	if(!StartPMNotifications())
	{
		dwRet = TPR_ABORT;
		goto end;
	}

	for(DWORD i = 0; i < g_StressCount; i++)
	{
		// Give time for the PM notifications to start
		g_pKato->Log(LOG_COMMENT,(TEXT("Starting Test:\r\n")));
		Sleep(5000);

		// test that we can go into the ON state correctly
		g_pKato->Log(LOG_COMMENT,(TEXT("Testing ON State:\r\n")));
		g_bTestBegin = TRUE;
		SetSystemPowerState(SSTATE_NAME_ON,NULL,POWER_FORCE);
		Sleep(5000);
		if(g_dwOnStateSeen != STATE_SEEN)
		{
			g_pKato->Log(LOG_COMMENT,(TEXT("ON state not seen, test failed!\r\n")));
			dwRet = TPR_FAIL;
			goto end;
		}
		else
		{
			g_pKato->Log(LOG_COMMENT,(TEXT("ON state seen!\r\n")));
		}

		// test that we can go into the USERIDLE state correctly
		g_pKato->Log(LOG_COMMENT,(TEXT("Testing USERIDLE State:\r\n")));
		SetSystemPowerState(SSTATE_NAME_USERIDLE,NULL,POWER_FORCE);
		Sleep(5000);
		if(g_dwUserIdleStateSeen != STATE_SEEN)
		{
			g_pKato->Log(LOG_COMMENT,(TEXT("USERIDLE state not seen, test failed!\r\n")));
			dwRet = TPR_FAIL;
			goto end;
		}
		else
		{
			g_pKato->Log(LOG_COMMENT,(TEXT("USERIDLE state seen!\r\n")));
		}

		// 5 seconds after useridle state is on, system idle will trigger
		// check that this has happened
		g_pKato->Log(LOG_COMMENT,(TEXT("Testing SYSTEMIDLE State:\r\n")));
		Sleep(5000);
		if(g_dwSystemIdleStateSeen != STATE_SEEN)
		{
			g_pKato->Log(LOG_COMMENT,(TEXT("SYSTEMIDLE state not seen, test failed!\r\n")));
			dwRet = TPR_FAIL;
			goto end;
		}
		else
		{
			g_pKato->Log(LOG_COMMENT,(TEXT("SYSTEMIDLE state seen!\r\n")));
		}

		// test the suspend state
		g_pKato->Log(LOG_COMMENT,(TEXT("Testing SUSPEND State, please wake system to continue test:\r\n")));
		SetSystemPowerState(SSTATE_NAME_SUSPEND,NULL,POWER_FORCE);
		Sleep(5000);
		if(g_dwUserIdleStateSeen != STATE_SEEN)
		{
			g_pKato->Log(LOG_COMMENT,(TEXT("SUSPEND state not seen, test failed!\r\n")));
			dwRet = TPR_FAIL;
			goto end;
		}
		else
		{
			g_pKato->Log(LOG_COMMENT,(TEXT("SUSPEND state seen!\r\n")));
		}

	}

	dwRet =  TPR_PASS;

end:
	// restore the system state timeout values we disabled before
	g_pKato->Log(LOG_COMMENT,(TEXT("Restore System State Timeouts\r\n")));
	RestoreStateTimeouts();

	Sleep(3000);

	// deregister for PM notifications
	if(g_hPMThread2 != NULL)
	{
		GetExitCodeThread(g_hPMThread2,&dwExitCode);
		TerminateThread(g_hPMThread2,dwExitCode);
	}

	g_bTestBegin = FALSE;

	return dwRet;
}

////////////////////////////////////////////////////////////////////////////////
// TestProc
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI TestSystemStateMachine(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	DWORD dwExitCode;
	DWORD dwRet;
    // The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

	if(!InitializeStateMachine())
	{
		dwRet = TPR_ABORT;
		goto cleanup;
	}

	if(!InitializePMNotifications())
	{
		dwRet = TPR_ABORT;
		goto cleanup;
	}


	for(DWORD i = 0; i < g_StressCount; i++)
	{
		// reset these values each time we run the test
		g_bTestStatus = TEST_RUNNING;
		//g_PreviousState = dwStateId_on;

		Sleep(3000);

		// set the test start time
		GetSystemTimeAsFileTime(&g_CurrentSystemTime);
		g_TestStartTime = g_CurrentSystemTime.dwLowDateTime;

		// force state to on so that we can see the timeouts happen
		//SetSystemPowerState(NULL,POWER_STATE_USERIDLE,POWER_FORCE);

		keybd_event((BYTE)0x87,0,(KEYEVENTF_KEYUP|KEYEVENTF_SILENT),0);

		//wait for test to run
		while(g_bTestStatus == TEST_RUNNING){Sleep(10);}

		// check results of test
		if(g_bTestStatus == TEST_PASS)
		{
			g_pKato->Log(LOG_COMMENT,(TEXT("Test succeeded\r\n")));
		}
		else if(g_bTestStatus == TEST_FAIL)
		{
			g_pKato->Log(LOG_COMMENT,(TEXT("Test failed!\r\n")));
			g_bTestResult = TEST_FAIL;
			//return TPR_FAIL;
		}
		else if(g_bTestStatus == TEST_ERROR)
		{
			g_pKato->Log(LOG_COMMENT,(TEXT("Test error\r\n")));
			g_bTestResult = TEST_ERROR;
			//return TPR_ABORT;
		}
		else
		{
			g_pKato->Log(LOG_COMMENT,(TEXT("Test Abort\r\n")));
			g_bTestResult = TEST_ERROR;
			//return TPR_ABORT;
		}
	}

	g_pKato->Log(LOG_COMMENT,(TEXT("Test End\r\n")));
	// if none of the tests failed or had an error, g_bTestResult should still contain TEST_PASS
	switch(g_bTestResult)
	{
	case TEST_PASS:
		dwRet = TPR_PASS;
		//return TPR_PASS;
		break;
	case TEST_FAIL:
		dwRet = TPR_FAIL;
		//return TEST_FAIL;
		break;
	case TEST_ERROR:
		dwRet = TPR_ABORT;
		//return TPR_ABORT;
		break;
	default:
		dwRet = TPR_ABORT;
		//return TPR_ABORT;
		break;
	}

cleanup:
	GetExitCodeThread(g_hPMThread1,&dwExitCode);
	TerminateThread(g_hPMThread1,dwExitCode);
	return dwRet;
}

DWORD file_thread(void *args)
{
	struct test_args_t *test_args = (struct test_args_t *)args;
	FILE * pFile;
	BYTE buffer[255];
	BYTE buffer_read[255];
	int size;
	int i;

	size = MIN(test_args->n_bytes, 255);

	for (i =0; i<size; i++)
	{
		buffer[i] = i;
	}

	while (test_args->running)
	{
		pFile=fopen(FILE_PATH, "w");
		
		if (pFile == NULL)
		{
			g_bTestStatus=TEST_ERROR;
			g_pKato->Log(LOG_COMMENT,(TEXT("Cannot open file\r\n")));
			break;
		}

		if (fwrite(buffer,1,size,pFile) != size)
		{
			g_bTestStatus=TEST_ERROR;
			g_pKato->Log(LOG_COMMENT,(TEXT("Cannot write all expected bytes to file\r\n")));
			break;
		}

		fclose(pFile);

		pFile=fopen(FILE_PATH, "r");
		
		if (pFile == NULL)
		{
			g_bTestStatus=TEST_ERROR;
			g_pKato->Log(LOG_COMMENT,(TEXT("Cannot open file\r\n")));
			break;
		}

		if (fread(buffer_read,1,size,pFile) != size)
		{
			g_bTestStatus=TEST_ERROR;
			g_pKato->Log(LOG_COMMENT,(TEXT("Cannot read all expected bytes from file\r\n")));
			break;
		}

		for (i = 0; i<size; i++)
		{
			if (buffer_read[i] != i)
			{
				g_bTestStatus=TEST_ERROR;
				g_pKato->Log(LOG_COMMENT,TEXT("Byte read %u is different from byte expected %u\r\n"),buffer_read[i],i);
				break;
			}
		}

		fclose(pFile);

		if (g_bTestStatus == TEST_ERROR)
		{
			break;
		}
	}

	return TEST_PASS;
}

////////////////////////////////////////////////////////////////////////////////
// TestProc
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI TestSuspendResume(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	DWORD dwRet;
	struct test_args_t test_args;
	HANDLE thread;
	HANDLE hEvent;
	int i;
	SYSTEMTIME curSysTime;
	FILETIME fileTime;

    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

	// Fill arguments structure
	test_args.n_loops = g_NbLoop;
	test_args.sleep_time = g_TimeToSleep;
	test_args.n_bytes = g_NbBytes;
	test_args.running = TRUE;

	// Create event
	hEvent = CreateEvent(NULL, FALSE, FALSE, RESUME_EVENT);

	g_bTestStatus = TEST_RUNNING;

	// Create thread
	thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)file_thread, (void *)&test_args, 0, NULL);

	for (i = 0; i < test_args.n_loops; i++)
	{
		if (g_bTestStatus != TEST_RUNNING)
		{
			dwRet = TPR_FAIL;
			break;
		}
		// Set time of user notification
		GetLocalTime(&curSysTime);
		SystemTimeToFileTime(&curSysTime, &fileTime);
		*((UINT64 *)&fileTime) += test_args.sleep_time * 10000000;
		FileTimeToSystemTime(&fileTime, &curSysTime);

		// Launch notification
		CeRunAppAtTime(L"\\\\.\\Notifications\\NamedEvents\\"RESUME_EVENT, &curSysTime);

		// Suspend the system
		RETAILMSG(1, (L"Suspending...\r\n"));
		SetSystemPowerState(SSTATE_NAME_SUSPEND,NULL,POWER_FORCE);
		RETAILMSG(1, (L"Resumed !\r\n"));
		g_pKato->Log(LOG_COMMENT,TEXT("Platform resumed %u time(s)\r\n"),i+1);
		Sleep(10000);
	}

	// Stop thread
	test_args.running = FALSE;
	WaitForSingleObject(thread,INFINITE);

	// Close handles
	CloseHandle(thread);
	CloseHandle(hEvent);

	g_pKato->Log(LOG_COMMENT,(TEXT("Test End\r\n")));
	dwRet =  TPR_PASS;

	return dwRet;
}

static BOOL StartPMNotifications(void)
{
	g_hPMThread2 = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)PMNotificationThread2,NULL,0,NULL);
	if(g_hPMThread2 == NULL)
	{
		RETAILMSG(1,(TEXT("Failed to create PMNotificationThread\r\n")));
		return TPR_ABORT;
	}

	return TRUE;
}


static BOOL InitializePMNotifications(void)
{
	g_hPMThread1 = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)PMNotificationThread,NULL,0,NULL);
	if(g_hPMThread1 == NULL)
	{
		//RETAILMSG(1,(TEXT("KERNUTILS_InitPMMessageQueue: Failed to create PMNotificationThread\r\n")));
		return FALSE;
	}

	return TRUE;
}


static DWORD WINAPI PMNotificationThread2(void)
{
	DWORD cbPowerMsgSize = sizeof(POWER_BROADCAST) + (MAX_PATH * sizeof TCHAR);
	DWORD dwWaitStatus = WAIT_OBJECT_0;

	// Initialize a MSGQUEUEOPTIONS structure
	MSGQUEUEOPTIONS mqo;
	mqo.dwSize = sizeof(MSGQUEUEOPTIONS);
	mqo.dwFlags = MSGQUEUE_NOPRECOMMIT;
	mqo.dwMaxMessages = 4;
	mqo.cbMaxMessage = cbPowerMsgSize;
	mqo.bReadAccess = TRUE;

	// Create a message queue to receive power notifications
	HANDLE hPowerMsgQ = CreateMsgQueue(NULL, &mqo);
	if (NULL == hPowerMsgQ)
	{
		g_pKato->Log(LOG_ABORT,(L"CreateMsgQueue failed \r\n"));
		//RETAILMSG(1, (L"CreateMsgQueue failed: %x\n", GetLastError()));
		g_bTestStatus = TEST_ERROR;
	}
	// Request power notifications.
	HANDLE hPowerNotifications = RequestPowerNotifications(hPowerMsgQ, PBT_TRANSITION);

	while(WaitForSingleObject(hPowerMsgQ, INFINITE) == WAIT_OBJECT_0)
	{
		DWORD cbRead;
		DWORD dwFlags;

		//g_pKato->Log(LOG_DETAIL,(TEXT("Recieved a new Power Notification!\r\n")));

		POWER_BROADCAST *ppb = (POWER_BROADCAST *)new BYTE[cbPowerMsgSize];
		if (!ppb)
		{
			g_pKato->Log(LOG_ABORT,(TEXT("ERROR: failed to allocate memory\r\n")));
			g_bTestStatus = TEST_ERROR;
		}

		// Loop through in case there is more than 1 msg
		while(ReadMsgQueue(hPowerMsgQ, ppb, cbPowerMsgSize, &cbRead, 0, &dwFlags))
		{
			if (ppb->Message == PBT_TRANSITION)
			{
				//RETAILMSG(1, (L"SystemPowerState = %s\r\n", ppb->SystemPowerState));

				if(wcsncmp(ppb->SystemPowerState,SSTATE_NAME_ON,sizeof(ppb->SystemPowerState))==0 && g_bTestBegin)
				{
					g_dwOnStateSeen = STATE_SEEN;
				}
				else if(wcsncmp(ppb->SystemPowerState,SSTATE_NAME_USERIDLE,sizeof(ppb->SystemPowerState))==0 && g_bTestBegin)
				{
					g_dwUserIdleStateSeen = STATE_SEEN;
				}
				else if(wcsncmp(ppb->SystemPowerState,SSTATE_NAME_SYSTEMIDLE,sizeof(ppb->SystemPowerState))==0 && g_bTestBegin)
				{
					g_dwSystemIdleStateSeen = STATE_SEEN;
				}
				else if(wcsncmp(ppb->SystemPowerState,SSTATE_NAME_SUSPEND,sizeof(ppb->SystemPowerState))==0 && g_bTestBegin)
				{
					g_dwSuspendStateSeen = STATE_SEEN;
				}

			}
		}

		delete [] ppb;
	}

	return 0;
}

static DWORD WINAPI PMNotificationThread(void)
{
	DWORD cbPowerMsgSize = sizeof(POWER_BROADCAST) + (MAX_PATH * sizeof TCHAR);
	DWORD dwWaitStatus = WAIT_OBJECT_0;

	// Initialize a MSGQUEUEOPTIONS structure
	MSGQUEUEOPTIONS mqo;
	mqo.dwSize = sizeof(MSGQUEUEOPTIONS);
	mqo.dwFlags = MSGQUEUE_NOPRECOMMIT;
	mqo.dwMaxMessages = 4;
	mqo.cbMaxMessage = cbPowerMsgSize;
	mqo.bReadAccess = TRUE;

	//RETAILMSG(1,(TEXT("starting  pm notification thread!!!!!!!!!!!!!!!\r\n")));

	// Create a message queue to receive power notifications
	HANDLE hPowerMsgQ = CreateMsgQueue(NULL, &mqo);
	if (NULL == hPowerMsgQ)
	{
		g_pKato->Log(LOG_ABORT,(L"CreateMsgQueue failed \r\n"));
		//RETAILMSG(1, (L"CreateMsgQueue failed: %x\n", GetLastError()));
		g_bTestStatus = TEST_ERROR;
	}
	// Request power notifications.
	HANDLE hPowerNotifications = RequestPowerNotifications(hPowerMsgQ, PBT_TRANSITION);

	// Wait for a power notification or for the app to exit.
	while(g_bTestStatus == TEST_RUNNING)
	{
		while(dwWaitStatus == WAIT_TIMEOUT || dwWaitStatus == WAIT_OBJECT_0)
		{
			DWORD cbRead;
			DWORD dwFlags;

			dwWaitStatus = WaitForSingleObject(hPowerMsgQ, ((g_dwMaxTimeout*1000)+10000));

		//while(WaitForSingleObject(hPowerMsgQ, ((g_dwMaxTimeout*1000)+10000)) == WAIT_OBJECT_0)
		//{

			if(dwWaitStatus == WAIT_TIMEOUT)
			{
				// if timeout occurs, it could be because we are in on state already,
				// so no transition will occur and we will timeout
				// if this is the case the test still passes, we just dont have any
				// power states to transition to
				if(IsLastState(dwStateId_on))
				{
					g_pKato->Log(LOG_DETAIL,(TEXT("Test Finished Successfully on \"On\" State\r\n")));
					g_bTestStatus = TEST_PASS;
				}
				else
				{
					g_pKato->Log(LOG_DETAIL,(TEXT("State transition timeout reached!\r\n")));
					g_bTestStatus = TEST_FAIL;
				}
			}
			else
			{
				g_pKato->Log(LOG_DETAIL,(TEXT("Recieved a new Power Notification!\r\n")));

				POWER_BROADCAST *ppb = (POWER_BROADCAST *)new BYTE[cbPowerMsgSize];
				if (!ppb)
				{
					g_pKato->Log(LOG_ABORT,(TEXT("ERROR: failed to allocate memory\r\n")));
					g_bTestStatus = TEST_ERROR;
				}

				// Loop through in case there is more than 1 msg
				while(ReadMsgQueue(hPowerMsgQ, ppb, cbPowerMsgSize, &cbRead, 0, &dwFlags))
				{
					if (ppb->Message == PBT_TRANSITION)
					{
						//RETAILMSG(1, (L"Transition detected !\r\n"));
						//RETAILMSG(1, (L"SystemPowerState = %s\r\n", ppb->SystemPowerState));
						
						// got a new power state, check if it happened in the correct amount of time
						GetSystemTimeAsFileTime(&g_CurrentSystemTime);

						// only check the state transition if the test hasn't already finished
						if(g_bTestStatus == TEST_RUNNING)
						{
							switch(CheckStateMachine(ppb,&g_CurrentSystemTime))
							{
								case TEST_COMPLETE:
									g_bTestStatus = TEST_PASS;
									break;
								case TEST_FAIL:
									g_bTestStatus = TEST_FAIL;
									break;
								case TEST_ERROR:
									g_bTestStatus = TEST_ERROR;
									break;
								case TEST_RUNNING:
									break;
							}
						}
					}
				}

				delete [] ppb;
			}
		}
	}
	return 0;
}

static BOOL RestoreStateTimeouts(void)
{
	HKEY    hRegKey             = NULL;
	DWORD   dwNumSubKeys        = 0;
	LPWSTR  SubKeyName           = NULL;
	LPCWSTR regSubkey		    = TEXT("System\\CurrentControlSet\\Control\\Power\\Timeouts");

    DWORD dwKeyIndex = 0;       
    TCHAR szNewKey[MAX_PATH]; 
	DWORD valuedatasize = sizeof(DWORD);
    DWORD dwNewKeySize;       // size of name of DeviceX subkey
    dwNewKeySize = (sizeof(szNewKey) / sizeof(TCHAR));

	// find all of the states that are defined in the registry
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regSubkey, 0, 0, &hRegKey) == ERROR_SUCCESS)
	{
		RegSetValueEx(hRegKey,SREGISTRY_AC_USERIDLE,0,REG_DWORD,(BYTE *)&g_dwSaveACUserIdle,sizeof(DWORD));
		RegSetValueEx(hRegKey,SREGISTRY_AC_SYSTEMIDLE,0,REG_DWORD,(BYTE *)&g_dwSaveACSystemIdle,sizeof(DWORD));
		RegSetValueEx(hRegKey,SREGISTRY_AC_SUSPEND,0,REG_DWORD,(BYTE *)&g_dwSaveACSuspend,sizeof(DWORD));
		RegSetValueEx(hRegKey,SREGISTRY_BATTERY_USERIDLE,0,REG_DWORD,(BYTE *)&g_dwSaveBattUserIdle,sizeof(DWORD));
		RegSetValueEx(hRegKey,SREGISTRY_BATTERY_SYSTEMIDLE,0,REG_DWORD,(BYTE *)&g_dwSaveBattSystemIdle,sizeof(DWORD));
		RegSetValueEx(hRegKey,SREGISTRY_BATTERY_SUSPEND,0,REG_DWORD,(BYTE *)&g_dwSaveBattSuspend,sizeof(DWORD));
		
		// Tell power manager to reload the registry timeouts
		HANDLE hevReloadActivityTimeouts = 
			OpenEvent(EVENT_ALL_ACCESS, FALSE, _T("PowerManager/ReloadActivityTimeouts"));
		if (hevReloadActivityTimeouts) {
			SetEvent(hevReloadActivityTimeouts);
			CloseHandle(hevReloadActivityTimeouts);
		}
		else {
			RETAILMSG(1, (TEXT("Error: Could not open PM reload event\r\n")));
		}

		// done with the key, close it
		RegCloseKey(hRegKey);
	}
	else
	{
		RETAILMSG(1, (TEXT("Tux_PowerManagement: Error opening registry key\r\n")));
		return FALSE;
	}

	return TRUE;
}


static BOOL DisableStateTimeouts(void)
{
	HKEY    hRegKey             = NULL;
	DWORD   dwNumSubKeys        = 0;
	LPWSTR  SubKeyName           = NULL;
	LPCWSTR regSubkey		    = TEXT("System\\CurrentControlSet\\Control\\Power\\Timeouts");

	SYSTEM_POWER_STATUS_EX2		powerStatus;
    DWORD dwKeyIndex = 0;       
    TCHAR szNewKey[MAX_PATH]; 
	DWORD valuedatasize = sizeof(DWORD);
    DWORD dwNewKeySize;       // size of name of DeviceX subkey
    dwNewKeySize = (sizeof(szNewKey) / sizeof(TCHAR));

	// find all of the states that are defined in the registry
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regSubkey, 0, 0, &hRegKey) == ERROR_SUCCESS)
	{
		DWORD dwKeyValue = 0;
		DWORD dwBuffSize = sizeof(DWORD);

		RegQueryValueEx(hRegKey,SREGISTRY_AC_USERIDLE,NULL,NULL,(LPBYTE)&g_dwSaveACUserIdle,&dwBuffSize);
		RegQueryValueEx(hRegKey,SREGISTRY_AC_SYSTEMIDLE,NULL,NULL,(LPBYTE)&g_dwSaveACSystemIdle,&dwBuffSize);
		RegQueryValueEx(hRegKey,SREGISTRY_AC_SUSPEND,NULL,NULL,(LPBYTE)&g_dwSaveACSuspend,&dwBuffSize);
		RegQueryValueEx(hRegKey,SREGISTRY_BATTERY_USERIDLE,NULL,NULL,(LPBYTE)&g_dwSaveBattUserIdle,&dwBuffSize);
		RegQueryValueEx(hRegKey,SREGISTRY_BATTERY_SYSTEMIDLE,NULL,NULL,(LPBYTE)&g_dwSaveBattSystemIdle,&dwBuffSize);
		RegQueryValueEx(hRegKey,SREGISTRY_BATTERY_SUSPEND,NULL,NULL,(LPBYTE)&g_dwSaveBattSuspend,&dwBuffSize);


		// By default, settings are set for a platform running on AC power
		RegSetValueEx(hRegKey,SREGISTRY_AC_USERIDLE,0,REG_DWORD,(BYTE *)&dwKeyValue,sizeof(DWORD));
		dwKeyValue = 5;
		RegSetValueEx(hRegKey,SREGISTRY_AC_SYSTEMIDLE,0,REG_DWORD,(BYTE *)&dwKeyValue,sizeof(DWORD));
		dwKeyValue = 0;
		RegSetValueEx(hRegKey,SREGISTRY_AC_SUSPEND,0,REG_DWORD,(BYTE *)&dwKeyValue,sizeof(DWORD));
		RegSetValueEx(hRegKey,SREGISTRY_BATTERY_USERIDLE,0,REG_DWORD,(BYTE *)&dwKeyValue,sizeof(DWORD));
		RegSetValueEx(hRegKey,SREGISTRY_BATTERY_SYSTEMIDLE,0,REG_DWORD,(BYTE *)&dwKeyValue,sizeof(DWORD));
		RegSetValueEx(hRegKey,SREGISTRY_BATTERY_SUSPEND,0,REG_DWORD,(BYTE *)&dwKeyValue,sizeof(DWORD));

		
		// If a real battery driver is present
		if (GetSystemPowerStatusEx2(&powerStatus,sizeof(powerStatus),TRUE))
		{
			// If the platform runs currently only on battery
			if ( (powerStatus.ACLineStatus == AC_LINE_OFFLINE) && (powerStatus.BatteryFlag!=BATTERY_FLAG_NO_BATTERY))
			{
				dwKeyValue = 5;
				RegSetValueEx(hRegKey,SREGISTRY_BATTERY_SYSTEMIDLE,0,REG_DWORD,(BYTE *)&dwKeyValue,sizeof(DWORD));
				dwKeyValue = 0;
				RegSetValueEx(hRegKey,SREGISTRY_AC_SYSTEMIDLE,0,REG_DWORD,(BYTE *)&dwKeyValue,sizeof(DWORD));
			}
		}
		
		// Tell power manager to reload the registry timeouts
		HANDLE hevReloadActivityTimeouts = 
			OpenEvent(EVENT_ALL_ACCESS, FALSE, _T("PowerManager/ReloadActivityTimeouts"));
		if (hevReloadActivityTimeouts) {
			SetEvent(hevReloadActivityTimeouts);
			CloseHandle(hevReloadActivityTimeouts);
		}
		else {
			RETAILMSG(1, (TEXT("Error: Could not open PM reload event\r\n")));
			return FALSE;
		}

		// done with the key, close it
		RegCloseKey(hRegKey);
	}
	else
	{
		RETAILMSG(1, (TEXT("Tux_PowerManagement: Error opening registry key\r\n")));
		return FALSE;
	}

	return TRUE;
}


static BOOL InitializeStateMachine(void)
{
	HKEY    hRegKey             = NULL;
	DWORD   dwNumSubKeys        = 0;
	LPWSTR  SubKeyName           = NULL;
	LPCWSTR regSubkey		    = TEXT("System\\CurrentControlSet\\Control\\Power\\Timeouts");

    DWORD dwKeyIndex = 0;       
    TCHAR szNewKey[MAX_PATH]; 
	DWORD valuedata;
	DWORD valuedatasize = sizeof(DWORD);
    DWORD dwNewKeySize;       // size of name of DeviceX subkey
    dwNewKeySize = (sizeof(szNewKey) / sizeof(TCHAR));

	SYSTEM_POWER_STATUS_EX2		powerStatus;
	WCHAR* powerStateName[3];

	g_StateInfoTable[dwStateId_on].dwStateTimeout = 0x00;
	wcsncpy(g_StateInfoTable[dwStateId_on].szStateName,L"on",MAX_PATH);
	wcsncpy(g_StateInfoTable[dwStateId_on].szTimeoutName,L"PowerOn",MAX_PATH);
	g_StateInfoTable[dwStateId_on].bEnabled = TRUE;
	g_dwMaxTimeout = 0x00;


	powerStateName[0]=SREGISTRY_AC_USERIDLE;
	powerStateName[1]=SREGISTRY_AC_SYSTEMIDLE;
	powerStateName[2]=SREGISTRY_AC_SUSPEND;

	// If a real battery driver is present
	if (GetSystemPowerStatusEx2(&powerStatus,sizeof(powerStatus),TRUE))
	{
		// If the platform runs currently only on battery
		if ( (powerStatus.ACLineStatus == AC_LINE_OFFLINE) && (powerStatus.BatteryFlag!=BATTERY_FLAG_NO_BATTERY))
		{
			powerStateName[0]=SREGISTRY_BATTERY_USERIDLE;
			powerStateName[1]=SREGISTRY_BATTERY_SYSTEMIDLE;
			powerStateName[2]=SREGISTRY_BATTERY_SUSPEND;
		}
	}
	

	// find all of the states that are defined in the registry
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regSubkey, 0, 0, &hRegKey) == ERROR_SUCCESS)
	{
		//RETAILMSG(1, (TEXT("Tux_PowerManagement: getting PM timeouts from registry\r\n")));


		while (ERROR_SUCCESS == RegEnumValue(hRegKey,dwKeyIndex,
			szNewKey,
			&dwNewKeySize,NULL,NULL,
			(LPBYTE)&valuedata,
			&valuedatasize))
		{
			dwKeyIndex += 1;
			dwNewKeySize = (sizeof(szNewKey) / sizeof(TCHAR));

			//RETAILMSG(1,(TEXT("name: %s   value: %x\r\n"),szNewKey,valuedata));

			if(wcsncmp(szNewKey,powerStateName[0],dwNewKeySize)==0 && valuedata != 0)
			{
				//RETAILMSG(1,(TEXT("mapping useridle: key: %x\r\n"),dwKeyIndex));
				//dwStateId_useridle = dwKeyIndex;
				wcsncpy(g_StateInfoTable[dwStateId_useridle].szTimeoutName,szNewKey,dwNewKeySize);
				g_StateInfoTable[dwStateId_useridle].dwStateTimeout = valuedata;
				wcsncpy(g_StateInfoTable[dwStateId_useridle].szStateName,L"useridle",dwNewKeySize);
				g_StateInfoTable[dwStateId_useridle].bEnabled = TRUE;

				// check if this transition is the longest transition
				if(valuedata > g_dwMaxTimeout)
					g_dwMaxTimeout = valuedata;

				g_numofstates++;
			}
			else if(wcsncmp(szNewKey,powerStateName[1],dwNewKeySize)==0 && valuedata != 0)
			{
				//RETAILMSG(1,(TEXT("mapping systemidle: key: %x\r\n"),dwKeyIndex));
				//dwStateId_systemidle = dwKeyIndex;
				wcsncpy(g_StateInfoTable[dwStateId_systemidle].szTimeoutName,szNewKey,dwNewKeySize);
				g_StateInfoTable[dwStateId_systemidle].dwStateTimeout = valuedata;
				wcsncpy(g_StateInfoTable[dwStateId_systemidle].szStateName,L"systemidle",dwNewKeySize);
				g_StateInfoTable[dwStateId_systemidle].bEnabled = TRUE;

				if(valuedata > g_dwMaxTimeout)
					g_dwMaxTimeout = valuedata;

				g_numofstates++;
			}
			else if(wcsncmp(szNewKey,powerStateName[2],dwNewKeySize)==0 && valuedata != 0)
			{
				//RETAILMSG(1,(TEXT("mapping suspend key: %x\r\n"),dwKeyIndex));
				//dwStateId_suspend = dwKeyIndex;
				wcsncpy(g_StateInfoTable[dwStateId_suspend].szTimeoutName,szNewKey,dwNewKeySize);
				g_StateInfoTable[dwStateId_suspend].dwStateTimeout = valuedata;
				wcsncpy(g_StateInfoTable[dwStateId_suspend].szStateName,L"suspend",dwNewKeySize);
				g_StateInfoTable[dwStateId_suspend].bEnabled = TRUE;

				if(valuedata > g_dwMaxTimeout)
					g_dwMaxTimeout = valuedata;

				g_numofstates++;
			}
		}		

		// done with the key, close it
		RegCloseKey(hRegKey);
	}
	else
	{
		RETAILMSG(1, (TEXT("Tux_PowerManagement: Error opening registry key\r\n")));
		return FALSE;
	}

	return TEST_RUNNING;
}

static DWORD CheckStateMachine(POWER_BROADCAST *ppb, FILETIME *systime)
{
	if(ppb == NULL || systime == NULL)
	{
		g_pKato->Log(LOG_ABORT,(TEXT("Invalid parameters passed to CheckStateMachine\r\n")));
		return TEST_ERROR;
	}

	if(wcsncmp(ppb->SystemPowerState,SSTATE_NAME_ON,sizeof(ppb->SystemPowerState))==0)
	{
		//RETAILMSG(1,(TEXT("got state1: %s, %x\r\n"),g_StateInfoTable[dwStateId_on].szStateName,g_StateInfoTable[dwStateId_on].dwStateTimeout));
		
		bOnState = TRUE;

		if(!UpdateTestTicker(dwStateId_on, systime->dwLowDateTime))
			return TEST_ERROR;

		if(IsLastState(dwStateId_on))
		{
			g_pKato->Log(LOG_DETAIL,(TEXT("Test Finished Successfully on \"On\" State\r\n")));
			return TEST_COMPLETE;
		}

		g_PreviousState = dwStateId_on;
	}
	else if(wcsncmp(ppb->SystemPowerState,SSTATE_NAME_USERIDLE,sizeof(ppb->SystemPowerState))==0)
	{
		//RETAILMSG(1,(TEXT("got state2: %s, %x\r\n"),g_StateInfoTable[dwStateId_useridle].szStateName,g_StateInfoTable[dwStateId_useridle].dwStateTimeout));
		bUserIdleState = TRUE;

		// we got user idle state, if previous state wasn't on, something is wrong
		if(g_PreviousState != dwStateId_on)
		{
			g_pKato->Log(LOG_DETAIL,(TEXT("Test failed on transition to User Idle\r\n")));
			return TEST_FAIL;
		}

		// check this state's timeout to see if it matches the registry
		if(!UpdateTestTicker(dwStateId_useridle, systime->dwLowDateTime))
			return TEST_ERROR;
		
		if(IsLastState(dwStateId_useridle))
		{
			g_pKato->Log(LOG_DETAIL,(TEXT("Test Finished Successfully on \"User Idle\" State\r\n")));
			return TEST_COMPLETE;
		}

		g_PreviousState = dwStateId_useridle;
	}
	else if(wcsncmp(ppb->SystemPowerState,SSTATE_NAME_SYSTEMIDLE,sizeof(ppb->SystemPowerState))==0)
	{
		//RETAILMSG(1,(TEXT("got state3: %s, %x\r\n"),g_StateInfoTable[dwStateId_systemidle].szStateName,g_StateInfoTable[dwStateId_systemidle].dwStateTimeout));
		bSystemIdleState = TRUE;

		// we got system idle state, if previous state wasn't user idle, something is wrong
		if(g_PreviousState != dwStateId_useridle)
		{
			g_pKato->Log(LOG_DETAIL,(TEXT("Test failed on transition to System Idle\r\n")));
			return TEST_FAIL;
		}

		if(!UpdateTestTicker(dwStateId_systemidle, systime->dwLowDateTime))
			return TEST_ERROR;

		if(IsLastState(dwStateId_systemidle))
		{
			g_pKato->Log(LOG_DETAIL,(TEXT("Test Finished Successfully on \"System Idle\" State\r\n")));
			return TEST_COMPLETE;
		}

		g_PreviousState = dwStateId_systemidle;
	}
	else if(wcsncmp(ppb->SystemPowerState,SSTATE_NAME_SUSPEND,sizeof(ppb->SystemPowerState))==0)
	{
		//RETAILMSG(1,(TEXT("got state4: %s, %x\r\n"),g_StateInfoTable[dwStateId_suspend].szStateName,g_StateInfoTable[dwStateId_suspend].dwStateTimeout));
		bSuspendState = TRUE;

		// we got suspend state, if previous state wasn't system idle, something is wrong
		if(g_PreviousState != dwStateId_systemidle)
		{
			g_pKato->Log(LOG_DETAIL,(TEXT("Test failed on transition to Suspend\r\n")));
			return TEST_FAIL;
		}

		if(!UpdateTestTicker(dwStateId_suspend, systime->dwLowDateTime))
			return TEST_ERROR;

		if(IsLastState(dwStateId_suspend))
		{
			g_pKato->Log(LOG_DETAIL,(TEXT("Test Finished Successfully on \"Suspend\" State\r\n")));
			return TEST_COMPLETE;
		}

		g_PreviousState = dwStateId_suspend;
	}
	else
	{
		// PM notified us of a state that shouldn't be transitioning to
		g_pKato->Log(LOG_DETAIL,(TEXT("Received invalid power state transition\r\n")));
		return TEST_FAIL;
	}

	return TRUE;
}

static BOOL IsLastState(DWORD dwStateId)
{
	DWORD dwNextStateId = dwStateId + 1;

	if(dwStateId == dwStateId_suspend)
	{
		return TRUE;
	}
	else if(g_StateInfoTable[dwNextStateId].bEnabled == FALSE)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static BOOL UpdateTestTicker(DWORD dwStateId, DWORD tcount)
{
	DWORD dwElapsedTime;
	dwElapsedTime = (tcount-g_TestStartTime)/(10000000);

	if(dwStateId == dwStateId_on)
	{
		g_TestStartTime = tcount;
		g_pKato->Log(1,(TEXT("Transition: ON state\r\n")));
		//RETAILMSG(1,(TEXT("Transition Detected: On					Starting to measure elapsed time.\r\n")));
		//return TRUE;
	}
	else if(dwStateId == dwStateId_useridle)
	{
		g_pKato->Log(1, TEXT("Transition: ON->USERIDLE, measured timeout: %d, expected timeout: %d\r\n"),
			dwElapsedTime,
			g_StateInfoTable[dwStateId_useridle].dwStateTimeout);

		g_TestStartTime = tcount;
	}
	else if(dwStateId == dwStateId_systemidle)
	{
		g_pKato->Log(1, TEXT("Transition: USERIDLE->SYSTEMIDLE, measured timeout: %d, expected timeout: %d\r\n"),
			dwElapsedTime,
			g_StateInfoTable[dwStateId_systemidle].dwStateTimeout);

		g_TestStartTime = tcount;
	}
	else if(dwStateId == dwStateId_suspend)
	{
		g_pKato->Log(1, TEXT("Transition: SYSTEMIDLE->SUSPEND\r\n"));

		g_TestStartTime = tcount;
	}
	else
		return FALSE;

	return TRUE;
}

#pragma warning(pop)