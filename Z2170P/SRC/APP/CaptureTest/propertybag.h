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


#ifndef PROPERTYBAG_H 
#define PROPERTYBAG_H

struct VAR_LIST
{
    VARIANT var;
    VAR_LIST *pNext;
    BSTR pBSTRName;
};

class CPropertyBag : public IPropertyBag
{  
public:
    CPropertyBag();
    ~CPropertyBag();
    
    HRESULT STDMETHODCALLTYPE
    Read(
        LPCOLESTR pszPropName, 
        VARIANT *pVar, 
        IErrorLog *pErrorLog
        );
    
    
    HRESULT STDMETHODCALLTYPE
    Write(
        LPCOLESTR pszPropName, 
        VARIANT *pVar
        );
        
    ULONG STDMETHODCALLTYPE AddRef();        
    ULONG STDMETHODCALLTYPE Release();        
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);   

private:
     ULONG _refCount;
     VAR_LIST *pVar;
};

#endif
