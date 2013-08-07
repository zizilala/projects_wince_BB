#include <windows.h>
#include <ceddk.h>
#include <bsp.h>
#include "sysctrl.h"

extern BOOL  FlashWrite(PBYTE lpFlashBaseAddress, UINT8 *pDataBuffer, DWORD DataLength);

BOOL GetSerialNumber(LPTSTR lpszSerial)
{
	  PHYSICAL_ADDRESS RegPA;    	       
    
    char TempStr[MAX_PATH];	
    
    volatile ZSYSDeviceID *lpDeviceID=NULL;        
    
    RegPA.QuadPart = ZEBEX_DEVICE_ID_PA_START;
      
    lpDeviceID = ( volatile ZSYSDeviceID *) MmMapIoSpace(RegPA, sizeof(ZSYSDeviceID), FALSE);        
    
    wcscpy(lpszSerial,L"");
    
    RETAILMSG(1, (TEXT("\n\r+GetSerialNumber\r\n"))); 
    
    if(lpDeviceID)
    {	     
     if(strlen((char *)lpDeviceID->SerialNumberStr))
     { 
      strcpy(TempStr,(char *)lpDeviceID->SerialNumberStr); 
      
      RETAILMSG(1, (TEXT("\n\rSN: %S\r\n"),TempStr)); 
      
      wsprintf(lpszSerial,L"%S",TempStr);		            
     }
      VirtualFree((void *)lpDeviceID, 0, MEM_RELEASE);	      
    }
    
    RETAILMSG(1, (TEXT("\n\r-GetSerialNumber\r\n"))); 
    
   return wcslen(lpszSerial) ? TRUE : FALSE;
}

BOOL SetSerialNumber(LPTSTR lpszSerial)
{
	  PHYSICAL_ADDRESS RegPA;     
	   
    volatile BYTE *lpStartadd=NULL;
    
    ZSYSDeviceID  dwID;  
    
    BYTE dwBuffer[FLASH_BLOCK_SIZE];                 
    
    strcpy(dwID.SerialNumberStr,""); 
    
    RegPA.QuadPart = ZEBEX_DEVICE_ID_PA_START;
    
    RETAILMSG(1, (TEXT("\n\r+SetSerialNumber\r\n"))); 
    
      
  if(FLASH_BLOCK_SIZE>=sizeof(ZSYSDeviceID))
  {	    
    lpStartadd = ( volatile BYTE *) MmMapIoSpace(RegPA, FLASH_BLOCK_SIZE, FALSE);              
 
    
    if(lpStartadd)
    {	 
    	if(wcslen(lpszSerial)&& (wcslen(lpszSerial)<sizeof(dwID.SerialNumberStr)))
    	{	
    	 memcpy((PBYTE)dwBuffer,(PBYTE)lpStartadd,FLASH_BLOCK_SIZE);	
    	 
    	 memcpy((PBYTE)&dwID,(PBYTE)lpStartadd,sizeof(ZSYSDeviceID));  	
    	 
    	 memcpy((PBYTE)dwBuffer,(PBYTE)&dwID, sizeof(ZSYSDeviceID));
    	 
    	 sprintf(dwID.SerialNumberStr,"%S",lpszSerial);	
    	 
    	 RETAILMSG(1, (TEXT("\n\rSN: %S\r\n"),dwID.SerialNumberStr));    	     	 
    	
    	 FlashWrite((PBYTE)lpStartadd, (PBYTE)dwBuffer, FLASH_BLOCK_SIZE);    	      	
      }
      
      VirtualFree((void *)lpStartadd, 0, MEM_RELEASE);		
      
      
    }
  }  
  
   RETAILMSG(1, (TEXT("\n\r-SetSerialNumber\r\n"))); 
   
   
   return strlen(dwID.SerialNumberStr) ? TRUE : FALSE;
}