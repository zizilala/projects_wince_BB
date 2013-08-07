//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
//------------------------------------------------------------------------------
//
//  File:  menu.c
//
#include <windows.h>
#include <ceddk.h>
#include <nkintr.h>
#include <oal.h>
#include <oal_blmenu.h>

//------------------------------------------------------------------------------

#define OAL_MENU_MAX_DEVICES    8
#define dimof(x)                (sizeof(x)/sizeof(x[0]))

//------------------------------------------------------------------------------

static LPCSTR DeviceId(LPCSTR deviceId);

//------------------------------------------------------------------------------

VOID OALBLMenuActivate(UINT32 delay, OAL_BLMENU_ITEM *pMenu)
{
    UINT32 time;
    WCHAR key = 0;
    
    // First let user break to menu
    while (delay > 0 && key != L' ') {
       OALLog(L"Hit space to enter configuration menu %d...\r\n", delay);
       time = OALGetTickCount();
       while ((OALGetTickCount() - time) < 1000) {
          if ((key = OALBLMenuReadKey(FALSE)) == L' ') break;
       }
       delay--;
    }
    
    if (key == L' ') {
#ifdef OAL_BLMENU_PCI
        OALPCIConfig(0, 0, 0, 0, 0, 0, NULL);
#endif
        OALBLMenuShow(pMenu);
    }        
}

//------------------------------------------------------------------------------

VOID OALBLMenuHeader(LPCWSTR format, ...)
{
    va_list pArgList;
    UINT32 i;

    va_start(pArgList, format);

    OALLog(L"\r\n");
    for (i = 0; i < 80; i++) OALLog(L"-");
    OALLog(L"\r\n ");
    OALLogV(format, pArgList);
    OALLog(L"\r\n");
    for (i = 0; i < 80; i++) OALLog(L"-");
    OALLog(L"\r\n");
}

//------------------------------------------------------------------------------

VOID OALBLMenuShow(OAL_BLMENU_ITEM *pMenu)
{
    LPCWSTR title = pMenu->pParam1;
    OAL_BLMENU_ITEM *aMenu = pMenu->pParam2, *pItem;
    WCHAR key;

    while (TRUE) {
        
        OALBLMenuHeader(L"%s", title);

        // Print menu items
        for (pItem = aMenu; pItem->key != 0; pItem++) {
            OALLog(L" [%c] %s\r\n", pItem->key, pItem->text);
        }
        OALLog(L"\r\n Selection: ");

        while (TRUE) {
            // Get key
            key = OALBLMenuReadKey(TRUE);
            // Look for key in menu
            for (pItem = aMenu; pItem->key != 0; pItem++) {
                if (pItem->key == key) break;
            }
            // If we find it, break loop
            if (pItem->key != 0) break;
        }

        // Print out selection character
        OALLog(L"%c\r\n", key);
        
        // When action is NULL return back to parent menu
        if (pItem->pfnAction == NULL) break;
        
        // Else call menu action
        pItem->pfnAction(pItem);

    } 

}

//------------------------------------------------------------------------------

VOID OALBLMenuEnable(OAL_BLMENU_ITEM *pMenu)
{
    LPCWSTR title = pMenu->pParam1;
    UINT32 *pFlags = pMenu->pParam2;
    UINT32 mask = (UINT32)pMenu->pParam3;
    BOOL flag;
    WCHAR key;

    // First check input parameter   
    if (title == NULL || pFlags == NULL) {
        OALMSG(OAL_ERROR, (L"ERROR: OALBLMenuEnable: Invalid parameter\r\n"));
        goto cleanUp;
    }
    if (mask == 0) mask = 0xFFFF;

    flag = (*pFlags & mask) != 0;
    
    OALLog(
        L" %s %s (actually %s) [y/-]: ", flag ? L"Disable" : L"Enable",
        title, flag ? L"enabled" : L"disabled"
    );

    // Get key
    key = OALBLMenuReadKey(TRUE);
    OALLog(L"%c\r\n", key);

    if (key == L'y' || key == L'Y') {
        flag = !flag;
        OALLog(L" %s %s\r\n", title, flag ? L"enabled" : L"disabled");
    } else {
        OALLog(
            L" %s stays %s\r\n", title, flag ? L"enabled" : L"disabled"
        );
    }

    // Save value
    if (flag) {
        *pFlags |= mask;
    } else {
        *pFlags &= ~mask;
    }

cleanUp:;
}

//------------------------------------------------------------------------------

VOID OALBLMenuSetMacAddress(OAL_BLMENU_ITEM *pMenu)
{
    LPCWSTR title = pMenu->pParam1;
    UINT16 *pMac = pMenu->pParam2;
    UINT16 mac[3];
    WCHAR buffer[18];

    // First check input parameters    
    if (title == NULL || pMac == NULL) {
        OALMSG(OAL_ERROR, (
            L"ERROR: OALBLMenuSetMacAddress: Invalid parameter\r\n"
        ));
        goto cleanUp;
    }

    // Get actual setting
    mac[0] = pMac[0];
    mac[1] = pMac[1];
    mac[2] = pMac[2];

    // Print prompt
    OALLog(
        L" Enter %s MAC address (actual %s): ", title, OALKitlMACtoString(mac)
    );
    
    // Read input line
    if (OALBLMenuReadLine(buffer, dimof(buffer)) == 0) goto cleanUp;

    // Convert string to MAC address
    if (!OALKitlStringToMAC(buffer, mac)) {
        OALLog(L" '%s' isn't valid MAC address\r\n", buffer);
        goto cleanUp;
    }

    // Print final MAC address
    OALLog(L" %s MAC address set to %s\r\n", title, OALKitlMACtoString(mac));        

    // Save new setting
    pMac[0] = mac[0];
    pMac[1] = mac[1];
    pMac[2] = mac[2];
    
cleanUp:;
}

//------------------------------------------------------------------------------

VOID OALBLMenuSetIpAddress(OAL_BLMENU_ITEM *pMenu)
{
    LPCWSTR title = pMenu->pParam1;
    UINT32 *pIp = pMenu->pParam2;
    UINT32 ip;
    WCHAR buffer[16];

    // First check input parameters    
    if (title == NULL || pIp == NULL) {
        OALMSG(OAL_ERROR, (
            L"ERROR: OALBLMenuSetIpAddress: Invalid parameters\r\n"
        ));
        goto cleanUp;
    }

    // Get actual value
    ip = *pIp;

    // Print prompt
    OALLog(L" Enter %s IP address (actual %s): ", title, OALKitlIPtoString(ip));

    // Read input line
    if (OALBLMenuReadLine(buffer, dimof(buffer)) == 0) goto cleanUp;

    // Convert string to IP address
    ip = OALKitlStringToIP(buffer);
    if (ip == 0) {
        OALLog(L" '%s' isn't valid IP address\r\n", buffer);
        goto cleanUp;
    }

    // Print final IP address
    OALLog(L" %s IP address set to %s\r\n", title, OALKitlIPtoString(ip));

    // Save new setting
    *pIp = ip;
    
cleanUp:;
}

//------------------------------------------------------------------------------

VOID OALBLMenuSetIpMask(OAL_BLMENU_ITEM *pMenu)
{
    LPCWSTR title = pMenu->pParam1;
    UINT32 *pIp = pMenu->pParam2;
    UINT32 ip;
    WCHAR buffer[16];

    // First check input parameters    
    if (title == NULL || pIp == NULL) {
        OALMSG(OAL_ERROR, (
            L"ERROR: OALBLMenuSetIpMask: Invalid parameters\r\n"
        ));
        goto cleanUp;
    }

    // Get actual value
    ip = *pIp;

    // Print prompt
    OALLog(L" Enter %s IP mask (actual %s): ", title, OALKitlIPtoString(ip));

    // Read input line
    if (OALBLMenuReadLine(buffer, dimof(buffer)) == 0) goto cleanUp;

    // Convert string to MAC address
    ip = OALKitlStringToIP(buffer);
    if (ip == 0) {
        OALLog(L" '%s' isn't valid IP mask\r\n", buffer);
        goto cleanUp;
    }

    // Print final IP mask
    OALLog(L" %s IP mask set to %s\r\n", title, OALKitlIPtoString(ip));

    // Save new setting
    *pIp = ip;
    
cleanUp:;
}

//------------------------------------------------------------------------------

VOID OALBLMenuSetDeviceId(OAL_BLMENU_ITEM * pMenu)
{
    WCHAR deviceId[OAL_KITL_ID_SIZE];
    LPSTR pDeviceId = pMenu->pParam1;
    UINT i;

    memset(deviceId, 0, sizeof(deviceId));
    
    OALLog(L" Enter Device Id (* = auto, actual '%S'): ", DeviceId(pDeviceId));
    
    // Read input line
    if (OALBLMenuReadLine(deviceId, dimof(deviceId)) == 0) goto cleanUp;

    // Convert unicode to one-byte character string
    if (deviceId[0] == L'*' && deviceId[1] == L'\0') {
        memset(pDeviceId, 0, OAL_KITL_ID_SIZE);
    } else {
        for (i = 0; i < OAL_KITL_ID_SIZE; i++) {
            pDeviceId[i] = (UCHAR)deviceId[i];
        }
    }        

    OALLog(L" Device Id set to '%S'\r\n", DeviceId(pDeviceId));       
    
cleanUp: ;
}

//------------------------------------------------------------------------------

VOID OALBLMenuSelectDevice(OAL_BLMENU_ITEM *pMenu)
{
    LPCWSTR title = pMenu->pParam1;
    DEVICE_LOCATION *pDevLoc = pMenu->pParam2;
    OAL_KITL_DEVICE *aDevices = pMenu->pParam3, *pDevice;
    DEVICE_LOCATION devLoc[OAL_MENU_MAX_DEVICES];
    UINT32 i;
    WCHAR key;

    // First check input parameters    
    if (title == NULL || pDevLoc == NULL || aDevices == NULL) {
        OALMSG(OAL_ERROR, (
            L"ERROR: OALBLMenuSelectDevice: Invalid parameters\r\n"
        ));
        goto cleanUp;
    }

    OALBLMenuHeader(L"Select %s Device", title);

    i = 0;
    pDevice = aDevices;
    while (pDevice->name != NULL && i < OAL_MENU_MAX_DEVICES) {
        switch (pDevice->ifcType) {
        case Internal:
            devLoc[i].IfcType = pDevice->ifcType;
            devLoc[i].BusNumber = 0;
            devLoc[i].LogicalLoc = pDevice->id;
            devLoc[i].PhysicalLoc = 0;
            OALLog(
                L" [%d] %s\r\n", i + 1, OALKitlDeviceName(&devLoc[i], aDevices)
            );
            i++;
            pDevice++;
            break;
#ifdef OAL_BLMENU_PCI
        case PCIBus:
            {
                OAL_PCI_LOCATION pciLoc;

                pciLoc.logicalLoc = 0;
                while (i < OAL_MENU_MAX_DEVICES) {
                    if (!OALPCIFindNextId(0, pDevice->id, &pciLoc)) break;
                    devLoc[i].IfcType = pDevice->ifcType;
                    devLoc[i].BusNumber = 0;
                    devLoc[i].LogicalLoc = pciLoc.logicalLoc;
                    devLoc[i].PhysicalLoc = 0;
                    OALLog(
                        L" [%d] %s\r\n", i + 1, 
                        OALKitlDeviceName(&devLoc[i], aDevices)
                    );
                    i++;
                }
                pDevice++;
            }
            break;
#endif
        default:
            pDevice++;
            break;
        }        
    }    
    OALLog(L" [0] Exit and Continue\r\n");

    OALLog(
        L"\r\n Selection (actual %s): ", OALKitlDeviceName(pDevLoc, aDevices)
    );

    do {
        key = OALBLMenuReadKey(TRUE);
    } while (key < L'0' || key > L'0' + i);
    OALLog(L"%c\r\n", key);

    // If user select exit don't change device
    if (key == L'0') goto cleanUp;
    
    memcpy(pDevLoc, &devLoc[key - L'0' - 1], sizeof(DEVICE_LOCATION));
    OALLog(
        L" %s device set to %s\r\n", title, OALKitlDeviceName(pDevLoc, aDevices)
    );

cleanUp:;    
}

//------------------------------------------------------------------------------

UINT32 OALBLMenuReadLine(LPWSTR szBuffer, size_t CharCount)
{
    UINT32 count;
    WCHAR key;
    
    count = 0;
    while (count < CharCount) {
        key = OALBLMenuReadKey(TRUE);
       if (key == L'\r' || key == L'\n') {
          OALLog(L"\r\n");
          break;
       } if (key == L'\b' && count > 0) {
          OALLog(L"\b \b");
          count--;
       } else if (key >= L' ' && key < 128 && count < (CharCount - 1)) {
          szBuffer[count++] = key;
          OALLog(L"%c", key);
       } 
    }
    szBuffer[count] = '\0';
    return count;
    
}

//------------------------------------------------------------------------------

WCHAR OALBLMenuReadKey(BOOL wait)
{
    CHAR key;

    while ((key = OEMReadDebugByte()) == OEM_DEBUG_READ_NODATA && wait);
    if (key == OEM_DEBUG_READ_NODATA) key = 0;
    return (WCHAR)key;
}

//------------------------------------------------------------------------------

static LPCSTR DeviceId(LPCSTR deviceId)
{
    return deviceId[0] == '\0' ? "*** auto ***" : deviceId;
}

//------------------------------------------------------------------------------
