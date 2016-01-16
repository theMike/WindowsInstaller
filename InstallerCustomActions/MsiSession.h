#pragma once
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
#include "stdafx.h"
#include <tchar.h>
#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <string> 
#include <map>

#define TMAX_PATH 1024
#define WIN_MAX_PATH 1024 * 32
#pragma comment(lib, "msi.lib") 

using namespace std;

class MsiSession
{
private:
	MSIHANDLE hinstall;
	UINT nLastError;
	wstring lastvalue;

public:
	MsiSession(void);
	MsiSession(MSIHANDLE hInstall);
	~MsiSession(void);

public:
	wstring GetProperty(LPCTSTR szMsiProperty);
	wstring GetProperty(wstring& sMsiProperty);
	BOOL SetProperty(LPCTSTR szMsiProperty, LPCTSTR szVal);
	BOOL SetProperty(LPCTSTR szMsiPorpeorty, const wstring& sVal);
	BOOL AddPropertyMap(wstring& sMsiProperty,map<wstring,wstring>& mProperties); 
	BOOL AddPropertyMap(LPCTSTR szMsiProperty,map<wstring,wstring>& mProperties);
	void SetPropertyMap(LPCTSTR szMsiProperty,wstring& sVal, map<wstring,wstring>& mProperties);

};


