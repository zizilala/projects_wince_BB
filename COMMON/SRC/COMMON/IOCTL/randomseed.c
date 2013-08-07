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
//  File:  RandomSeed.c
//
//  This file implements the IOCTL_HAL_GET_RANDOM_SEED handler.
//
#include <windows.h>
#include <oal.h>
#include <nkintr.h>

//------------------------------------------------------------------------------
//
//  Function:  OALIoCtlHalGetRandomSeed
//
//  Implements the IOCTL_HAL_GET_RANDOM_SEED handler. This function creates a
//  64-bit random number and stores it in the output buffer.
//
BOOL OALIoCtlHalGetRandomSeed( 
    UINT32 dwIoControlCode, VOID *lpInBuf, UINT32 nInBufSize, VOID *lpOutBuf, 
    UINT32 nOutBufSize, UINT32* lpBytesReturned)
{
    BOOL rc = FALSE;
    LARGE_INTEGER perfCounterEntropy;
    SYSTEMTIME systemTimeEntropy;

    UINT32 maxAllowedRequest = 1024;

    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"+OALIoctlHalGetRandomSeed(...)\r\n"));
    if (lpOutBuf == NULL || nOutBufSize > maxAllowedRequest)
    {
        NKSetLastError(ERROR_INVALID_PARAMETER);
        OALMSG(OAL_WARN, (L"WARN: OALIoCtlHalGetRandomSeed: Invalid parameter\r\n"));
        goto cleanUp;
    }

    if(!pQueryPerformanceCounter)
    {   
        //function pointer invalid - platform doesn't have a valid performance counter
        if(lpBytesReturned)
        {
            *lpBytesReturned = 0;
        }
        NKSetLastError(ERROR_NOT_SUPPORTED);
        goto cleanUp;
    }

    if(pQueryPerformanceCounter(&perfCounterEntropy))
    {
        //generated random number successfully
        BYTE entropyBuffer[sizeof(perfCounterEntropy)+sizeof(systemTimeEntropy)];
        UINT32 numBytesToFill; //we only have a small amount of random data, so don't fill up more than that amount
        UINT32 fillBufferSize = sizeof(perfCounterEntropy); //how many bytes of random data we have available to us

        memcpy(entropyBuffer,&perfCounterEntropy,sizeof(perfCounterEntropy)); //copy performance counter data into the buffer

        if(OEMGetRealTime(&systemTimeEntropy))
        {
            fillBufferSize += sizeof(systemTimeEntropy); //real-time clock can provide additional random data
            memcpy(((BYTE*)entropyBuffer)+sizeof(perfCounterEntropy),&systemTimeEntropy,fillBufferSize - sizeof(perfCounterEntropy)); //copy systemtime data (if we have it) into the buffer
        }

        if(nOutBufSize < fillBufferSize)
        {
            UINT32 xorLoopCounter;
            UINT32 entropyBufferPos = 0;
            numBytesToFill = nOutBufSize; //the caller may want less random data than have to give, so use XOR to distribute leftover randomness
            for(xorLoopCounter = numBytesToFill; xorLoopCounter < fillBufferSize; xorLoopCounter++)
            {
                //XOR leftover random bytes to pack all of our randomness into the amount of bytes requested
                entropyBuffer[entropyBufferPos] = (entropyBuffer[entropyBufferPos] ^ entropyBuffer[xorLoopCounter]);
                if(entropyBufferPos >= nOutBufSize)
                {
                   entropyBufferPos = 0;  //wrap back around to the beginning
                }
                else
                {
                    entropyBufferPos++;
                }
            }
        }
        else
        {
            numBytesToFill = fillBufferSize; //the caller may want equal or more random data than we have to give, fill as much as we've got
        }

        memcpy(lpOutBuf,entropyBuffer,(numBytesToFill)); //copy the amount of data that we have into the caller's buffer

        if(nOutBufSize > numBytesToFill)
        {
            memset((void*)(((BYTE*)lpOutBuf)+((numBytesToFill))),0,nOutBufSize-numBytesToFill); //if the caller asked for more than we had, zero the remainder
        }

        if(lpBytesReturned)
        {
            *lpBytesReturned = numBytesToFill;
        }
    }
    else
    { 
        //failed on generating random number
        if(lpBytesReturned)
        {
            *lpBytesReturned = 0;
        }
        NKSetLastError(ERROR_PROCESS_ABORTED);
        goto cleanUp;
    }
    
    // We are done
    rc = TRUE;
    
cleanUp:
    // Indicate status
    OALMSG(OAL_IOCTL&&OAL_FUNC, (L"-OALIoCtlHalGetRandomSeed(rc = %d)\r\n", rc));
    return rc;
}

