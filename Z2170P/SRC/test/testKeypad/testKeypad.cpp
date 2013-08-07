// testKeypad.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"

#include <winuserm.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                      // current instance
TCHAR szTitle[MAX_LOADSTRING];        // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];  // The title bar text

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

static int counters[256];

int WINAPI WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR     lpCmdLine,
                     int       nCmdShow)
{
    // TODO: Place code here.
    MSG msg;
    HACCEL hAccelTable;
    memset(counters,0,sizeof(counters));
    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_testKeypad, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) 
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_testKeypad);

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = (WNDPROC) WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = szWindowClass;

    return RegisterClass(&wc);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
      0, 0, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}
typedef struct {
    UINT vkey;
    TCHAR* name;
}T_VKEY_AND_NAME;

T_VKEY_AND_NAME table[]= {
    {VK_TACTION    ,TEXT("VK_TACTION") },
    {VK_THOME      ,TEXT("VK_THOME") },
    {VK_TDOWN    ,TEXT("VK_TDOWN") },
    {VK_TRIGHT ,TEXT("VK_TRIGHT") },
    {VK_TAB  ,TEXT("VK_TAB") },
    {VK_TSOFT2  ,TEXT("VK_TSOFT2") },
    {VK_SPACE   ,TEXT("VK_SPACE") },
    {VK_TBACK    ,TEXT("VK_TBACK") },
    {VK_TSOFT1   ,TEXT("VK_TSOFT1") },
    {VK_TLEFT  ,TEXT("VK_TLEFT") },
    {VK_TUP   ,TEXT("VK_TUP") },
    {VK_ESCAPE ,TEXT("VK_ESCAPE") },
	{0 ,TEXT("N/A") },
};

TCHAR* GetKeyString(WPARAM wParam)
{
    for (int i=0;i<sizeof(table)/sizeof(table[0]);i++)
    {
        if (wParam == table[i].vkey)
        {
            return table[i].name;
        }
    }
    return TEXT("key");
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
TCHAR labelText[MAX_LOADSTRING]= TEXT("....");

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;  
    
    //LoadString(hInst, IDS_HELLO, szHello, MAX_LOADSTRING);

    switch (message) 
    {
        case WM_KEYUP:
        case WM_KEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
            {
                TCHAR* key = GetKeyString(wParam);
                if ((message == WM_KEYDOWN) || (message == WM_SYSKEYDOWN))
                {
                    counters[wParam]++;
                    _stprintf(labelText,TEXT("%s (0x%x) pressed %d time(s)"),key,wParam,counters[wParam]);
                }
                else
                {
                    counters[wParam]=0;
                    _stprintf(labelText,TEXT("%s (0x%x) released"),key,wParam);
                }

                InvalidateRect(hWnd,NULL,TRUE);
                UpdateWindow(hWnd);
            }
            break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code here...
            RECT rt;
            GetClientRect(hWnd, &rt);
            DrawText(hdc, labelText, _tcslen(labelText), &rt, DT_CENTER);
            EndPaint(hWnd, &ps);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

