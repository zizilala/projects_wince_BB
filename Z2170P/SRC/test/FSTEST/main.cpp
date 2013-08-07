//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
////////////////////////////////////////////////////////////////////////////////
//
//  CETK_FSTest TUX DLL
//
//  Module: main.cpp
//          Contains the shell processing function.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "globals.h"


static bool ParseCommandLine(LPTSTR szCmdLine);
static void LogHelpInfo(void);



////////////////////////////////////////////////////////////////////////////////
// DllMain
//  Main entry point of the DLL. Called when the DLL is loaded or unloaded.
//
// Parameters:
//  hInstance       Module instance of the DLL.
//  dwReason        Reason for the function call.
//  lpReserved      Reserved for future use.
//
// Return value:
//  TRUE if successful, FALSE to indicate an error condition.

BOOL WINAPI DllMain(HANDLE hInstance, ULONG dwReason, LPVOID lpReserved)
{
    // Any initialization code goes here.
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(dwReason);
	UNREFERENCED_PARAMETER(lpReserved);

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////
// Debug
//  Printf-like wrapping around OutputDebugString.
//
// Parameters:
//  szFormat        Formatting string (as in printf).
//  ...             Additional arguments.
//
// Return value:
//  None.

void Debug(LPCTSTR szFormat, ...)
{
    static  TCHAR   szHeader[] = TEXT("CETK_FSTest: ");
    TCHAR   szBuffer[1024];

    va_list pArgs;
    va_start(pArgs, szFormat);
    lstrcpy(szBuffer, szHeader);
    wvsprintf(
        szBuffer + countof(szHeader) - 1,
        szFormat,
        pArgs);
    va_end(pArgs);

    _tcscat(szBuffer, TEXT("\r\n"));

    OutputDebugString(szBuffer);
}

////////////////////////////////////////////////////////////////////////////////
// ShellProc
//  Processes messages from the TUX shell.
//
// Parameters:
//  uMsg            Message code.
//  spParam         Additional message-dependent data.
//
// Return value:
//  Depends on the message.

SHELLPROCAPI ShellProc(UINT uMsg, SPPARAM spParam)
{
    LPSPS_BEGIN_TEST    pBT;
    LPSPS_END_TEST      pET;

    switch (uMsg)
    {
    case SPM_LOAD_DLL:
        // Sent once to the DLL immediately after it is loaded. The spParam
        // parameter will contain a pointer to a SPS_LOAD_DLL structure. The
        // DLL should set the fUnicode member of this structure to TRUE if the
        // DLL is built with the UNICODE flag set. If you set this flag, Tux
        // will ensure that all strings passed to your DLL will be in UNICODE
        // format, and all strings within your function table will be processed
        // by Tux as UNICODE. The DLL may return SPR_FAIL to prevent the DLL
        // from continuing to load.
        Debug(TEXT("ShellProc(SPM_LOAD_DLL, ...) called"));

        // If we are UNICODE, then tell Tux this by setting the following flag.
#ifdef UNICODE
        ((LPSPS_LOAD_DLL)spParam)->fUnicode = TRUE;
#endif // UNICODE
        g_pKato = (CKato*)KatoGetDefaultObject();
        break;

    case SPM_UNLOAD_DLL:
        // Sent once to the DLL immediately before it is unloaded.
        Debug(TEXT("ShellProc(SPM_UNLOAD_DLL, ...) called"));
        break;

    case SPM_SHELL_INFO:
        // Sent once to the DLL immediately after SPM_LOAD_DLL to give the DLL
        // some useful information about its parent shell and environment. The
        // spParam parameter will contain a pointer to a SPS_SHELL_INFO
        // structure. The pointer to the structure may be stored for later use
        // as it will remain valid for the life of this Tux Dll. The DLL may
        // return SPR_FAIL to prevent the DLL from continuing to load.
        Debug(TEXT("ShellProc(SPM_SHELL_INFO, ...) called"));

        // Store a pointer to our shell info for later use.
        g_pShellInfo = (LPSPS_SHELL_INFO)spParam;
        
        // Parse the command line parameters
        // This can be done here or in each test using
        //  the global g_pShellInfo pointer
        if ( !ParseCommandLine((LPTSTR)g_pShellInfo->szDllCmdLine) )
        {
            return SPR_FAIL;
        }
        break;

    case SPM_REGISTER:
        // This is the only ShellProc() message that a DLL is required to
        // handle (except for SPM_LOAD_DLL if you are UNICODE). This message is
        // sent once to the DLL immediately after the SPM_SHELL_INFO message to
        // query the DLL for its function table. The spParam will contain a
        // pointer to a SPS_REGISTER structure. The DLL should store its
        // function table in the lpFunctionTable member of the SPS_REGISTER
        // structure. The DLL may return SPR_FAIL to prevent the DLL from
        // continuing to load.
        Debug(TEXT("ShellProc(SPM_REGISTER, ...) called"));
        ((LPSPS_REGISTER)spParam)->lpFunctionTable = g_lpFTE;
#ifdef UNICODE
        return SPR_HANDLED | SPF_UNICODE;
#else // UNICODE
        return SPR_HANDLED;
#endif // UNICODE
        break;

    case SPM_START_SCRIPT:
        // Sent to the DLL immediately before a script is started. It is sent
        // to all Tux DLLs, including loaded Tux DLLs that are not in the
        // script. All DLLs will receive this message before the first
        // TestProc() in the script is called.
        Debug(TEXT("ShellProc(SPM_START_SCRIPT, ...) called"));
        break;

    case SPM_STOP_SCRIPT:
        // Sent to the DLL when the script has stopped. This message is sent
        // when the script reaches its end, or because the user pressed
        // stopped prior to the end of the script. This message is sent to
        // all Tux DLLs, including loaded Tux DLLs that are not in the script.
        Debug(TEXT("ShellProc(SPM_STOP_SCRIPT, ...) called"));
        break;

    case SPM_BEGIN_GROUP:
        // Sent to the DLL before a group of tests from that DLL is about to
        // be executed. This gives the DLL a time to initialize or allocate
        // data for the tests to follow. Only the DLL that is next to run
        // receives this message. The prior DLL, if any, will first receive
        // a SPM_END_GROUP message. For global initialization and
        // de-initialization, the DLL should probably use SPM_START_SCRIPT
        // and SPM_STOP_SCRIPT, or even SPM_LOAD_DLL and SPM_UNLOAD_DLL.
        Debug(TEXT("ShellProc(SPM_BEGIN_GROUP, ...) called"));
        g_pKato->BeginLevel(0, TEXT("BEGIN GROUP: CETK_FSTest.DLL"));
        break;

    case SPM_END_GROUP:
        // Sent to the DLL after a group of tests from that DLL has completed
        // running. This gives the DLL a time to cleanup after it has been
        // run. This message does not mean that the DLL will not be called
        // again; it just means that the next test to run belongs to a
        // different DLL. SPM_BEGIN_GROUP and SPM_END_GROUP allow the DLL
        // to track when it is active and when it is not active.
        Debug(TEXT("ShellProc(SPM_END_GROUP, ...) called"));
        g_pKato->EndLevel(TEXT("END GROUP: CETK_FSTest.DLL"));
        break;

    case SPM_BEGIN_TEST:
        // Sent to the DLL immediately before a test executes. This gives
        // the DLL a chance to perform any common action that occurs at the
        // beginning of each test, such as entering a new logging level.
        // The spParam parameter will contain a pointer to a SPS_BEGIN_TEST
        // structure, which contains the function table entry and some other
        // useful information for the next test to execute. If the ShellProc
        // function returns SPR_SKIP, then the test case will not execute.
        Debug(TEXT("ShellProc(SPM_BEGIN_TEST, ...) called"));
        // Start our logging level.
        pBT = (LPSPS_BEGIN_TEST)spParam;
        g_pKato->BeginLevel(
            pBT->lpFTE->dwUniqueID,
            TEXT("BEGIN TEST: \"%s\", Threads=%u, Seed=%u"),
            pBT->lpFTE->lpDescription,
            pBT->dwThreadCount,
            pBT->dwRandomSeed);
        break;

    case SPM_END_TEST:
        // Sent to the DLL after a single test executes from the DLL.
        // This gives the DLL a time perform any common action that occurs at
        // the completion of each test, such as exiting the current logging
        // level. The spParam parameter will contain a pointer to a
        // SPS_END_TEST structure, which contains the function table entry and
        // some other useful information for the test that just completed. If
        // the ShellProc returned SPR_SKIP for a given test case, then the
        // ShellProc() will not receive a SPM_END_TEST for that given test case.
        Debug(TEXT("ShellProc(SPM_END_TEST, ...) called"));
        // End our logging level.
        pET = (LPSPS_END_TEST)spParam;
        g_pKato->EndLevel(
            TEXT("END TEST: \"%s\", %s, Time=%u.%03u"),
            pET->lpFTE->lpDescription,
            pET->dwResult == TPR_SKIP ? TEXT("SKIPPED") :
            pET->dwResult == TPR_PASS ? TEXT("PASSED") :
            pET->dwResult == TPR_FAIL ? TEXT("FAILED") : TEXT("ABORTED"),
            pET->dwExecutionTime / 1000, pET->dwExecutionTime % 1000);
        break;

    case SPM_EXCEPTION:
        // Sent to the DLL whenever code execution in the DLL causes and
        // exception fault. TUX traps all exceptions that occur while
        // executing code inside a test DLL.
        Debug(TEXT("ShellProc(SPM_EXCEPTION, ...) called"));
        g_pKato->Log(LOG_EXCEPTION, TEXT("Exception occurred!"));
        break;

    default:
        // Any messages that we haven't processed must, by default, cause us
        // to return SPR_NOT_HANDLED. This preserves compatibility with future
        // versions of the TUX shell protocol, even if new messages are added.
        return SPR_NOT_HANDLED;
    }

    return SPR_HANDLED;
}

////////////////////////////////////////////////////////////////////////////////
static bool ParseCommandLine(LPTSTR szCmdLine) 
{
    PTSTR pCmdLineOpt;
    TCHAR ws[] = TEXT(" \t");
    bool success = TRUE;
    bool showHelp = FALSE;
    wchar_t *cmdLineContext=NULL;
    bool bGetFileName;
    DWORD dwMode=MODE_NONE;
    DWORD minSize;
    DWORD maxSize;
    
    Debug(TEXT("ParseCommandLine: %s \r\n"),szCmdLine);
    // Get first parameter
    pCmdLineOpt = wcstok_s(szCmdLine, ws, &cmdLineContext);
    while ( pCmdLineOpt != NULL )
    {
        bGetFileName = FALSE;
        
        // Check multi-letter options first
        if (wcsicmp(pCmdLineOpt, TEXT("-nocache")) == 0)
        {
            g_bNoCaching = TRUE;
        }
        else if (wcsicmp(pCmdLineOpt, TEXT("-writethru")) == 0)
        {
            g_bWriteThrough = TRUE;
        }
        else if (wcsicmp(pCmdLineOpt, TEXT("-flushall")) == 0)
        {
            g_bFlushBuffersOnWrite = TRUE;
        }
        else if (wcsicmp(pCmdLineOpt, TEXT("-flushend")) == 0)
        {
            g_bFlushBuffersOnClose = TRUE;
        }
        else if (wcsicmp(pCmdLineOpt, TEXT("-wr")) == 0)
        {
            dwMode = MODE_WRITEREAD;
            bGetFileName = TRUE;
        }
        
        // Now check single letter options
        else if (wcsicmp(pCmdLineOpt, TEXT("-c")) == 0)
        {
            dwMode = MODE_VERIFY;
            bGetFileName = TRUE;
        }
        else if (wcsicmp(pCmdLineOpt, TEXT("-r")) == 0)
        {
            dwMode = MODE_READ;
            bGetFileName = TRUE;
        }
        else if (wcsicmp(pCmdLineOpt, TEXT("-w")) == 0)
        {
            dwMode = MODE_WRITE;
            bGetFileName = TRUE;
        }
        else if (wcsicmp(pCmdLineOpt, TEXT("-s")) == 0)
        {
            pCmdLineOpt = wcstok_s(NULL, ws, &cmdLineContext);
            if ( pCmdLineOpt == NULL )
            {
                g_pKato->Log(LOG_FAIL,
                             TEXT("ERROR: File size not specified\r\n"));
                success = FALSE;
            }
            else
            {
                minSize = FILE_SIZE_MIN;
                maxSize = FILE_SIZE_MAX;
                g_dwTestFileSize = _ttoi(pCmdLineOpt);
                if (g_dwTestFileSize < minSize || g_dwTestFileSize > maxSize)
                {
                    g_pKato->Log(LOG_FAIL,
                                 TEXT("ERROR: File size outside acceptable range (%d - %d)\r\n"),
                                 minSize, maxSize);
                    success = FALSE;
                }
                else 
                {
                    g_dwTestFileSize *= (1024 * 1024);
                }
            }

        }
        else if (wcsicmp(pCmdLineOpt, TEXT("-n")) == 0)
        {
            pCmdLineOpt = wcstok_s(NULL, ws, &cmdLineContext);
            if ( pCmdLineOpt == NULL )
            {
                g_pKato->Log(LOG_FAIL,
                             TEXT("ERROR: Number of tests not specified\r\n"));
                success = FALSE;
            }
            else
            {
                minSize = NUM_TESTS_MIN;
                maxSize = NUM_TESTS_MAX;
                g_dwNumTests = _ttoi(pCmdLineOpt);
                if (g_dwNumTests < minSize || g_dwNumTests > maxSize)
                {
                    g_pKato->Log(LOG_FAIL,
                                 TEXT("ERROR: Number of tests outside acceptable range (%d - %d)\r\n"),
                                 minSize, maxSize);
                    success = FALSE;
                }
            }
        }
        else if (wcsicmp(pCmdLineOpt, TEXT("-b")) == 0)
        {
            pCmdLineOpt = wcstok_s(NULL, ws, &cmdLineContext);
            if ( pCmdLineOpt == NULL )
            {
                g_pKato->Log(LOG_FAIL,
                             TEXT("ERROR: Blocksize not specified\r\n"));
                success = FALSE;
            }
            else
            {
                minSize = BLOCK_SIZE_MIN;
                maxSize = BLOCK_SIZE_MAX;
                g_dwBlockSize = _ttoi(pCmdLineOpt);
                if (g_dwBlockSize < minSize || g_dwBlockSize > maxSize)
                {
                    g_pKato->Log(LOG_FAIL,
                                 TEXT("ERROR: Blocksize outside acceptable range (%d - %d)\r\n"),
                                 minSize, maxSize);
                    success = FALSE;
                }
            }
        }
        else if (wcsicmp(pCmdLineOpt, TEXT("-a")) == 0)
        {
            g_bDoAllTests = TRUE;
        }
        else if (wcsicmp(pCmdLineOpt, TEXT("-v")) == 0)
        {
            g_bVerboseOutput = TRUE;
        }
        else if (wcsicmp(pCmdLineOpt, TEXT("-h")) == 0)
        {
            showHelp = TRUE;
        }
        else
        {
            g_pKato->Log(LOG_FAIL,
                         TEXT("ERROR: Unrecognised option: %s"),
                         pCmdLineOpt);
            success = FALSE;
        }

        if ( bGetFileName )
        {
            if ( g_dwMode != MODE_NONE )
            {
                g_pKato->Log(LOG_FAIL,
                         TEXT("ERROR: Illegal combination of -r, -w, -wr or -c options"));
                success = FALSE;
            }
            else
            {
                g_dwMode = dwMode;

                if ( wcslen(cmdLineContext) < 2 )
                {
                    g_pKato->Log(LOG_FAIL,
                                 TEXT("ERROR: File info not specified\r\n"));
                    success = FALSE;
                }
                else if ( (cmdLineContext[0] == '\'') &&
                          (cmdLineContext[1] == '\'') )
                {
                    pCmdLineOpt = wcstok_s(NULL, ws, &cmdLineContext);
                    if ( pCmdLineOpt == NULL )
                    {
                        g_pKato->Log(LOG_FAIL,
                                     TEXT("ERROR: File info not specified\r\n"));
                        success = FALSE;

                    }
                    else if ( wcsicmp(pCmdLineOpt, TEXT("''")) != 0 )
                    {
                        g_pKato->Log(LOG_FAIL,
                                     TEXT("ERROR: Invalid root dir info specified\r\n"));
                        success = FALSE;
                    }
                    else
                    {
                        // Root dir supplied (special case)
                        if ( g_dwMode != MODE_VERIFY )
                        {
                            g_pKato->Log(LOG_FAIL,
                                 TEXT("ERROR: ERROR: Illegal file info specified"));
                            success = FALSE;
                        }
                        else
                        {
                            g_pFileName = TEXT("");
                        }
                    }
                }
                else
                {
                    pCmdLineOpt = wcstok_s(NULL, TEXT("'"), &cmdLineContext);
                    if ( pCmdLineOpt == NULL )
                    {
                        g_pKato->Log(LOG_FAIL,
                                     TEXT("ERROR: File info not specified\r\n"));
                        success = FALSE;
                    }
                    else if ( wcsicmp(pCmdLineOpt, TEXT("\\")) == 0 )
                    {
                        if ( g_dwMode != MODE_VERIFY )
                        {
                            g_pKato->Log(LOG_FAIL,
                                 TEXT("ERROR: ERROR: Illegal file info specified"));
                            success = FALSE;
                        }
                        else
                        {
                            g_pFileName = TEXT("");
                        }
                    }
                    else
                    {
                        g_pFileName = _wcsdup(pCmdLineOpt);
                    }
                }
            }
        }

        // Get next parameter
        pCmdLineOpt = wcstok_s(NULL, ws, &cmdLineContext);

    }

    if ( g_dwMode == MODE_NONE )
    {
        g_pKato->Log(LOG_FAIL,
                     TEXT("ERROR: Must supply either the -r, -w, -wr or -c option"));
        success = FALSE;
    }
    else if ( g_pFileName == NULL )
    {
        g_pKato->Log(LOG_FAIL,
                     TEXT("ERROR: No file info specified"));
        success = FALSE;
    }


    if ( showHelp || !success )
    {
        LogHelpInfo();
    }

    return success;
}

static void LogHelpInfo(void)
{
    g_pKato->Log(LOG_COMMENT, TEXT(""));
    g_pKato->Log(LOG_COMMENT, TEXT("Command Line Options:"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -r  'filename'   Perform read speed tests on the named file"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -w  'filename'   Perform write speed tests on the named file"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -wr 'filename'   Perform write then read speed tests on the named file"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -c  'dirname'    Create a file in the named directory and verify its contents"));
    g_pKato->Log(LOG_COMMENT, TEXT("                    (do not include trailing backslash)"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -n NumTests      Number of times each test is repeated"));
    g_pKato->Log(LOG_COMMENT, TEXT("                    (valid range = %d - %d, default = %d)"),
                              NUM_TESTS_MIN, NUM_TESTS_MAX, NUM_TESTS_DEFAULT);
    g_pKato->Log(LOG_COMMENT, TEXT("  -s FileSize      Size of file(s) (in MB) to create during tests"));
    g_pKato->Log(LOG_COMMENT, TEXT("                    (valid range = %d - %d, default = %d)"),
                              FILE_SIZE_MIN, FILE_SIZE_MAX, FILE_SIZE_DEFAULT);
    g_pKato->Log(LOG_COMMENT, TEXT("  -b BufferSize    Size of data buffer (in bytes) to use during custom test"));
    g_pKato->Log(LOG_COMMENT, TEXT("                    (valid range = %d - %d, default = skip custom test)"),
                              BLOCK_SIZE_MIN, BLOCK_SIZE_MAX);
    g_pKato->Log(LOG_COMMENT, TEXT("  -a               Perform all tests"));
    g_pKato->Log(LOG_COMMENT, TEXT("                    (default = use 256B, 8K and 64K data buffers only)"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -nocache         Access files with no system caching"));
    g_pKato->Log(LOG_COMMENT, TEXT("                    (default = system caching enabled)"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -writethru       Write through any intermediate cache"));
    g_pKato->Log(LOG_COMMENT, TEXT("                    (default = Lazy flush possible)"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -flushall        Flush file buffers after every write"));
    g_pKato->Log(LOG_COMMENT, TEXT("                    (default = Not performed)"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -flushend        Flush file buffers before closing file"));
    g_pKato->Log(LOG_COMMENT, TEXT("                    (default = Not performed)"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -v               Verbose output"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -h               Show command line option help"));
    g_pKato->Log(LOG_COMMENT, TEXT("Examples:"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -r '\\test.dat' -n 10 -v"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -w '\\Storage Card\\test.dat' -s 4 -nocache"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -wr '\\Hard Disk\\test.dat' -b 15000 -flushall"));
    g_pKato->Log(LOG_COMMENT, TEXT("  -c '\\NAND Flash' -s 1 -n 3 -writethru"));
    g_pKato->Log(LOG_COMMENT, TEXT("Notes:"));
    g_pKato->Log(LOG_COMMENT, TEXT("  * When testing throughput on a TFAT file system it is strongly recommended"));
    g_pKato->Log(LOG_COMMENT, TEXT("     that the -writethru option is used."));
    g_pKato->Log(LOG_COMMENT, TEXT("  * Do not combine the -nocache and -flushall options"));
    g_pKato->Log(LOG_COMMENT, TEXT("  * When combining the -b and -a options, the -a option will be ignored"));
    g_pKato->Log(LOG_COMMENT, TEXT("  * Option identifiers (-w, -h etc) may be supplied in upper or lower case"));
    g_pKato->Log(LOG_COMMENT, TEXT(""));
}

