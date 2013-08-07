/*
==============================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
==============================================================
*/
#ifndef __BSPCAMERAPROP_H__
#define __BSPCAMERAPROP_H__


// The IAMCameraControl::Set() & IAMCameraControl::Get() methods have been 
// overloaded to support setting and retrieveing custom properties of the 
// OMAP3530/DM3730 camera subsytem and the TVP5146 decoder chip
// NOTE: The 3rd argument to these methods is ignored when setting
// one of the custome properties.

// CameraControlProperty_t
// This enum defines values that can be passed as the first arg (Property) to 
// IAMCameraControl::Set() and IAMCameraControl::Get()
typedef enum {
    CameraControl_InputPort = 0x100,  // Select input port -  use one of the
                                          // values specified by the enum
                                          // type CameraInput
} CameraControlProperty_t;

// CameraInput_t
// This enum defines values that can be passed as the second arg (lValue) to 
// IAMCameraControl::Set() and IAMCameraControl::Get()
typedef enum {
    CameraInput_YPbPr = 0,
    CameraInput_AV,
    CameraInput_SVideo
} CameraInputPort_t;


#endif /* __BSPCAMERAPROP_H__ */

