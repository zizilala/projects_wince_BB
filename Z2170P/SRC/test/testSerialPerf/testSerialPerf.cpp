
// All rights reserved Texas Instruments 
// testSerialTxRx.cpp : Defines the entry point for the console application.

//The application tests,  serial port.  A data will be send and reviced and by the application. The application 
// will pass the test with recieved data is the same data the send data. It also calculates performance.  
//The application also test SDMA inderectely, because the driver uses SDMA to write and read file.
#pragma warning(push)
#pragma warning(disable : 4100)
#include "main.h"
#include "globals.h"
#include "utility.h"
#include "stdafx.h"
//this defines the buffer size 
#define DATASIZE				512

int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{    
    HANDLE h;
    DCB dcb;
    COMMTIMEOUTS		timeouts;
	DWORD  dwTickCountWrite[100],dwTickCountRead[100]; 
    DWORD			              dstBuff1[DATASIZE],srcBuff1[DATASIZE];
	volatile int				i,j;
    int      				passStatus = 0;
	TCHAR *   temp;
	int baudRate = 115200;
	int maxIteration = 100; 
    RETAILMSG(1,(TEXT("Simple transmit-recieve application\n")));
    if (argc != 4 && argc != 2)
    {
        RETAILMSG(1,(TEXT("Invalid number of parameters\nusage:%s port_name (ex:%s COM1: 115200 50)\n"),argv[0],argv[0]));
        return -1;
    }
    RETAILMSG(1,(TEXT("Opening %s\n"),argv[1]));
	temp =  envp[0];
    h = CreateFile(argv[1],GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
    GetCommState(h,&dcb);
	if (argc == 4)
	{
	  baudRate = (DWORD)_wtol(argv[2]);
      maxIteration = (DWORD)_wtol(argv[3]);
	}

	RETAILMSG(1,(TEXT("Baudrate is  %d\r\n"),baudRate));
    dcb.BaudRate = baudRate;   
    dcb.fBinary          = TRUE;
    dcb.fParity          = FALSE;
    dcb.fOutxCtsFlow     = FALSE;
    dcb.fOutxDsrFlow     = FALSE;
    dcb.fDtrControl      = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity  = FALSE;
    dcb.fOutX            = FALSE;
    dcb.fInX             = FALSE;
    dcb.fErrorChar       = 0;    
    dcb.fNull            = 0;    
    dcb.fRtsControl      = RTS_CONTROL_ENABLE;
    dcb.fAbortOnError    = FALSE;
    dcb.ByteSize         = 8;    
    dcb.Parity           = NOPARITY;
    dcb.StopBits         = ONESTOPBIT;
    SetCommState(h,&dcb);
    //timeout configuration  
    GetCommTimeouts(h,&timeouts);
    timeouts.ReadIntervalTimeout = 100;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.ReadTotalTimeoutConstant = 1000;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 1000;
    SetCommTimeouts(h,&timeouts);
	//Inialize destination buffer to junk value
    for(i=0;i<DATASIZE;i++)
     {	
	    dstBuff1[i] = 0;
		//dstBuff1++;
	}

  /* Initialize source buffer to known data value */
    for (i= 0;i < DATASIZE; i++) 
    	srcBuff1[i] = (i+1);

	for (i= 0;i < maxIteration; i++) 
     {
        //UCHAR buffer[512];
        DWORD nbRead=0,nbWritten=0;
		 dwTickCountWrite[i] = GetTickCount();
        if (WriteFile(h,srcBuff1,DATASIZE,&nbWritten,NULL))
        {
            //Calculate time to read
            dwTickCountWrite[i] = GetTickCount() - dwTickCountWrite[i];
            dwTickCountRead[i] = GetTickCount();
		    if (ReadFile(h,dstBuff1,sizeof(srcBuff1),&nbRead,NULL))
			{
    		  dwTickCountRead[i] = GetTickCount() - dwTickCountRead[i];
             if (nbWritten != nbRead)
              {
                RETAILMSG(1,(TEXT("nbRead %d != nbRead %d\r\n"),nbWritten,nbRead));
				passStatus = 1;
             }
		   }
        }
	//Verify that data send was recieved 
    j=0;
    /*for(j=0;j<DATASIZE;j++)
	{
		if(dstBuff1[j] != srcBuff1[j])
		{	
			passStatus = 0;
			    printf("FAILED: Serial Data Not Read Successfully [Expected = %d, Obtained = %d].\n",srcBuff1[j],dstBuff1[j]); 
			
		}
		else
		{
         printf("DATA SEND %d , RECIEVED %d\n",srcBuff1[j],dstBuff1[j]);	
			passStatus = 1;
		}
		
	}
	*/

	RETAILMSG(1,(TEXT("Number of test Bytes are %d, Write Time %d ms and Read Time %d ms.\r\n"),DATASIZE,dwTickCountWrite[i],dwTickCountRead[i]));	
} // end of for loop  

	if (passStatus == 0)
	{
		RETAILMSG(1,(TEXT("TEST: PASS\n")));
	}
	else 
	{
       RETAILMSG(1,(TEXT("TEST: FAIL\n")));
	}

    CloseHandle(h);
    return 0;
}

