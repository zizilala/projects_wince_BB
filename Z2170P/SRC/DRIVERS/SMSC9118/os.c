/*****************************************************************************
 * Copyright (c) 2010 Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *****************************************************************************/
/*****************************************************************************
   
	Copyright (c) 2004-2005 SMSC. All rights reserved.

	Use of this source code is subject to the terms of the Software
	License Agreement (SLA) under which you licensed this software product.	 
	If you did not accept the terms of the SLA, you are not authorized to use
	this source code. 

	This code and information is provided as is without warranty of any kind,
	either expressed or implied, including but not limited to the implied
	warranties of merchantability and/or fitness for a particular purpose.
	 
	File name   : os.c 
	Description : os related file

	History	    :
		03-16-05 WH			First Release
		08-12-05 MDG		ver 1.01 
			- add LED1 inversion, add PHY work around
		11-07-05 WH			ver 1.02
			- Fixed middle buffer handling bug
			  (Driver didn't handle middle buffers correctly if it is less than 
               4bytes size)
			- workaround for Multicast bug
			- Workaround for MAC RXEN bug
		11-17-05 WH			ver 1.03
			- 1.02 didn't have 1.01 patches
			- 1.03 is 1.02 + 1.01
		12-06-05 WH			ver 1.04
			- Fixed RX doesn't work on Silicon A1 (REV_ID = 0x011x0002)
			- Support SMSC9118x/117x/116x/115x family
		02-27-05 WH			ver 1.05
			- Fixing External Phy bug that doesn't work with 117x/115x
		03-23-05 WH			ver 1.06
			- Put the variable to avoid PHY_WORKAROUND for External PHY
			- Change product name to 9118x->9218, 9115x->9215
		07-26-06 WH, MDG, NL		ver 1.07
			- Add RXE and TXE interrupt handlers
			- Workaround Code for direct GPIO connection from 9118 family 
			  Interrupt (Level Interrupt -> Edge Interrupt)
			- Change GPT interrupt interval to 200mSec from 50mSec
			- clean up un-used SH3 code
		08-25-06  WH, MDG, NL       ver 1.08
		    - Fixed RXE and TXE interrupt handlers bug
			- support for direct and nondirect Interrupt
		02-15-07   NL               ver 1.09
			- First version of WinCE 6.0 driver
			- Removed Support for LAN9112
			- Added AutoMdix as modifiable parameter in the Registry
		    - Fixed DMA Xmit Bug
		04-17-07   NL               ver 1.10
			- Added Support LAN9211 Chip
			- Changed Register Name ENDIAN to WORD_SWAP According to the Menual
			- Merged CE6.0 & 5.0 Drivers Together
*****************************************************************************/

#pragma warning(disable:4127 4201 4214 4115 4100 4514)
#include "OS.h"

#ifdef SMSC_DBG
DWORD g_DebugMode = 0x0UL;
		//DBG_POWER | DBG_FLOW | DBG_MULTICAST | DBG_DMA | DBG_INIT
		//DBG_ISR | DBG_TX | DBG_RX | DBG_PHY | DBG_SHUT_DOWN
		//DBG_EEPROM | DBG_ENABLE_BREAK | DBG_ERROR | DBG_WARNING
#endif

