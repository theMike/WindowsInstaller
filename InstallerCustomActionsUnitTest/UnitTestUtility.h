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
#pragma once

#include <windows.h>
#include <string>
#include <Msi.h>
#include <MsiQuery.h>
#include <MsiDefs.h>
#include <wtypes.h>
#include "..\InstallerCustomActions\ExecuteProcess.h"
#include "..\InstallerCustomActions\InstallerCustomActions.h"
#include "..\InstallerCustomActions\InstallerCustomActionsUtility.h"

using namespace std;

class MSIExecuteProcessUnitTest : public ExecuteProcess
{
public:
	wstring proc_stdout;
	wstring proc_stderr;
	//These methods will be writing to the MSI log.
public:
	MSIExecuteProcessUnitTest(void)
	{
		//hInstall = hMsi;
	}
	void WriteStdOut(LPCSTR pszMSIoutput)
	{
		size_t ssize = strlen(pszMSIoutput)+1;
		wchar_t* wBuf = new wchar_t[ssize];
		MultiByteToWideChar(CP_ACP,0,pszMSIoutput,-1,wBuf,(int)ssize);
		//WriteMSILog(hInstall, wBuf);
		proc_stdout.append(wBuf);
		delete []wBuf;
		return;
	}
	void WriteStdError(LPCSTR pszMSIError)
	{
		size_t ssize = strlen(pszMSIError)+1;
		wchar_t* wBuf = new wchar_t[ssize];
		MultiByteToWideChar(CP_ACP,0,pszMSIError,-1,wBuf,(int)ssize);
		proc_stderr.append(wBuf);
		delete []wBuf;
		return;
	}
};


class MSIUnit
{
public:
	MSIHANDLE hdb;
	MSIHANDLE hsummary;
	MSIHANDLE hproduct;

	TCHAR msifilename[MAX_PATH];
	HMODULE hc;
	wstring strActionData;
	HMODULE hca;
	wchar_t handle[12];

	MSIUnit(void)
	{
		hdb = NULL;
		hproduct = NULL;
		GetTempFileName(L".",L"MSIUnit",0,msifilename);
		CreateMSIFile();
	}

	~MSIUnit(void)
	{
		FreeLibrary(hca);
	}

	void SetActionData(LPCTSTR szActionData)
	{
		MsiSetProperty(hproduct, L"CustomActionData",szActionData);
	}
	void SetMsiProperty(LPCTSTR szProp, LPCTSTR szVal)
	{
		MsiSetProperty(hproduct, szProp,szVal);
	}

	void CreateMSIFile(void)
	{
		MsiOpenDatabase(msifilename,MSIDBOPEN_CREATEDIRECT, &hdb);

		MsiGetSummaryInformation(hdb, NULL, 7, & hsummary);
		MsiSummaryInfoSetPropertyA(hsummary, PID_REVNUMBER, VT_LPSTR, 0, NULL, "{00000000-0000-0000-0000-000000000000}");
		MsiSummaryInfoSetPropertyA(hsummary, PID_SUBJECT, VT_LPSTR, 0, NULL, "Test MSI");
		MsiSummaryInfoSetPropertyA(hsummary, PID_TITLE, VT_LPSTR, 0, NULL, "Test MSI");
		MsiSummaryInfoSetPropertyA(hsummary, PID_AUTHOR, VT_LPSTR, 0, NULL, "dB.");
		MsiSummaryInfoSetPropertyA(hsummary, PID_TEMPLATE, VT_LPSTR, 0, NULL, ";1033");
		MsiSummaryInfoSetProperty(hsummary, PID_PAGECOUNT, VT_I4, 100, NULL, NULL);
		MsiSummaryInfoSetProperty(hsummary, PID_WORDCOUNT, VT_I4, 100, NULL, NULL);
		// persiste the summary in the stream
		MsiSummaryInfoPersist(hsummary);
		MsiCloseHandle(hsummary);
		// commit changes to disk
		MsiDatabaseCommit(hdb);
		_snwprintf(handle, ARRAYSIZE(handle), L"#%d", (UINT)hdb);
		MsiOpenPackage(handle, &hproduct);
	}

	int CallMSICA(LPCTSTR szCustomActionDll, LPCSTR szCustomActionFunction)
	{

		hca = LoadLibrary(szCustomActionDll);
		typedef int (__stdcall * LPCUSTOMACTION) (MSIHANDLE h);
		LPCUSTOMACTION lpca = (LPCUSTOMACTION) GetProcAddress(hca, szCustomActionFunction);
		//Set CustomActionData if we have 

		int nret =lpca(hproduct);

		//FreeLibrary(hca);
		MsiCloseHandle(hproduct);
		return nret;

	}

};


//LPCTSTR MSIUnit::msifilename = L"test.msi";
