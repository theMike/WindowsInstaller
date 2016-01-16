#include "StdAfx.h"
#include "MsiSession.h"
/*
The MIT License (MIT)

Copyright (c) 2014 Michael Presutti

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

MsiSession::MsiSession(void)
{
	hinstall = NULL;
	lastvalue.clear();
}
MsiSession::MsiSession(MSIHANDLE hInstall)
{
	hinstall = hInstall;
	lastvalue.clear();
}

MsiSession::~MsiSession(void)
{

}

wstring MsiSession::GetProperty(LPCTSTR szMsiProperty)
{
	DWORD cchValBuf=0;
	LPTSTR szValBuf = NULL;

	UINT uiStat = MsiGetProperty(hinstall,szMsiProperty,L"",&cchValBuf);
	//cchValBuf should now be the properties string length
	if(ERROR_MORE_DATA == uiStat)
	{
		++cchValBuf; //dont forget add 1 for '\0'
		szValBuf = new TCHAR[cchValBuf];
		if(szValBuf)
		{
			uiStat = MsiGetProperty(hinstall, szMsiProperty,szValBuf,&cchValBuf);
			nLastError = ERROR_SUCCESS;
		}
	}
	if(ERROR_SUCCESS != uiStat)
	{
		if(szValBuf != NULL)
			delete[] szValBuf;
		nLastError = ERROR_INSTALL_FAILURE;
	}
	else
	{
		lastvalue.assign(szValBuf);
	}
	return lastvalue;
}

wstring MsiSession::GetProperty(wstring& sMsiProperty)
{
	return GetProperty(sMsiProperty.c_str());
}

BOOL MsiSession::SetProperty(LPCTSTR szMsiProperty, LPCTSTR szVal)
{
	nLastError = MsiSetProperty(hinstall,szMsiProperty,szVal);

	if(ERROR_SUCCESS == nLastError)
		return TRUE;
	else
		return FALSE;
}

BOOL MsiSession::SetProperty(LPCTSTR szMsiPorpeorty, const wstring& sVal)
{
	return SetProperty(szMsiPorpeorty, sVal.c_str());
}

BOOL MsiSession::AddPropertyMap(wstring& sMsiProperty,map<wstring,wstring>& mProperties)
{
	BOOL bret=FALSE;

	mProperties.insert(pair<wstring,wstring>(sMsiProperty,GetProperty(sMsiProperty)));

	return bret;
}

BOOL MsiSession::AddPropertyMap(LPCTSTR szMsiProperty,map<wstring,wstring>& mProperties)
{
	BOOL bret=FALSE;
	wstring propLabel(szMsiProperty);
	mProperties.insert(pair<wstring,wstring>(propLabel,GetProperty(propLabel)));

	return bret;
}

void MsiSession::SetPropertyMap(LPCTSTR szMsiProperty,wstring& sVal, map<wstring,wstring>& mProperties)
{
	wstring propLabel(szMsiProperty);
	SetProperty(szMsiProperty,sVal);
	mProperties[szMsiProperty]=sVal;
}
