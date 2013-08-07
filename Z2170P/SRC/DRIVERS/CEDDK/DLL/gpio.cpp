// All rights reserved ADENEO EMBEDDED 2010
#include "bsp.h"
#include <initguid.h>
#include "sdk_gpio.h"
#include "bsp_cfg.h"
#include "ceddkex.h"



void BSPGpioInit()
{
    BSPInsertGpioDevice(0,NULL,L"GIO1:");   // Omap GPIOs
    BSPInsertGpioDevice(TRITON_GPIO_PINID_START,NULL,L"GIO2:");   // GPIO on Triton
}