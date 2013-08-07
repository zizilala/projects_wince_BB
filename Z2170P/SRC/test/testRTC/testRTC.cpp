// All rights reserved ADENEO EMBEDDED 2010
// testRTC.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <notify.h>
#include <sdk_gpio.h>

#define TEST_EVENT_NAME L"MyTestEvent"

int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{
    int nbsec = 25;
    int notifDelay;
    SYSTEMTIME systime;
    FILETIME fileTime;


    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(envp);

    HANDLE hEvent = CreateEvent(NULL,FALSE,FALSE,TEST_EVENT_NAME);

    _tprintf(_T("Test RTC\n"));

    GetLocalTime(&systime);    
    SystemTimeToFileTime(&systime,&fileTime);
    *((UINT64*)&fileTime) += (nbsec*10000000);
    FileTimeToSystemTime(&fileTime,&systime);

    DWORD date = GetTickCount();
    if (CeRunAppAtTime(L"\\\\.\\Notifications\\NamedEvents\\"TEST_EVENT_NAME,&systime) == FALSE)
    {
        RETAILMSG(1,(TEXT("CeRunAppAtEvent failed (error : %d)\r\n"),GetLastError()));
        return -1;
    }
  
    if (WaitForSingleObject(hEvent,(nbsec+10+1)*1000) == WAIT_TIMEOUT)
    {
        RETAILMSG(1,(TEXT("RTC alarm failed\r\n")));
        return -1;
    }

    notifDelay = (GetTickCount() - date)/1000;
    RETAILMSG(1,(TEXT("notification delay %d seconds (expected %d)\r\n"),notifDelay,nbsec));
    

    return 0;
}

