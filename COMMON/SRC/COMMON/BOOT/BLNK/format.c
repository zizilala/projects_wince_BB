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
//  File:  format.c
//
//  This file implements format function equivalent to kernel one.
//  It is intended to be used in boot loader.
//
#include <windows.h>
#include <oal.h>

//------------------------------------------------------------------------------

static VOID GetFormatValue(LPCWSTR *pszFormat, LONG *pWidth, va_list *ppArgList);
static VOID Reverse(WCHAR *pFirst, WCHAR *pLast);

//------------------------------------------------------------------------------
//
//  Function:  NKwvsprintfW
//
//
int NKwvsprintfW(
    LPWSTR szBuffer, LPCWSTR szFormat, va_list pArgList, int maxChars
) {
    static WCHAR upch[]  = L"0123456789ABCDEF";
    static WCHAR lowch[] = L"0123456789abcdef";
    enum { typeNone = 0, typeNumber, typeCh, typeString } type;
    enum { modeNone = 0, modeH, modeL, modeX } mode;
    BOOL padLeft, prefix, sign, upper;
    INT32 width, precision, radix, chars;
    WCHAR ch, fillCh;
    LPWSTR szPos, szW;
    LPSTR szC;
    UINT64 value;


    // First check input params
    if (szBuffer == NULL || szFormat == NULL || maxChars < 1) return 0;

    // Set actual possition
    szPos = szBuffer;
    
    // While there is format strings
    while (*szFormat != L'\0' && maxChars > 0) {

        // If it is other than format prefix, print it and move to next one
        if (*szFormat != L'%') {
            if (--maxChars <= 0) goto cleanUp;
            *szPos++ = *szFormat++;
            continue;
        }

        // Set flags to default values        
        padLeft = FALSE;
        prefix = FALSE;
        sign = FALSE;
        upper = FALSE;
        fillCh = L' ';
        width = 0;
        precision = -1;
        type = typeNone;
        mode = modeNone;

        // read pad left and prefix flags
        while (*++szFormat != L'\0') {
            if (*szFormat == L'-') {
                padLeft = TRUE;
            } else if (*szFormat == L'#') {
                prefix = TRUE;
            } else {
                break;
            }
        }

        // find fill character
        if (*szFormat == L'0') {
            fillCh = L'0';
            szFormat++;
        }
            
        // read the width specification 
        GetFormatValue(&szFormat, &width, &pArgList);

        // read the precision
        if (*szFormat == L'.') {
            szFormat++;
            GetFormatValue(&szFormat, &precision, &pArgList);
        }                

        // get the operand size
        if (*szFormat == L'l') {
            mode = modeL;
            szFormat++;
        } else if (*szFormat == L'w') {
            mode = modeL;
            szFormat++;
        } else if (
            szFormat[0] == L'I' && szFormat[1] == L'3' && szFormat[2] == L'2'
        ) {                
            mode = modeL;
            szFormat += 3;
        } else if (*szFormat == L'h') {
            mode = modeH;
            szFormat++;
        } else if (
            szFormat[0] == L'I' && szFormat[1] == L'6' && szFormat[2] == L'4'
        ) {
            mode = modeX;
            szFormat += 3;
        }

        // break if there is unexpected format string end
        if (*szFormat == L'\0') break;
            
        switch (*szFormat) {

        case L'i':
        case L'd':
            sign = TRUE;
        case L'u':
            radix = 10;
            type = typeNumber;
            if (mode == modeNone) mode = modeL;
            break;

        case L'X':
            upper = TRUE;
        case L'p':
        case L'x':
            radix = 16;
            type = typeNumber;
            if (mode == modeNone) mode = modeL;
            break;

        case L'c':
            if (mode == modeNone) mode = modeL;
            type = typeCh;
            break;
                
        case L'C':
            if (mode == modeNone) mode = modeH;
            type = typeCh;
            break;

        case L'a':
            mode = modeH;
            type = typeString;
            break;
            
        case L'S':
            if (mode == modeNone) mode = modeH;
            type = typeString;
            break;

        case 's':
            if (mode == modeNone) mode = modeL;
            type = typeString;
            break;

        default:
            if (--maxChars <= 0) goto cleanUp;
            *szPos++ = *szFormat;
        }

        // Move to next format character
        szFormat++;

        switch (type) {
        case typeNumber:
            // Special cases to act like MSC v5.10
            if (padLeft || precision >= 0) fillCh = L' ';
            // Fix possible prefix
            if (radix != 16) prefix = FALSE;
            // Depending on mode obtain value
            if (mode == modeH) {
                if (sign) {
                    value = (INT64)va_arg(pArgList, INT16);
                } else {
                    value = (UINT64)va_arg(pArgList, UINT16);
                }                    
            } else if (mode == modeL) {
                if (sign) {
                    value = (INT64)va_arg(pArgList, INT32);
                } else {
                    value = (UINT64)va_arg(pArgList, UINT32);
                }                    
            } else if (mode == modeX) {
                value = va_arg(pArgList, UINT64);
            } else {
                goto cleanUp;
            }
            // Should sign be printed?
            if (sign && (INT64)value < 0) {
                (INT64)value = -(INT64)value;
            } else {
                sign = FALSE;
            }
            // Start with reverse string
            szW = szPos;
            chars = 0;
            do {
                if (--maxChars <= 0) goto cleanUp;
                *szW++ = upper ? upch[value%radix] : lowch[value%radix];
                chars++;
            } while ((value /= radix) != 0 && maxChars > 0);
            // Fix sizes
            width -= chars;
            precision -= chars;
            if (precision > 0) width -= precision;
            // Fill to the field precision
            while (precision-- > 0) {
                if (--maxChars <= 0) goto cleanUp;
                *szW++ = L'0';
            }
            if (width > 0 && !padLeft) {
                // If we're filling with spaces, put sign first
                if (fillCh != L'0') {
                    if (sign) {
                        if (--maxChars <= 0) goto cleanUp;
                        *szW++ = L'-';
                        width--;
                        sign = FALSE;
                    }
                    if (prefix && radix == 16) {
                        if (--maxChars <= 0) goto cleanUp;
                        *szW++ = upper ? L'X' : L'x';
                        if (--maxChars <= 0) goto cleanUp;
                        *szW++ = L'0';
                        prefix = FALSE;
                    }
                }
                // Leave place for sign
                if (sign) width--;
                // Fill to the field width
                while (width-- > 0) {
                    if (--maxChars <= 0) goto cleanUp;
                    *szW++ = fillCh;
                }
                // Still have sign?
                if (sign) {
                    if (--maxChars <= 0) goto cleanUp;
                    *szW++ = L'-';
                    sign = FALSE;
                }
                // Or prefix?
                if (prefix) {
                    if (--maxChars <= 0) goto cleanUp;
                    *szW++ = upper ? L'X' : L'x';
                    if (--maxChars <= 0) goto cleanUp;
                    *szW++ = L'0';
                    prefix = FALSE;
                }
                // Now reverse the string in place
                Reverse(szPos, szW - 1);
                szPos = szW;
            } else {
                // Add the sign character
                if (sign) {
                    if (--maxChars <= 0) goto cleanUp;
                    *szW++ = L'-';
                    sign = FALSE;
                }
                if (prefix) {
                    if (--maxChars <= 0) goto cleanUp;
                    *szW++ = upper ? L'X' : L'x';
                    if (--maxChars <= 0) goto cleanUp;
                    *szW++ = L'0';
                    prefix = FALSE;
                }
                // Reverse the string in place
                Reverse(szPos, szW - 1);
                szPos = szW;
                // Pad to the right of the string in case left aligned
                while (width-- > 0) {
                    if (--maxChars <= 0) goto cleanUp;
                    *szPos++ = fillCh;
                }                    
            }
            break;

        case typeCh:
            // Depending on size obtain value
            if (mode == modeH) {
                ch = (WCHAR)va_arg(pArgList, CHAR);
            } else if (mode == modeL) {
                ch = va_arg(pArgList, WCHAR);
            } else {
                goto cleanUp;
            }
            if (--maxChars <= 0) goto cleanUp;
            *szPos++ = ch;
            break;

        case typeString:
            if (mode == modeH) {
                // It is ascii string
                szC = va_arg(pArgList, LPSTR);
                if (szC == NULL) szC = "(NULL)";
                // Get string size
                chars = 0;
                while (chars < maxChars && szC[chars] != '\0') chars++;
                // Fix string size
                if (precision >= 0 && chars > precision) chars = precision;
                width -= chars;
                if (padLeft) {
                    while (chars-- > 0) {
                        if (--maxChars <= 0) goto cleanUp;
                        *szPos++ = (WCHAR)*szC++;
                    }                        
                    while (width-- > 0) {
                        if (--maxChars <= 0) goto cleanUp;
                        *szPos++ = fillCh;
                    }                        
                } else {
                    while (width-- > 0) {
                        if (--maxChars <= 0) goto cleanUp;
                        *szPos++ = fillCh;
                    }                        
                    while (chars-- > 0) {
                        if (--maxChars <= 0) goto cleanUp;
                        *szPos++ = (WCHAR)*szC++;
                    }                        
                }
            } else if (mode == modeL) {
                // It is unicode string
                szW = va_arg(pArgList, LPWSTR);
                if (szW == NULL) szW = L"(NULL)";
                // Get string size
                chars = 0;
                while (chars < maxChars && szW[chars] != L'\0') chars++;
                // Fix string size
                if (precision >= 0 && chars > precision) chars = precision;
                width -= chars;
                if (padLeft) {
                    while (chars-- > 0) {
                        if (--maxChars <= 0) goto cleanUp;
                        *szPos++ = *szW++;
                    }                        
                    while (width-- > 0) {
                        if (--maxChars <= 0) goto cleanUp;
                        *szPos++ = fillCh;
                    }                        
                } else {
                    while (width-- > 0) {
                        if (--maxChars <= 0) goto cleanUp;
                        *szPos++ = fillCh;
                    }                        
                    while (chars-- > 0) {
                        if (--maxChars <= 0) goto cleanUp;
                        *szPos++ = *szW++;
                    }                        
                }
            } else {
                goto cleanUp;
            }
            break;
        }
            
    }

cleanUp:
    *szPos = L'\0';
    return (szPos - szBuffer);
}

//------------------------------------------------------------------------------

static VOID GetFormatValue(LPCWSTR *pszFormat, LONG *pWidth, va_list *ppArgList)
{
    LONG width = 0;
    
    if (**pszFormat == L'*') {
        *pWidth = va_arg(*ppArgList, int);
        (*pszFormat)++;
    } else {
        while (**pszFormat >= L'0' && **pszFormat <= L'9') {
            width = width * 10 + (**pszFormat - L'0');
            (*pszFormat)++;
        }
        *pWidth = width;
    }
}

//------------------------------------------------------------------------------

static VOID Reverse(WCHAR *pFirst, WCHAR *pLast) 
{
    INT swaps;
    WCHAR ch;

    swaps = (pLast - pFirst + 1) >> 1;
    while (swaps--) {
        ch = *pFirst;
        *pFirst++ = *pLast;
        *pLast-- = ch;
    }
}

//------------------------------------------------------------------------------

