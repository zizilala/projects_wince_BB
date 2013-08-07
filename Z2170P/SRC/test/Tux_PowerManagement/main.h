//
//  All rights reserved ADENEO EMBEDDED 2010
//
////////////////////////////////////////////////////////////////////////////////
//
//  Tux_PowerManagement TUX DLL
//
//  Module: main.h
//          Header for all files in the project.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MAIN_H__
#define __MAIN_H__

////////////////////////////////////////////////////////////////////////////////
// Included files

#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <tux.h>
#include <kato.h>

////////////////////////////////////////////////////////////////////////////////
// Suggested log verbosities

#define LOG_EXCEPTION          0
#define LOG_FAIL               2
#define LOG_ABORT              4
#define LOG_SKIP               6
#define LOG_NOT_IMPLEMENTED    8
#define LOG_PASS              10
#define LOG_DETAIL            12
#define LOG_COMMENT           14
#define LOG_WARNING			  16

////////////////////////////////////////////////////////////////////////////////


#define __THIS_FILE__   TEXT("Tux_PowerManagementTest.cpp")

#define DEFAULT_ERROR( TST, ACT );  \
    if( TST ) { \
        g_pKato->Log( LOG_FAIL, TEXT("FAIL in %s @ line %d: CONDITION( %s ) ACTION( %s )" ), \
                      __THIS_FILE__, __LINE__, TEXT( #TST ), TEXT( #ACT ) ); \
        ACT; \
    }

static BOOL SetTestOptions( LPTSTR lpszCmdLine );

/* ------------------------------------------------------------------------
	Global values
------------------------------------------------------------------------ */
extern BOOL        g_fStress;
extern DWORD       g_StressCount;
extern DWORD	   g_dwMaxTimeout;
extern DWORD	   g_NbLoop;
extern DWORD	   g_TimeToSleep;
extern DWORD	   g_NbBytes;

/* ------------------------------------------------------------------------
	Some constances
------------------------------------------------------------------------ */
static const  TCHAR CmdFlag			= (TCHAR)'-';
static const  TCHAR CmdItr          = (TCHAR)'i';
static const  TCHAR CmdLoop			= (TCHAR)'l';
static const  TCHAR CmdSleep		= (TCHAR)'s';
static const  TCHAR CmdBytes		= (TCHAR)'b';
static const  TCHAR CmdSpace        = (TCHAR)' ';

#endif // __MAIN_H__
