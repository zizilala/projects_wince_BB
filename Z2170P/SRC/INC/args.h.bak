// All rights reserved ADENEO EMBEDDED 2010
//
//=============================================================================
//            Texas Instruments OMAP(TM) Platform Software
// (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
//
//  Use of this software is controlled by the terms and conditions found
// in the license agreement under which this software has been supplied.
//
//=============================================================================
//

//------------------------------------------------------------------------------
//
//  File:  args.h
//
//  This header file defines device structures and constant related to boot
//  configuration. BOOT_CFG structure defines layout of persistent device
//  information. It is used to control boot process. BSP_ARGS structure defines
//  information passed from boot loader to kernel HAL/OAL. Each structure has
//  version field which should be updated each time when structure layout
//  change.
//
#ifndef __ARGS_H
#define __ARGS_H

//------------------------------------------------------------------------------

#include <oal_args.h>
#include <oal_kitl.h>
#include <bsp_cfg.h>

//------------------------------------------------------------------------------

#define BSP_ARGS_VERSION    1

typedef struct {
    OAL_ARGS_HEADER header;
    BOOL updateMode;                    // Should IPL run in update mode?
    BOOL coldBoot;                      // Cold boot (erase registry)?
    UINT32 deviceID;                    // Unique ID for development platform
    UINT32 imageLaunch;                 // Image launch address
    OAL_KITL_ARGS kitl;                 // KITL parameters
    UINT32 oalFlags;                    // OAL flags
    UCHAR DevicePrefix[24];
    OMAP_LCD_DVI_RES  dispRes;                     // display resolution
    UINT32 ECCtype;    
    UINT32 opp_mode;
} BSP_ARGS;

//------------------------------------------------------------------------------

#endif
