#include <windows.h>
#include <windev.h>
#include <winbase.h>
#include <nkintr.h>
#include <bsp.h>
#include "keybdpdd.h"
#include "resource.h"
#include "sysctrl.h"

#define UID_BT_TASK     0x00000011


#define BT_TASKBARMSG   (WM_USER + 101)


static NOTIFYICONDATA   BT_NotifyIcon;


extern HINSTANCE ghInstance;

extern void LaunchProgram(LPCTSTR szProgramFile, LPCTSTR szParameter);

LRESULT IconProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static HWND   hTrayIcon = NULL;



DWORD TrayIconThread(LPVOID pvarg)
{
    LPCTSTR     szClassName = _T("ZTrayIcon");

    WNDCLASS    wc;

    /*while(!IsAPIReady(SH_SHELL))
        {}*/
    WaitForAPIReady(SH_SHELL, INFINITE);


    if(hTrayIcon == NULL)
    {
        ZeroMemory(&wc, sizeof(wc));
        wc.lpfnWndProc = IconProc;
        wc.hInstance   = GetModuleHandle(NULL);
        wc.lpszClassName = szClassName;
        wc.hCursor = NULL;
        wc.hbrBackground = NULL;
        RegisterClass(&wc);

        hTrayIcon = CreateWindow(szClassName, _T(""), WS_POPUP | WS_CLIPCHILDREN, 0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);


        MSG  msg;

        while(GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        hTrayIcon = NULL;
    }


    RETAILMSG(1, (TEXT("\n\rExitIconProc -> TrayIconThread\n\r")));

    return 0;
}

LRESULT IconProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    UINT uID = (UINT)wParam;

    if(uID == UID_BT_TASK)
    {
        // what kind of message did we get
        switch((UINT)lParam)
        {
            case WM_LBUTTONDBLCLK:
                LaunchProgram(L"ctlpnl.exe", L"bthpnl.cpl");
                return 0;

            default:
                break;
        }
    }




    switch(uMsg)
    {
        case WM_CREATE:
            return 0;

        case WM_COMMAND:
            return 0;



        case WM_QUIT:
            RETAILMSG(1, (TEXT("\n\rIconProc -> WM_QUIT")));
            return 0;

        case WM_DESTROY:

            RETAILMSG(1, (TEXT("\n\rIconProc -> WM_DESTROY\n\r")));

            PostQuitMessage(0);

            return 0;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);

    }


    return 0;
}




BOOL AddBluetoothTrayIcon(void)
{

    BOOL    status = FALSE;

    HICON   hIcon;

    BYTE  	icount = 0;

    RETAILMSG(1, (TEXT("\n\r+AddBluetoothTrayIcon\r\n")));

    if(!hTrayIcon)
    {
        return FALSE;
    }

    hIcon = (HICON) LoadImage(ghInstance, MAKEINTRESOURCE(IDI_BT), IMAGE_ICON, 16, 16, 0);

    if(hIcon == NULL)
    {
        ERRORMSG(1, (TEXT("\n\rAddBluetoothTrayIcon--> LoadImage failed !!!\r\n")));
    }

    BT_NotifyIcon.cbSize = sizeof(NOTIFYICONDATA);
    BT_NotifyIcon.hWnd = hTrayIcon;
    BT_NotifyIcon.uID = UID_BT_TASK;
    BT_NotifyIcon.uFlags = NIF_ICON | NIF_MESSAGE;
    BT_NotifyIcon.uCallbackMessage = BT_TASKBARMSG;
    BT_NotifyIcon.hIcon = hIcon;

    // add the icon
    while(hIcon && icount < 10)
    {

        status = Shell_NotifyIcon(NIM_ADD, &BT_NotifyIcon);

        if(status)
        {
            break;
        }

        icount++;

        Sleep(300);
    }

    RETAILMSG(1, (TEXT("\n\r-AddBluetoothTrayIcon (%x)\r\n"), status));


    return status;
}




BOOL RemoveBluetoothTrayIcon(void)
{
    //RETAILMSG(1, (TEXT("\n\r+RemoveBluetoothTrayIcon: %d\n\r"),iIcon));

    BOOL status = FALSE;

    if(hTrayIcon)
    {

        status = Shell_NotifyIcon(NIM_DELETE, &BT_NotifyIcon);


    }

    //RETAILMSG(1, (TEXT("\n\r-RemoveBluetoothTrayIcon: %d\n\r"),status));
    return status;
}
