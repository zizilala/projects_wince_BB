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
//  File:  SBL.cpp
//
//  Secure Boot Loader - verify the digital signature of a data packet
//

#include <windows.h>
#include <mincrypt.h>

#include <SBL.h>


#define CBR(x) if (!(x)) { hr = E_FAIL; goto Error; }


/////////////////////////////////////////////////
// Globals
//

static BOOL         bFoundCorrectKey = FALSE;
static BYTE         bHash[MINCRYPT_MAX_HASH_LEN];
static const BYTE * gpbPublicKey;
static DWORD        gdwPublicKeyLen;

/////////////////////////////////////////////////
// Verify
//

static HRESULT Verify(const BYTE * pbSig,   DWORD dwSigLen,
                      const BYTE * pbKey,   DWORD dwKeyLen,
                      BYTE * pbHash,        DWORD dwHashLen) 
{
    
    HRESULT hr = S_OK;
    LONG lResult;

    CRYPT_DER_BLOB blobSig;
    CRYPT_DER_BLOB blobKey;

    blobSig.cbData = dwSigLen;
    blobSig.pbData = const_cast <BYTE *> (pbSig);

    blobKey.cbData = dwKeyLen;
    blobKey.pbData = const_cast <BYTE *> (pbKey);

    lResult = MinCryptVerifySignedHash(
        CALG_SHA1,
        pbHash,
        dwHashLen,
        &blobSig,
        &blobKey);

#ifdef DEBUG
    NKDbgPrintfW(L"MinCryptVerifySignedHash() returned 0x%x\r\n", lResult);
#endif 

    CBR(ERROR_SUCCESS == lResult);

Error:
    return hr;
}

/////////////////////////////////////////////////
// SBL_VerifyPacket
//

HRESULT SBL_VerifyPacket(const PACKETDATA    * pPacketData, 
                         const PUBLICKEYDATA * pKeyData) 
{
    
    DWORD i, hHash, dwHashLen = 0;
    HRESULT hr = S_OK;
    LONG lReturn;
    BOOL bVerified = FALSE;
    CRYPT_DER_BLOB blob;
    
    lReturn = MinCryptCreateHashMemory(CALG_SHA1, &hHash);
    CBR(ERROR_SUCCESS == lReturn);
    
    // See if this packet contains any data
    // If so hash that data
    
    if (0 != pPacketData->dwDataLength) 
    {
        blob.cbData = pPacketData->dwDataLength;
        blob.pbData = const_cast <BYTE *> (pPacketData->pbData);

        lReturn = MinCryptUpdateHashMemory(CALG_SHA1, hHash, 1, &blob);
        CBR(ERROR_SUCCESS == lReturn);
    }

    // Store RecAddress, RecLength, RecCheck in the hash
    // Allows verification of these elements

    blob.cbData = sizeof(pPacketData->dwRecAddress);
    blob.pbData = (BYTE *) &pPacketData->dwRecAddress;
    lReturn = MinCryptUpdateHashMemory(CALG_SHA1, hHash, 1, &blob);
    CBR(ERROR_SUCCESS == lReturn);

    blob.cbData = sizeof(pPacketData->dwRecLength);
    blob.pbData = (BYTE *) &pPacketData->dwRecLength;
    lReturn = MinCryptUpdateHashMemory(CALG_SHA1, hHash, 1, &blob);
    CBR(ERROR_SUCCESS == lReturn);

    blob.cbData = sizeof(pPacketData->dwRecCheck);
    blob.pbData = (BYTE *) &pPacketData->dwRecCheck;
    lReturn = MinCryptUpdateHashMemory(CALG_SHA1, hHash, 1, &blob);
    CBR(ERROR_SUCCESS == lReturn);

    blob.cbData = sizeof(pPacketData->bFlags);
    blob.pbData = (BYTE *) &pPacketData->bFlags;
    lReturn = MinCryptUpdateHashMemory(CALG_SHA1, hHash, 1, &blob);
    CBR(ERROR_SUCCESS == lReturn);

    // Hash the global file random seed

    blob.cbData = RANDOM_SEED_LENGTH;
    blob.pbData = (BYTE *) pPacketData->bRandomSeed;
    lReturn = MinCryptUpdateHashMemory(CALG_SHA1, hHash, 1, &blob);
    CBR(ERROR_SUCCESS == lReturn);

    // Hash the sequence number

    blob.cbData = sizeof(pPacketData->dwSequenceNumber);
    blob.pbData = (BYTE *) &pPacketData->dwSequenceNumber;
    lReturn = MinCryptUpdateHashMemory(CALG_SHA1, hHash, 1, &blob);
    CBR(ERROR_SUCCESS == lReturn);

    lReturn = MinCryptGetHashParam(CALG_SHA1, hHash, bHash, &dwHashLen);
    CBR(ERROR_SUCCESS == lReturn);

#ifdef DEBUG
    NKDbgPrintfW(L"Hash = [");
    for (i = 0; i < dwHashLen; i++)
    {
        NKDbgPrintfW(L"%x ", bHash[i]);
    }
    NKDbgPrintfW(L"]\r\n\r\n");
#endif

    // This is an optimization. We could have many public keys and we don't know
    // which one this image was signed with. However once we find one that works
    // this will be the key that every packet is signed with so we don't have to
    // search again. After finding a key that works just verify each packet with
    // that key.

    if (!bFoundCorrectKey) 
    {
        for (i = pKeyData->wMinSearchIndex; i <= pKeyData->wMaxSearchIndex; i++) 
        {
            bVerified = (S_OK == Verify(pPacketData->pbSig, pPacketData->dwSigLength, pKeyData->rgpbPublicKeys[i], pKeyData->rgdwKeyLengths[i], bHash, dwHashLen));
            gpbPublicKey = pKeyData->rgpbPublicKeys[i];
            gdwPublicKeyLen = pKeyData->rgdwKeyLengths[i];
            
            if (bVerified) 
            {
                break;
            }
        }

        if (bVerified) 
        {
            bFoundCorrectKey = TRUE;
        }
    } 
    else 
    {
        bVerified = (S_OK == Verify(pPacketData->pbSig, pPacketData->dwSigLength, gpbPublicKey, gdwPublicKeyLen, bHash, dwHashLen));
    }

    CBR(bVerified);
    
Error:
    return hr;
}

