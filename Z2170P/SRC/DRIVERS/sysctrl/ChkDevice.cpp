#include <windows.h>
#include <ceddk.h>
#include <bsp.h>
#include "sysctrl.h"
#include "ComProtocol.h"

BOOL ChkBuiltInRFID(void)
{
	CComProtocol hRFID;
	TCHAR COMString[32];
	BYTE szBufffer[128];
	DWORD dwReturn=0;
	HANDLE hComPort=INVALID_HANDLE_VALUE;
	int i = 5;
	BOOL retval;

	RETAILMSG(1, (TEXT("+ChkBuiltInRFID: EnumCOMPorts\n\r")));
	if(hRFID.EnumCOMPorts(L"Drivers\\BuiltIn\\BC_UART",COMString))
	{
		RETAILMSG(1, (TEXT("ChkBuiltInRFID: SetupCOMPort: %s\n\r"),COMString));
		hComPort=hRFID.SetupCOMPort(COMString,CBR_115200, NOPARITY, 8,ONESTOPBIT);
		if( hComPort!=INVALID_HANDLE_VALUE)
		{			
			retval = hRFID.Write_RFID_CMD(hComPort,"010B000304140401000000"); //UUID CMD
			RETAILMSG(1, (TEXT("ChkBuiltInRFID: Write_RFID_CMD - %d\n\r"),retval));
			do
			{  
				retval = hRFID.Read_RFID_CMD(hComPort,&szBufffer[0],128,&dwReturn);
				RETAILMSG(1, (TEXT("ChkBuiltInRFID: Read_RFID_CMD - %d - %d\n\r"),retval,dwReturn));
				if(retval)           
					break;
			}while(i & i--);
			hRFID.CloseCOMPort(hComPort);
		}
	}
	return dwReturn ? TRUE : FALSE;
}