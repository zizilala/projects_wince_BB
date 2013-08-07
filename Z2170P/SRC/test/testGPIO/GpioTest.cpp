// All rights reserved ADENEO EMBEDDED 2010
// GpioTest.cpp : Defines the entry point for the console application.
//

#include "omap.h"
#include "sdk_gpio.h"
#include "gpio_ioctls.h"

int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{
    DWORD id = 168;
    if (argc == 2)
    {
        id = _wtoi(argv[1]);
    }
    _tprintf(_T("Hello World!\n"));
#if 0
    DWORD config[2] = {id,GPIO_DIR_OUTPUT};

    HANDLE h = CreateFile(L"GIO1:",0,0,NULL,OPEN_EXISTING,0,NULL);
    DeviceIoControl(h,IOCTL_GPIO_SETMODE,config,sizeof(config),NULL,0,NULL,NULL);

    for (;;)
    {
        DeviceIoControl(h,IOCTL_GPIO_SETBIT,config,sizeof(config),NULL,0,NULL,NULL);
        Sleep(10);
        DeviceIoControl(h,IOCTL_GPIO_CLRBIT,config,sizeof(config),NULL,0,NULL,NULL);
        Sleep(10);
    }
    CloseHandle(h);
#else
    HANDLE h = GPIOOpen();
    GPIOSetMode(h,id,GPIO_DIR_OUTPUT);
    for (;;)
    {
        GPIOSetBit(h,id);
        Sleep(10);
        GPIOClrBit(h,id);
        Sleep(10);
    }
    GPIOClose(h);
#endif
    return 0;
}

