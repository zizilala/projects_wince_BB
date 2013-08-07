//
// Copyright (c) MPC Data Limited 2010. All Rights Reserved.
//
// File:  fstest.c
//
// File System Read/Write Perform Test CETK DLL
//

#include <windows.h>

#include "main.h"
#include "globals.h"

#define LOGTYPE_NONE  0
#define LOGTYPE_ERROR 1
#define LOGTYPE_INFO  2


BOOL TEST_Aborted = FALSE;
TCHAR * ActiveFileName = NULL;


static BOOL VerifyFile(TCHAR *FileName, DWORD BufferSize);
static BOOL ReadFile(TCHAR *FileName, DWORD BufferSize, DWORD NumTests, BOOL Compare, BOOL LogData=TRUE);
static BOOL WriteFile(TCHAR *FileName, DWORD BufferSize, DWORD NumTests, BOOL Compare, BOOL LogData=TRUE);


static BOOL ReadFileBlock(HANDLE fHandle, char * Buffer,
                          DWORD BytesToRead, LPDWORD BytesRead, 
                          DWORD Position);

static void BufferSetup(BOOL First,
                        char *BufferPtr, DWORD BufferSize,
                        DWORD Data1);


TCHAR *BytesToString(DWORD numBytes)
{
    static TCHAR txtBuf[64];
    float calc;
    
    if (numBytes < 2048)
    {
        wsprintf(txtBuf, L"%u bytes", numBytes);
    }
    else
    {
        calc = (float)numBytes / (float)1024.0;
        if (calc <= (float)1024.0)
        {
            wsprintf(txtBuf, L"%.2fKB", calc);
        }
        else
        {
            calc /= (float)1024.0;
            wsprintf(txtBuf, L"%.2fMB", calc);
        }
    }
    
    return txtBuf;
}


void LogResult(DWORD dwVerbosity, DWORD dwType, LPCWSTR wszFormat, ...)
{
    va_list pArgs;
    TCHAR msgFormat[300];
    TCHAR *prefix;

    switch(dwType)
    {
        case LOGTYPE_ERROR:
            prefix = TEXT("ERROR: ");
            break;

        case LOGTYPE_INFO:
            prefix = TEXT("INFO: ");
            break;

        case LOGTYPE_NONE:
        default:
            prefix = TEXT("");
            break;
    }
    wsprintf(msgFormat, TEXT("%s%s"), prefix, wszFormat);   

    va_start(pArgs, wszFormat);
    g_pKato->LogV(dwVerbosity, msgFormat, pArgs);
    va_end(pArgs);

    // increment error counter
    if (dwVerbosity == LOG_FAIL || dwType == LOGTYPE_ERROR)
    {
        g_dwNumErrors++;
    }
}


static void LogThroughput(DWORD TestsDone, DWORD BytesDone, DWORD Duration)
{
   	UNREFERENCED_PARAMETER(TestsDone);

    LogResult(LOG_COMMENT, LOGTYPE_INFO,
              TEXT("  System File Cache ..: %s"),
                  (g_bNoCaching ? TEXT("Off") : TEXT("On")));

#ifdef DEBUG_CALC

    LogResult(LOG_COMMENT, LOGTYPE_INFO,
              TEXT("  Total Bytes ........: %u"),
              BytesDone);

#else

    LogResult(LOG_COMMENT, LOGTYPE_INFO,
              TEXT("  Total Bytes ........: %s"),
              BytesToString(BytesDone));

#endif

    {
#ifdef DEBUG_CALC

        LogResult(LOG_COMMENT, LOGTYPE_INFO,
                  TEXT("  Total Time Taken ...: %u mS"),
                  Duration);

#else

        DWORD mins, secs, psecs;
        
        secs =  (Duration / 1000);
        psecs = (Duration % 1000);
        if (secs > 119)
        {
            secs = (secs % 60);
            mins =  (Duration / 60000);

            LogResult(LOG_COMMENT, LOGTYPE_INFO,
                      TEXT("  Total Time Taken ...: %u minutes %u.%03u seconds"),
                      mins, secs, psecs);
        }
        else
        {
            LogResult(LOG_COMMENT, LOGTYPE_INFO,
                      TEXT("  Total Time Taken ...: %u.%03d seconds"),
                      secs, psecs);
        }

#endif
    }
    
    if (BytesDone > 0 && Duration > 0)
    {
        double calc;
        DWORD rate;

        calc = (double)BytesDone;
        calc *= 1000.0;
        calc /= (double)Duration;
        rate = (DWORD)fabs(calc);
        
        LogResult(LOG_COMMENT, LOGTYPE_INFO,
                  TEXT("  Average Throughput .: %s / second"),
                  BytesToString(rate));
    }
}




//==========================================================================
// TestProc
//  Executes one test.
//
// Parameters:
//    uMsg        Message code.
//    tpParam     Additional message-dependent data.
//    lpFTE       Function table entry that generated this call.
//
// Return value:
//    TPR_PASS if the test passed
//    TPR_FAIL if the test fails
//    TPR_SKIP if test skipped
//==========================================================================
TESTPROCAPI TestProc_FileSystem(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    int status = TPR_PASS;
    //LPTPS_EXECUTE tExecute = (LPTPS_EXECUTE)tpParam;
    DWORD bufferSize = 0;
	UNREFERENCED_PARAMETER(tpParam);
    
    // See what the shell wants us to do
    if (uMsg == TPM_QUERY_THREAD_COUNT)
    {
        status = TPR_NOT_HANDLED;
    }
    else if (uMsg != TPM_EXECUTE)
    {
        status = TPR_NOT_HANDLED;
    }
    else
    {
        LogResult(LOG_COMMENT, LOGTYPE_NONE, TEXT(""));

        if (g_dwBlockSize != 0)
        {
            // We have a custom block size to use
            if (lpFTE->dwUserData != 0)
            {
                // Skip fixed block size tests
                LogResult(LOG_COMMENT, LOGTYPE_INFO,
                          TEXT("Only testing using the custom buffer size specified on the command line"));
                status = TPR_SKIP;
            }
            else
            {
                // Use our defined block size
                bufferSize = g_dwBlockSize;
            }
        }
        else
        {
            bufferSize = lpFTE->dwUserData;
            if (bufferSize == 0)
            {
                // Skip custom block size test
                LogResult(LOG_COMMENT, LOGTYPE_INFO,
                          TEXT("No custom buffer size was specifed on the command line"));
                status = TPR_SKIP;
            }
            else if (bufferSize != 256 &&
                bufferSize != (8 * 1024) &&
                bufferSize != (64 * 1024) )
            {
                if (!g_bDoAllTests)
                {
                    LogResult(LOG_COMMENT, LOGTYPE_INFO,
                              TEXT("Quick test using limited buffer sizes only"));
                    status = TPR_SKIP;
                }   
            }
        }
    
        if (status == TPR_PASS)
        {
            switch(g_dwMode)
            {
    
                case MODE_READ:
                    if (!ReadFile(g_pFileName, bufferSize, g_dwNumTests, FALSE))
                    {
                        status = TPR_FAIL;
                    }
                    break;
        
                case MODE_WRITE:
                    if (!WriteFile(g_pFileName, bufferSize, g_dwNumTests, FALSE))
                    {
                        status = TPR_FAIL;
                    }
                    break;
            
                case MODE_WRITEREAD:
                    if (!WriteFile(g_pFileName, bufferSize, g_dwNumTests, FALSE))
                    {
                        status = TPR_FAIL;
                    }
                    else if(!ReadFile(g_pFileName, bufferSize, g_dwNumTests, FALSE))
                    {
                        status = TPR_FAIL;
                    }
                    break;
            
                case MODE_VERIFY:
                    if (!VerifyFile(g_pFileName, bufferSize))
                    {
                        status = TPR_FAIL;
                    }
                    break;
            
                default:
                    status = TPR_FAIL;
                    break;
    
            }

        }
    
        LogResult(LOG_COMMENT, LOGTYPE_NONE, TEXT(""));
    }
    
    return status;
}


//==========================================================================
// VerifyFile
//    Do write / read & compare operations in named directory.
//
// Parameters:
//    FileName     Name of directory in which to create test files.
//    BufferSize   Buffer size to use when accessing the files.
//
// Return value:
//    TRUE if test performed without errors, FALSE if not
//==========================================================================
static BOOL VerifyFile(TCHAR *FileName, DWORD BufferSize)
{
    BOOL fSuccess = TRUE;
    DWORD dwLoopCount = 0;
    DWORD dwNumTries;
    TCHAR sFileName[300];
    DWORD dwMaxNumTries = 10;
    BOOL bLogData;

    while (fSuccess && !TEST_Aborted && (dwLoopCount++ < g_dwNumTests))
    {
        wsprintf(sFileName, TEXT("%s\\FSTest_%04u.dat"), FileName, dwLoopCount);
        
        for (dwNumTries = 1; (dwNumTries <= dwMaxNumTries) && !TEST_Aborted && fSuccess; dwNumTries++)
        {
            bLogData = (dwNumTries == dwMaxNumTries);
            fSuccess = WriteFile(sFileName, BufferSize, 1, TRUE, bLogData);
            if (fSuccess)
            {
                fSuccess = ReadFile(sFileName, BufferSize, 1, TRUE, bLogData);
            }
            DeleteFile(sFileName);
        }

    }

    return fSuccess;
}


//==========================================================================
// WriteFile
//    Times writing data to a specified file
//    At end of test, created file is not deleted.
//
// Parameters:
//    FileName     Name of file to create during testing.
//    BufferSize   Buffer size to use when writing to the file.
//    NumTests     Number of times to perform operation
//    Compare      Whether data comparison will be performed
//
// Return value:
//    TRUE if test performed without errors, FALSE if not
//==========================================================================
static BOOL WriteFile(TCHAR *FileName, DWORD BufferSize, DWORD NumTests,
                      BOOL Compare, BOOL bLogData)
{
    BOOL success = TRUE;
    DWORD dwCheckTick;
    DWORD dwStartTick;
    DWORD totalBytesDone;
    DWORD totalTransfers;
    DWORD totalDuration;
    DWORD bytesDone;
    DWORD passBytesDone;
    DWORD passDuration;
    DWORD dwError;
    DWORD loopcount;
    HANDLE fHandle;
    char *txBuffer = NULL;
    DWORD flags;

    if (FileName == NULL || BufferSize < 16)
    {
        LogResult(LOG_FAIL, LOGTYPE_ERROR,
                  TEXT("WriteFile - Invalid parameter supplied (%p %u)"),
                  FileName, BufferSize);
        success = FALSE;   
    }
    else
    {
        txBuffer = (char *)malloc(BufferSize + (sizeof(DWORD) * 2));
        if (txBuffer == NULL)
        {
            LogResult(LOG_FAIL, LOGTYPE_ERROR,
                      TEXT("Unable to allocate space for %u byte write buffer"),
                      BufferSize);
            success = FALSE;   
        }
    }

    loopcount = 0;
    totalBytesDone = 0;
    totalTransfers = 0;
    totalDuration = 0;

    if ( bLogData )
    {
        LogResult(LOG_COMMENT, LOGTYPE_INFO,
                  TEXT("Starting Write Tests (using file '%s')"),
                  FileName);
    }

    while (success && !TEST_Aborted && (loopcount++ < NumTests))
    {

        if ( bLogData && g_bVerboseOutput && (NumTests > 1))
        {
            LogResult(LOG_COMMENT, LOGTYPE_INFO,
                      TEXT(" Starting iteration %u"),
                      loopcount);
        }

        DeleteFile(FileName);

        if (loopcount > 1)
        {
            // Give system caches and other threads 
            // time to sort themselves out
            Sleep(2000);
        }

        if (bLogData && g_bVerboseOutput)
        {
            LogResult(LOG_COMMENT, LOGTYPE_INFO,
                      TEXT("   Creating file"));
        }

        flags = FILE_ATTRIBUTE_NORMAL;
        if (g_bNoCaching)
        {
            flags |= FILE_FLAG_NO_BUFFERING;
        }
        if (g_bWriteThrough)
        {
            flags |= FILE_FLAG_WRITE_THROUGH;
        }
        fHandle = CreateFile(FileName, GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ, NULL,
                                      CREATE_ALWAYS,
                                      flags,
                                      NULL);
        if ( fHandle == INVALID_HANDLE_VALUE )
        {
            // unable to open file
            dwError = GetLastError();
            LogResult(LOG_FAIL, LOGTYPE_ERROR,
                      TEXT("Unable to create file %s (error %d)"),
                           FileName, dwError);
            success = FALSE;
        }

        if ( success )
        {
            // Initialise the txBuffer with something so the
            // written file does not contain random data
            //if (Compare)
            {
                BufferSetup(TRUE, txBuffer, BufferSize, 0);
            }
            
            // Give other threads time to do something
            Sleep(500);

            passBytesDone = 0;
            dwStartTick = GetTickCount();
            dwCheckTick = dwStartTick;
            while (success && TEST_Aborted == 0 && passBytesDone < g_dwTestFileSize )
            {

                if (Compare)
                {
                    BufferSetup(FALSE, txBuffer, BufferSize, passBytesDone);
                }
                
                // Write data block
                bytesDone = 0;
                success = (WriteFile(fHandle, txBuffer, BufferSize, &bytesDone, NULL) != 0);
                passBytesDone += bytesDone;
                totalTransfers++;

                if (!success)
                {
                    dwError = GetLastError();
                    LogResult(LOG_FAIL, LOGTYPE_ERROR,
                              TEXT("Unable to write %u bytes to file '%s' at location %u (error %d)"),
                              (unsigned int)BufferSize, 
                              FileName, 
                              (unsigned long)passBytesDone,
                              dwError);
                }
                else
                {
                    // Log percentage done every 2 seconds
                    if (g_bVerboseOutput)
                    {
                        if ((GetTickCount() - dwCheckTick) > 2000)
                        {
                            if (passBytesDone < g_dwTestFileSize)
                            {
                                double calc;
                                
                                calc = (double)(passBytesDone * 100) / (double)g_dwTestFileSize;
                                if ( bLogData )
                                {
                                    LogResult(LOG_COMMENT, LOGTYPE_INFO,
                                          TEXT("   Written %5.1f%%"), calc);
                                }
                                dwCheckTick = GetTickCount();
                            }
                        }
                    }

                    // Flush buffers
                    if (g_bFlushBuffersOnWrite)
                    {
                        if (FlushFileBuffers(fHandle) == 0)
                        {
                            dwError = GetLastError();
                            LogResult(LOG_FAIL, LOGTYPE_ERROR,
                                      TEXT("Unable to flush buffers for file '%s' at location %u (error %d)"),
                                      FileName, 
                                      (unsigned long)passBytesDone,
                                      dwError);
                            success = FALSE;
                        }
                    }

                }

            }
            //passDuration = GetTickCount() - dwStartTick;

            if (bLogData && g_bVerboseOutput)
            {
                LogResult(LOG_COMMENT, LOGTYPE_INFO,
                          TEXT("   Written 100.00%%"));
            }

            if (g_bFlushBuffersOnClose)
            {
                if (bLogData && g_bVerboseOutput)
                {
                    LogResult(LOG_COMMENT, LOGTYPE_INFO,
                              TEXT("   Flushing buffers"));
                }
                if ( FlushFileBuffers(fHandle) == 0 )
                {
                    dwError = GetLastError();
                    LogResult(LOG_FAIL, LOGTYPE_ERROR,
                              TEXT("Unable to flush buffers for file %s (error %d)"),
                              FileName, dwError);
                    success = FALSE;
                }
            }
            passDuration = GetTickCount() - dwStartTick;

            if (bLogData && g_bVerboseOutput)
            {
                LogResult(LOG_COMMENT, LOGTYPE_INFO,
                          TEXT("   Closing file"));
            }
            if ( CloseHandle(fHandle) == 0 )
            {
                // unable to close file
                dwError = GetLastError();
                LogResult(LOG_FAIL, LOGTYPE_ERROR,
                          TEXT("Unable to close file %s (error %d)"),
                          FileName, dwError);
                success = FALSE;
            }

            if (bLogData && g_bVerboseOutput && (loopcount < NumTests))
            {
                LogThroughput(loopcount, passBytesDone, passDuration);
            }

            totalBytesDone += passBytesDone;
            totalDuration += passDuration;

        }

    }

    if (bLogData && (totalBytesDone > 0))
    {
        if (g_bVerboseOutput)
        {
            LogResult(LOG_COMMENT, LOGTYPE_INFO, TEXT(" Result"),
                        loopcount);
        }
        if (NumTests > 1)
        {
            LogResult(LOG_COMMENT, LOGTYPE_INFO, TEXT("  Test Iterations ....: %u"),
                      NumTests);
        }
        LogResult(LOG_COMMENT, LOGTYPE_INFO, TEXT("  File Size ..........: %s"),
                  BytesToString(g_dwTestFileSize));
        LogResult(LOG_COMMENT, LOGTYPE_INFO, TEXT("  Write Buffer Size ..: %s"),
                  BytesToString(BufferSize));
        LogThroughput(loopcount, totalBytesDone, totalDuration);
    }

    if (txBuffer != NULL)
    {
        free(txBuffer);
        txBuffer = NULL;
    }

    return success;
}


//==========================================================================
// ReadFile
//    Times reading data from a specified file
//
// Parameters:
//    FileName     Name of file to create during testing.
//    BufferSize   Buffer size to use when reading from the file.
//    NumTests     Number of times to perform operation
//    Compare      Whether data comparison will be performed
//
// Return value:
//    TRUE if test performed without errors, FALSE if not
//==========================================================================
static BOOL ReadFile(TCHAR *FileName, DWORD BufferSize, DWORD NumTests,
                     BOOL Compare, BOOL bLogData)
{
    BOOL success = TRUE;
    DWORD dwCheckTick;
    DWORD dwStartTick;
    DWORD totalBytesDone;
    DWORD totalTransfers;
    DWORD totalDuration;
    DWORD bytesDone;
    DWORD passBytesDone;
    DWORD passDuration;
    DWORD dwError;
    DWORD dwOffset;
    DWORD loopcount;
    HANDLE fHandle;
    DWORD fileSize;
    char *rxBuffer = NULL;
    char *cmpBuffer = NULL;
    DWORD flags;

    if (FileName == NULL || BufferSize < 16)
    {
        LogResult(LOG_FAIL, LOGTYPE_ERROR,
                  TEXT("ReadFile - Invalid parameter supplied (%p %u)"),
                  FileName, BufferSize);
        success = FALSE;   
    }
    else
    {
        rxBuffer = (char *)malloc(BufferSize + 8);
        if (rxBuffer == NULL)
        {
            LogResult(LOG_FAIL, LOGTYPE_ERROR,
                      TEXT("Unable to allocate space for %u byte read buffer"),
                      BufferSize);
            success = FALSE;    
        }
        else if (Compare)
        {
            cmpBuffer = (char *)malloc(BufferSize + 8);
            if (cmpBuffer == NULL)
            {
                LogResult(LOG_FAIL, LOGTYPE_ERROR,
                          TEXT("Unable to allocate space for %u byte comparison buffer"),
                          BufferSize);
                success = FALSE;    
            }
        }
    }

    fileSize = 0;
    loopcount = 0;
    totalBytesDone = 0;
    totalTransfers = 0;
    totalDuration = 0;

    if ( bLogData )
    {
        LogResult(LOG_COMMENT, LOGTYPE_INFO,
                  TEXT("Starting Read%s Tests (using file '%s')"),
                  ((Compare)?TEXT(" & Verify"):TEXT("")), FileName);
    }

    while (success && !TEST_Aborted && (loopcount++ < NumTests))
    {

        if (bLogData && g_bVerboseOutput && (NumTests > 1))
        {
            LogResult(LOG_COMMENT,
                      LOGTYPE_INFO, TEXT(" Starting iteration %u"),
                      loopcount);
        }

        if (loopcount > 1)
        {
            // Give system caches and other threads 
            // time to sort themselves out
            Sleep(2000);
        }

        if (bLogData && g_bVerboseOutput)
        {
            LogResult(LOG_COMMENT, LOGTYPE_INFO,
                      TEXT("   Opening file"));
        }

        flags = FILE_ATTRIBUTE_NORMAL;
        if (g_bNoCaching)
        {
            flags |= FILE_FLAG_NO_BUFFERING;
        }
        fHandle = CreateFile(FileName, GENERIC_READ,
                                      FILE_SHARE_READ, NULL,
                                      OPEN_EXISTING,
                                      flags,
                                      NULL);
        if ( fHandle == INVALID_HANDLE_VALUE )
        {
            // unable to open file
            dwError = GetLastError();
            LogResult(LOG_FAIL, LOGTYPE_ERROR,
                      TEXT("Unable to open file %s (error %d)"),
                      FileName, dwError);
            success = FALSE;
        }

        if ( success )
        {
            
            fileSize = GetFileSize(fHandle, NULL);
            if (fileSize == 0xFFFFFFFF)
            {
                dwError = GetLastError();
                LogResult(LOG_FAIL, LOGTYPE_ERROR,
                          TEXT("Unable to get size of file %s (error %d)"),
                          FileName, dwError);
                fileSize = 0;
                success = FALSE;
            }

            FlushFileBuffers(fHandle);
            
            if (Compare)
            {
                BufferSetup(TRUE, cmpBuffer, BufferSize, 0);
            }

            Sleep(500);

            passBytesDone = 0;
            dwStartTick = GetTickCount();
            dwCheckTick = dwStartTick;
            while (success && TEST_Aborted == 0 && passBytesDone < fileSize )
            {

                if (Compare)
                {
                    BufferSetup(FALSE, cmpBuffer, BufferSize, passBytesDone);
                }

                bytesDone = 0;
                success = ReadFileBlock(fHandle, rxBuffer, BufferSize, &bytesDone, passBytesDone);
                passBytesDone += bytesDone;
                totalTransfers++;

                if (bytesDone == 0)
                {
                    // Reached end of file
                    break;
                }
                else if (success)
                {
                    // Log percentage done every 2 seconds
                    if (g_bVerboseOutput)
                    {
                        if ((GetTickCount() - dwCheckTick) > 2000)
                        {
                            if (passBytesDone < fileSize )
                            {
                                double calc;
                                
                                calc = (double)(passBytesDone * 100) / (double)fileSize;
                                if ( bLogData )
                                {
                                    LogResult(LOG_COMMENT, LOGTYPE_INFO,
                                              TEXT("   Read %5.1f%%"), calc);
                                }
                                dwCheckTick = GetTickCount();
                            }
                        }
                    }
                    
                    if (Compare)
                    {
                        if (bytesDone != BufferSize)
                        {
                            LogResult(LOG_COMMENT, LOGTYPE_INFO,
                                      TEXT("Read different size to request at location %u (wanted=%u, got=%u)"),
                                      (unsigned long)passBytesDone-bytesDone, BufferSize, bytesDone);
                        }
                        else if (memcmp(cmpBuffer, rxBuffer, BufferSize) != 0)
                        {
                            success = FALSE;

                            dwOffset = 0;
                            while ((rxBuffer[dwOffset] == cmpBuffer[dwOffset]) &&
                                   (dwOffset < BufferSize))
                            {
                                dwOffset++;
                            }
                            LogResult(LOG_FAIL, LOGTYPE_ERROR,
                                      TEXT("Validation error at location %u"),
                                      (unsigned long)(passBytesDone-bytesDone)+dwOffset);
                        }
                    }
                    
                }

            }
            passDuration = GetTickCount() - dwStartTick;

            if (bLogData && g_bVerboseOutput)
            {
                LogResult(LOG_COMMENT, LOGTYPE_INFO,
                          TEXT("   Read 100.00%%"));
            }
            
            if (g_bFlushBuffersOnClose)
            {
                if (bLogData && g_bVerboseOutput)
                {
                    LogResult(LOG_COMMENT, LOGTYPE_INFO,
                              TEXT("   Flushing buffers"));
                }
                if ( FlushFileBuffers(fHandle) == 0 )
                {
                    dwError = GetLastError();
                    LogResult(LOG_FAIL, LOGTYPE_ERROR,
                              TEXT("Unable to flush buffers for file %s (error %d)"),
                              FileName, dwError);
                    success = FALSE;
                }
            }
            
            if (bLogData && g_bVerboseOutput)
            {
                LogResult(LOG_COMMENT, LOGTYPE_INFO,
                          TEXT("   Closing file"));
            }
            if ( CloseHandle(fHandle) == 0 )
            {
                // unable to close port
                dwError = GetLastError();
                LogResult(LOG_FAIL, LOGTYPE_ERROR,
                          TEXT("Unable to close file %s (error %d)"),
                          FileName, dwError);
                success = FALSE;
            }
        
            if (bLogData && g_bVerboseOutput && (loopcount < NumTests))
            {
                LogThroughput(loopcount, passBytesDone, passDuration);
            }

            totalBytesDone += passBytesDone;
            totalDuration += passDuration;

        } 

    }

    if (bLogData && (totalBytesDone > 0))
    {
        if (g_bVerboseOutput)
        {
            LogResult(LOG_COMMENT, LOGTYPE_INFO, TEXT(" Result"),
                        loopcount);
        }
        if (NumTests > 1)
        {
            LogResult(LOG_COMMENT, LOGTYPE_INFO, TEXT("  Test Iterations ....: %u"),
                  NumTests);
        }
        LogResult(LOG_COMMENT, LOGTYPE_INFO, TEXT("  File Size ..........: %s"),
                  BytesToString(fileSize));
        LogResult(LOG_COMMENT, LOGTYPE_INFO, TEXT("  Read Buffer Size ...: %s"),
                  BytesToString(BufferSize));
        LogThroughput(loopcount, totalBytesDone, totalDuration);
    }

    if (cmpBuffer != NULL)
    {
        free(cmpBuffer);
        cmpBuffer = NULL;
    }
    if (rxBuffer != NULL)
    {
        free(rxBuffer);
        rxBuffer = NULL;
    }

    return success;
}


//==========================================================================
// ReadFileBlock
//    Read specific length of data from file
//
// Parameters:
//    fHandle       Handle to open file
//    Buffer        Buffer to put data read from file into
//    BytesToRead   Size of buffer to be filled
//    BytesRead     Location to place number of bytes read in
//    Position      Starting location of file pointer
//
// Return value:
//    TRUE if data read in ok, FALSE if error
//==========================================================================
static BOOL ReadFileBlock(HANDLE fHandle, char * Buffer, 
                          DWORD BytesToRead, LPDWORD BytesRead, 
                          DWORD Position)
{
    BOOL success = TRUE;
    DWORD bytesDone = 0;
    DWORD bytesNeeded;
    DWORD dwError;
    DWORD bytesRead;
    
    while (success && (bytesDone < BytesToRead))
    {
        bytesNeeded = BytesToRead - bytesDone;
        
        if (ReadFile(fHandle,
                     &Buffer[bytesDone], bytesNeeded, 
                     &bytesRead, NULL) == 0)
        {
            dwError = GetLastError();
            LogResult(LOG_FAIL, LOGTYPE_ERROR,
                      TEXT("Unable to read %u bytes from file at location %u (error %d)"),
                      (unsigned int)bytesNeeded, 
                      (unsigned long)Position+bytesDone,
                      dwError);

            success = FALSE;
        }
        else if (bytesRead == 0) 
        {
            // Found end of file
            break;            
        }
        else
        {
            bytesDone += bytesRead;    
        }

    }
    
    *BytesRead = bytesDone;

    return success;
}


//==========================================================================
// BufferSetup
//    Setup buffer in standard way. 
//
// Parameters:
//    First        First time buffer used
//    Buffer       Buffer to initialise
//    BufferSize   Size of buffer
//    Data1        DWORD value
//
// Return value:
//    TRUE if data read in ok, FALSE if error
//==========================================================================
static void BufferSetup(BOOL First, char *Buffer, DWORD BufferSize, DWORD Data1)
{
    DWORD offset;
    int dSize;
    DWORD *xPtr;
    
    if (First)
    {
        DWORD blockSize;
        char dChar;
        
        dChar = '=';
        offset = 0;
        while (offset < BufferSize)
        {
            blockSize = min((BufferSize - offset), 202); 
            memset(&Buffer[offset], dChar, blockSize);
            Buffer[offset] = '<';
            Buffer[offset + 1] = '<';
            offset += blockSize;
            if (blockSize > 3)
            {
                Buffer[offset - 4] = '>';
                Buffer[offset - 3] = '>';
            }
            if (blockSize > 1)
            {
                Buffer[offset - 2] = '\r';
                Buffer[offset - 1] = '\n';
            }
            dChar = '-';
        }
    }
    else
    {
        // Add changing marker at end
        if (BufferSize > 64)
        {
            offset = BufferSize - 5 - 23;
            dSize = sprintf(&Buffer[offset], " @ %8u %8u @",
                            (unsigned long)Data1, (unsigned long)BufferSize);
            Buffer[offset + dSize] = ' ';
        }
        // Add changing marker at beginning
        if (BufferSize > 32)
        {
            offset = 3;
            dSize = sprintf(&Buffer[offset], " # %8u %8u #",
                            (unsigned long)Data1, (unsigned long)BufferSize);
            Buffer[offset + dSize] = ' ';
        }
        else if (BufferSize > 8)
        {
            xPtr = (DWORD *)Buffer;
            xPtr[0] = Data1;
            xPtr[1] = BufferSize;
        }

    }

}

