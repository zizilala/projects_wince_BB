// ComProtocol.cpp: implementation of the CComProtocol class.
//
//////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <ceddk.h>
#include <bsp.h>
#include "ComProtocol.h"

#define REG_NAME_KEY		L"Key"
#define REG_NAME_NAME		L"Name"
#define REG_NAME_ACTIVE		L"Drivers\\Active"

int CheckNameIndex(TCHAR * szName)
{
	TCHAR cBuf[6];	// this is the max it should be
	int iNum;
	int iRet;
		
	wcsncpy(cBuf, szName, 6);
	if(wcsncmp(cBuf, L"COM", 3) != 0)
		return -1;
	
	iRet = swscanf(cBuf, L"COM%d:", &iNum);
	if((iRet == 0) || (iRet == EOF))
		return -1;
	
	return iNum;
}

BOOL CheckDeviceType(HKEY hSubKey,LPTSTR DeviceKey)
{
	INT rc;
	TCHAR szName[128];
	DWORD dwType, dwSize;

	dwSize = sizeof(szName);

	rc = RegQueryValueEx(hSubKey, REG_NAME_KEY, 0, &dwType, (PBYTE)szName, &dwSize);
	if(rc !=  ERROR_SUCCESS)
		return FALSE;

	if(wcslen(szName) >= wcslen(DeviceKey)) {
		if(wcsncmp(szName, DeviceKey, wcslen(DeviceKey)) == 0)
			return TRUE;
	}

	return FALSE;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CComProtocol::CComProtocol()
{
   
}

CComProtocol::~CComProtocol()
{

}


BOOL  CComProtocol::EnumCOMPorts(LPTSTR DeviceKey,LPTSTR COMPortStr)
{
	int index = 0, iRet;
	HKEY hKey, hSubKey;
	TCHAR szName[MAX_PATH];
	DWORD dwType, dwSize;
	DWORD dwNameMask = 0;
	
	
        wcscpy(COMPortStr,L"UnKnow");
      
        
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_NAME_ACTIVE, 0,0, &hKey) != ERROR_SUCCESS)
	 return FALSE;

	while (1) 
	{
		// Enumerate active driver list
		dwSize = sizeof(szName);
		
		if(RegEnumKeyEx (hKey, index++, szName, &dwSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
			 break;

		// Open active driver key.
		iRet = RegOpenKeyEx(hKey, szName, 0, 0, &hSubKey);
		
		if(iRet !=  ERROR_SUCCESS)
			 continue;

		dwSize = sizeof(szName);
		
		iRet = RegQueryValueEx(hSubKey, REG_NAME_NAME, 0, &dwType, (PBYTE)szName, &dwSize);
		
		
		if(iRet ==  ERROR_SUCCESS)
		 {
			if((CheckNameIndex(szName) != -1) && (CheckDeviceType(hSubKey,DeviceKey))) 
			{
				RegCloseKey(hSubKey);
				RegCloseKey(hKey);
				
				 wcscpy(COMPortStr,&szName[0]);
				
				return TRUE;
			}
		 }
		 
	 RegCloseKey(hSubKey);
	}

RegCloseKey(hKey);
return FALSE;
}


HANDLE CComProtocol:: SetupCOMPort(LPCTSTR  COMPort,DWORD BaudRate,BYTE Parity,BYTE ByteSize,BYTE StopBits)
{    
        COMMTIMEOUTS cto;
        DCB PortDCB;   
         
        HANDLE hComPort=INVALID_HANDLE_VALUE;	
        
                                                                                        	  
	RETAILMSG(1, (L"\n\r+SetupCOMPort:%s BaudRate:%d\r\n",COMPort,BaudRate ));   
	
	hComPort = CreateFile(COMPort,	// Pointer to the name of the port
	GENERIC_READ | GENERIC_WRITE,	// Access (read-write) mode
	0,				// Share mode
	NULL,				// Pointer to the security attribute
	OPEN_EXISTING,			// How to open the serial port
	0,				// Port attributes
	NULL);			         // Handle to port with attribute to copy
	
	
	if(hComPort)
	{
	       memset(&PortDCB, 0, sizeof(DCB));
			
	       //set the COMM setting			 	
	       if(!GetCommState(hComPort,&PortDCB))
	       {
          //ERRORMSG(1, (L"\n\rOpenCOM->GetCommState fail (%d)\r\n",GetLastError()));
          
          CloseCOMPort(hComPort);  
          
          MessageBox(NULL,L"GetCommState Fail",L"",MB_OK|MB_TOPMOST); 
          
          return NULL;
         }	 	 
	       
    
         PortDCB.BaudRate = BaudRate;         // Current baud 
         PortDCB.fBinary = TRUE;               // Binary mode; no EOF check 
         PortDCB.fParity = FALSE;              // Enable parity checking 
         PortDCB.fOutxCtsFlow = FALSE;         // No CTS output flow control 
         PortDCB.fOutxDsrFlow = FALSE;         // No DSR output flow control 
         PortDCB.fDtrControl = DTR_CONTROL_ENABLE; 
                                               // DTR flow control type 
         PortDCB.fDsrSensitivity = FALSE;      // DSR sensitivity 
         PortDCB.fTXContinueOnXoff = FALSE;    // XOFF continues Tx 
         PortDCB.fOutX = FALSE;                // No XON/XOFF out flow control 
         PortDCB.fInX = FALSE;                 // No XON/XOFF in flow control 
         PortDCB.fErrorChar = FALSE;           // Disable error replacement 
         PortDCB.fNull = FALSE;                // Disable null stripping 
         PortDCB.fRtsControl = RTS_CONTROL_ENABLE ; 
                                               // RTS flow control 
         PortDCB.fAbortOnError = FALSE;        // Do not abort reads/writes on 
                                               // error
         PortDCB.ByteSize = ByteSize;          // Number of bits/byte, 4-8 
         PortDCB.Parity = Parity;              // 0-4=no,odd,even,mark,space 
         PortDCB.StopBits = StopBits;          // 0,1,2 = 1, 1.5, 2 
         
         SetCommState(hComPort, &PortDCB);
	 
	 
         //set the COMM timeout                
         cto.ReadIntervalTimeout	= 50;
         cto.ReadTotalTimeoutMultiplier	= 1 ;
         cto.ReadTotalTimeoutConstant	= 500;
         cto.WriteTotalTimeoutMultiplier= 1;
         cto.WriteTotalTimeoutConstant	= 50;												 
         SetCommTimeouts(hComPort, &cto);
	
         //clear thr input and output buffer
         PurgeComm(hComPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
        
         EscapeCommFunction (hComPort, CLRBREAK); 
         EscapeCommFunction (hComPort, CLRDTR);
         EscapeCommFunction (hComPort, CLRRTS); 
			
         EscapeCommFunction (hComPort, SETDTR);
         EscapeCommFunction (hComPort, SETRTS);  
       }
    
 	//RETAILMSG(1, (L"\n\r-SetupCOMPort:%x\r\n",hComPort ));              

return hComPort;
     

}


void CComProtocol::CloseCOMPort(HANDLE PortHandle)
{
   
	RETAILMSG(1, (L"CloseCOMPort\r\n" ));
     // disable event notification and wait for thread to halt
    SetCommMask(PortHandle, 0);
    
    // drop DTR
    EscapeCommFunction(PortHandle, CLRDTR);    
    
    EscapeCommFunction (PortHandle, CLRRTS); 
    
    
    
    // purge any outstanding reads/writes and close device handle
    PurgeComm(PortHandle, PURGE_TXABORT | PURGE_RXABORT |
              PURGE_TXCLEAR | PURGE_RXCLEAR);
    
    CloseHandle(PortHandle);


}


BOOL  CComProtocol::BaudRate(HANDLE PortHandle,DWORD BaudRate)
{
     DCB PortDCB; 
   
     GetCommState(PortHandle, &PortDCB);
     
     
     PortDCB.BaudRate = BaudRate;        
   
     return SetCommState(PortHandle, &PortDCB);


}


BOOL CComProtocol::WriteCOMPort(HANDLE PortHandle,LPCVOID DataBuffer,DWORD NumOfBytes) 
{      	
	BOOL status=FALSE;
	DWORD dwBytes=0; 	
	PBYTE lpBuffer=new BYTE[NumOfBytes];		 	
	 
	memcpy(&lpBuffer[0],(PBYTE)DataBuffer,NumOfBytes);                         	 	  	 		
	   
	 //Send the command		
	PurgeComm(PortHandle,PURGE_TXCLEAR);		 
	 
	for(DWORD index=0;index<NumOfBytes;index++)
	{	 
		if(!WriteFile (PortHandle,&lpBuffer[index],1,&dwBytes,NULL))
			break;  
	} 	 
	 
	delete lpBuffer;
	 
	(index==NumOfBytes) ? status=TRUE : status=FALSE ;
         
	return status;       
}
    

BOOL CComProtocol::Write_RFID_CMD(HANDLE hDevComPort,char  *CMDString)
{      	
	DWORD  dwSize=strlen(CMDString);					
	   
	BOOL status=WriteCOMPort(hDevComPort,CMDString,dwSize); 	
              
	return status; 
}

BOOL  CComProtocol::Read_RFID_CMD (HANDLE hDevComPort,PBYTE lpDataBuffer,DWORD dwBufferSize ,LPDWORD  lpNumberOfBytesRead)
{
	BOOL   status=FALSE;
	BYTE   dwByte;
	DWORD  dwBytesTransferred,iBytes;
  
	iBytes=0;
      // Loop for waiting for the data.
	do
	{
        // Read the data from the serial port.      
		status = ReadFile(hDevComPort,&dwByte, 1, &dwBytesTransferred, 0);
        if(!dwBytesTransferred)
             break;                         
        else  
		{
			iBytes++;
			if(iBytes<dwBufferSize)           
			{
				*lpDataBuffer=dwByte;
				lpDataBuffer++;
			}
		}
	} while (status); 
	*lpNumberOfBytesRead=iBytes;
        
	return iBytes ? TRUE : FALSE;
}
