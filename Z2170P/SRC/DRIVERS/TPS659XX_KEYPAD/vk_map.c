// All rights reserved ADENEO EMBEDDED 2010
/*
================================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
================================================================================
*/
//
//  File: vk_map.c
//
//  This file containts definition for keyboard row/column to virtual key
//  mapping.
//
#include "bsp.h"
#include "tps659xx_keypad.h"
#include <winuserm.h>


#ifndef dimof
#define dimof(x)            (sizeof(x)/sizeof(x[0]))
#endif

#if KEYPAD_ROWS != 8 || KEYPAD_COLUMNS != 8
    #error g_keypadVK dimensions do not match KEYPAD_ROWS and/or KEYPAD_COLUMNS
#endif

//------------------------------------------------------------------------------
const UCHAR g_keypadVK[KEYPAD_ROWS * KEYPAD_COLUMNS] = {
//  Column 0    Column 1        Column 2    Column 3    Column 4    Column 5    Column 6    Column 7
   
    0,			VK_UP,			'Q',		'I',		'G',        'C',		VK_RWIN,	0,		// Row 0

    0,			VK_DOWN,		'W',     	'O',		'H',        'V',		0,			0,		// Row 1

	0,			VK_LEFT,		'E',   		'P',		'J',        'B',		0,          0,		// Row 2

    VK_BACK,	VK_RIGHT,       'R',     	'A',		'K',        'N',		0,          0,		// Row 3

	VK_TAB,		VK_RETURN,		'T',		'S',		'L',		'M',		0,			0,		// Row 4

	VK_TAB,		'd',            'Y',		'D',        'Z',        0,          0,          0,		// Row 5

	VK_SPACE,	'f',            'U',        'F',        'X',        VK_DECIMAL,	0,			0,		// Row 6

	0,			0,				0,			0,			0,			0,			0,			0,		// Row 7
 };

const UCHAR g_powerVK[1] = {
//  Column 0
    VK_OFF // Row 0
 };

//------------------------------------------------------------------------------
const KEYPAD_REMAP g_keypadRemap = { 0, NULL };
const KEYPAD_REMAP g_powerRemap = { 0, 0 };

//------------------------------------------------------------------------------

static const KEYPAD_REPEAT_ITEM repeatItems[] = {
    { VK_TUP,     400, 200, FALSE, NULL },
    { VK_TRIGHT,  400, 200, FALSE, NULL },
    { VK_TLEFT,   400, 200, FALSE, NULL },
    { VK_TDOWN,   400, 200, FALSE, NULL }
};

const KEYPAD_REPEAT g_keypadRepeat = { dimof(repeatItems), repeatItems };
const KEYPAD_REPEAT g_powerRepeat = { 0, 0 };

//------------------------------------------------------------------------------

const KEYPAD_EXTRASEQ g_keypadExtraSeq = { 0, NULL };

//------------------------------------------------------------------------------
