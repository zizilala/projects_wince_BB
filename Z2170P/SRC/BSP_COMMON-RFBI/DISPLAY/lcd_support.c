// All rights reserved ADENEO EMBEDDED 2010
// Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

#include "bsp.h"
#include "sdk_gpio.h"

#include <..\lcd_cfg.h>

static HANDLE g_hGpio = NULL;
static DWORD g_gpioLcdPower;
static DWORD g_gpioLcdIni;
static DWORD g_gpioLcdResB;


BOOL LcdInitGpio(void)
{
    // Configure Backlight/Power pins as outputs
    g_hGpio = GPIOOpen();
/*
    GPIOSetBit(g_hGpio,BSP_LCD_POWER_GPIO);
	GPIOSetBit(g_hGpio,BSP_LCD_UP_DOWN_GPIO);
    GPIOSetBit(g_hGpio,BSP_LCD_LEFT_RIGHT_GPIO);  
	GPIOClrBit(g_hGpio,BSP_LCD_RESB_GPIO);
    GPIOClrBit(g_hGpio,BSP_LCD_INI_GPIO);
    GPIOClrBit(g_hGpio,BSP_LCD_DVIENABLE_GPIO);
    GPIOClrBit(g_hGpio,BSP_LCD_LCD_QVGA_nVGA);


	GPIOSetMode(g_hGpio,BSP_LCD_POWER_GPIO,GPIO_DIR_OUTPUT);    
    GPIOSetMode(g_hGpio,BSP_LCD_UP_DOWN_GPIO,GPIO_DIR_OUTPUT);    
    GPIOSetMode(g_hGpio, BSP_LCD_LEFT_RIGHT_GPIO,GPIO_DIR_OUTPUT);
	GPIOSetMode(g_hGpio,BSP_LCD_LCD_QVGA_nVGA,GPIO_DIR_OUTPUT);
    GPIOSetMode(g_hGpio,BSP_LCD_INI_GPIO,GPIO_DIR_OUTPUT);
    GPIOSetMode(g_hGpio,BSP_LCD_DVIENABLE_GPIO,GPIO_DIR_OUTPUT);
    GPIOSetMode(g_hGpio,BSP_LCD_RESB_GPIO,GPIO_DIR_OUTPUT);
*/
	return TRUE;
}

BOOL LcdDeinitGpio(void)
{
	// Close GPIO driver
    GPIOClose(g_hGpio);

    return TRUE;
}

void LcdPowerControl(BOOL bEnable)
{    
    if (bEnable)
    {
        GPIOClrBit(g_hGpio,BSP_LCD_POWER_GPIO);
    }
    else
    {
        GPIOSetBit(g_hGpio,BSP_LCD_POWER_GPIO);
    }
}

void LcdResBControl(BOOL bEnable)
{
    if (!bEnable)
    {
        GPIOClrBit(g_hGpio,BSP_LCD_RESB_GPIO);
    }
    else
    {
        GPIOSetBit(g_hGpio,BSP_LCD_RESB_GPIO);
    }
}

void LcdIniControl(BOOL bEnable)
{
    if (!bEnable)
    {
        GPIOClrBit(g_hGpio,BSP_LCD_INI_GPIO);
    }
    else
    {
        GPIOSetBit(g_hGpio,BSP_LCD_INI_GPIO);
    }
}

// Screen should be in rotated to lanscape mode, 640x480 for use with DVI
void LcdDviEnableControl(BOOL bEnable)
{
    if (!bEnable)
    {
        GPIOClrBit(g_hGpio,BSP_LCD_DVIENABLE_GPIO);
    }
    else
    {
        GPIOSetBit(g_hGpio,BSP_LCD_DVIENABLE_GPIO);
    }
}

/*
void LcdStall(DWORD dwMicroseconds)
{
    OALStall(dwMicroseconds);
}

void LcdSleep(DWORD dwMilliseconds)
{
    OALStall(1000 * dwMilliseconds);
}
*/