// All rights reserved ADENEO EMBEDDED 2010
// wakeme.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <notify.h>
#include "pm.h"

// Defines
#define DEFAULT_SUSPEND_TIME 120

#define RESUME_EVENT L"ResumeTestEvent"
#define RTC_RESOLUTION 60

int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{
	HANDLE     hEvent;
	SYSTEMTIME curSysTime;
	FILETIME fileTime;
	int numSecs = DEFAULT_SUSPEND_TIME;

	UNREFERENCED_PARAMETER(envp);

	RETAILMSG(1, (L"Wakeme: Application Started.\r\n"));

	//Check Application Parameters
	if(argc<2) //If there is no parameter, load default value
	{
		RETAILMSG(1, (L"No RTC Timeout Specified. Using Default of %d\r\n", numSecs));
	}
	else if(argc>2 || wcscmp(argv[1],L"?") == 0)
	{
		RETAILMSG(1, (L"Usage:\r\n"));
		RETAILMSG(1, (L"Help: wakeme ?\r\n"));
		RETAILMSG(1, (L"Custom RTC Timeout: wakeme <positive number in seconds>\r\n"));
		RETAILMSG(1, (L"i.e. wakeme 60 --> this will make the RTC timeout 60 seconds.\r\n"));
		goto EXIT;
	}
	else if(argc == 2 )// Set RTC Timeout
	{
		numSecs = (DWORD)_wtol(argv[1]);
		RETAILMSG(1, (L"RTC Timeout Set to %d\r\n", numSecs));
			
		if(numSecs <= 0)
		{
			RETAILMSG(1, (L"Negative and Zero RTC Timeout Not Supported. Exiting...\r\n %d\r\n", numSecs));
			goto EXIT;
		}		
	}

	// create a named event
	hEvent = CreateEvent( NULL, FALSE, FALSE, RESUME_EVENT );

	// create user notification
	GetLocalTime( &curSysTime );
	SystemTimeToFileTime(&curSysTime, &fileTime);
	*((UINT64*)&fileTime) += (numSecs*10000000);
	FileTimeToSystemTime(&fileTime, &curSysTime);

	if(CeRunAppAtTime(L"\\\\.\\Notifications\\NamedEvents\\"RESUME_EVENT, &curSysTime) == FALSE)
	{
		RETAILMSG(1, (L"CeRunAppAtTime failed! (error: %d)\r\n", GetLastError()));
		goto EXIT;
	}
	
	// Suspend the system
	RETAILMSG(1, (L"Wakeme: System will suspend in 5 seconds, RTC will wake system %d seconds after...\r\n", numSecs));
	Sleep(5000);
	SetSystemPowerState(NULL, POWER_STATE_SUSPEND, POWER_FORCE);
	
	// wait for my event
	if (WaitForSingleObject(hEvent,(numSecs+RTC_RESOLUTION+1)*1000) == WAIT_TIMEOUT)
	{
		RETAILMSG(1,(TEXT("RTC alarm timed out!\r\n")));
	}
	else
	{
		// my event was triggered
		RETAILMSG(1, (L"youve been woken up\r\n"));
	}	 

	EXIT:
    return 0;
}

