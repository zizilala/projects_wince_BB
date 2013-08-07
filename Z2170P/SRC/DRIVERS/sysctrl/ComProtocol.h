#ifndef __ComProtocol_H__
#define __ComProtocol_H__

#define MAX_Buffer_Size (1024*sizeof(TCHAR))

#define MAX_CMD_STR 1024

class CComProtocol  
{
private:
        
        
public:
 
  
  CComProtocol();
  
  ~CComProtocol();  
  
   BOOL   EnumCOMPorts(LPTSTR DeviceKey,LPTSTR COMPortStr); 
  
   HANDLE SetupCOMPort(LPCTSTR  COMPort,DWORD BaudRate,BYTE Parity,BYTE ByteSize,BYTE StopBits);  
   
   void   CloseCOMPort(HANDLE PortHandle);
   
   BOOL   BaudRate(HANDLE PortHandle,DWORD BaudRate);
   
  BOOL WriteCOMPort(HANDLE PortHandle,LPCVOID DataBuffer,DWORD NumOfBytes) ;
  
  BOOL Write_RFID_CMD(HANDLE hDevComPort,char  *CMDString);
  
  BOOL Read_RFID_CMD (HANDLE hDevComPort,PBYTE lpDataBuffer,DWORD dwBufferSize ,LPDWORD  lpNumberOfBytesRead);
  
};


#endif  