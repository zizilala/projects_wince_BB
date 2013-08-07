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

/*
    This file was obtained from http://www.codeproject.com/KB/windows/samplegrabberfilter-wm6.aspx.
    The site states it is licensed under "The Creative Commons Attribution-ShareAlike 2.5 License". 
    There were no licensing headers in the downloaded source.  You can view the license here:
    http://creativecommons.org/licenses/by-sa/2.5/
*/


#include <windows.h>
#include <Ocidl.h>
#include <oleauto.h>

#include "propertybag.h"

CPropertyBag::CPropertyBag() : _refCount(1), pVar(0)
{       
}

CPropertyBag::~CPropertyBag()
{
    VAR_LIST *pTemp = pVar;    
    
    while(pTemp)
    {
        VAR_LIST *pDel = pTemp;
        VariantClear(&pTemp->var);
        SysFreeString(pTemp->pBSTRName);
        pTemp = pTemp->pNext;
        delete pDel;
    }

}

HRESULT STDMETHODCALLTYPE
CPropertyBag::Read(LPCOLESTR pszPropName, 
                       VARIANT *_pVar, 
                       IErrorLog *pErrorLog)
{
	UNREFERENCED_PARAMETER(pErrorLog);

    VAR_LIST *pTemp = pVar;
    HRESULT hr = S_OK;
    
    while(pTemp)
    {
        if(0 == wcscmp(pszPropName, pTemp->pBSTRName))
        {
            hr = VariantCopy(_pVar, &pTemp->var);
            break;
        }
        pTemp = pTemp->pNext;
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE
CPropertyBag::Write(LPCOLESTR pszPropName, 
                            VARIANT *_pVar)
{    
    if( NULL == pszPropName || NULL == _pVar )
    {
        return E_POINTER;
    }
    

    VAR_LIST *pTemp = new VAR_LIST();    
    if( NULL == pTemp )
    {
        return E_OUTOFMEMORY;
    }

    VariantInit(&pTemp->var);
    pTemp->pBSTRName = SysAllocString(pszPropName);
    pTemp->pNext = pVar;
    pVar = pTemp;
    return VariantCopy(&pTemp->var, _pVar);
}

ULONG STDMETHODCALLTYPE 
CPropertyBag::AddRef() 
{
    return InterlockedIncrement((LONG *)&_refCount);
}

ULONG STDMETHODCALLTYPE 
CPropertyBag::Release() 
{
    ASSERT(_refCount != 0xFFFFFFFF);
    ULONG ret = InterlockedDecrement((LONG *)&_refCount);    
    if(!ret) 
        delete this; 
    return ret;
}

HRESULT STDMETHODCALLTYPE 
CPropertyBag::QueryInterface(REFIID riid, void** ppv) 
{
    if(!ppv) 
        return E_POINTER;
    if(riid == IID_IPropertyBag) 
        *ppv = static_cast<IPropertyBag*>(this);	
    else 
        return *ppv = 0, E_NOINTERFACE;
    
    return AddRef(), S_OK;	
}
