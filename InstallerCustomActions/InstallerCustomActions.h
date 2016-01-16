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
#include "stdafx.h"
#include <tchar.h>
#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <string> 
#include "ExecuteProcess.h"
#include "InstallerCustomActionsUtility.h"

#define TMAX_PATH 1024
#define WIN_MAX_PATH 1024 * 32
#pragma comment(lib, "msi.lib") 

extern "C" UINT __stdcall RunCmd(MSIHANDLE hInstall);

#define _CAHEADER_ L"< # Action Start #>"
#define _CAFOOTER_ L"< # Action End   #>"

class MSIExecuteProcess : public ExecuteProcess
{
	MSIHANDLE hInstall;
	//These methods will be writing to the MSI log.
	BOOL bEnableLogging;
	BOOL bEnableLogFilter;
	vector<wstring> filter_list;
	wstring strOutputString;

	list<wstring> buffer_window;

public:
	MSIExecuteProcess(MSIHANDLE hMsi):bEnableLogging(TRUE),bEnableLogFilter(FALSE)
	{
		hInstall = hMsi;
	}
	void EnableLogging(BOOL bLog)
	{
		bEnableLogging=bLog;
	}
	void SetFilterLogList(vector<wstring>& blacklist)
	{
		filter_list = blacklist;
		bEnableLogFilter=TRUE;
	}
	void AddFilterLogItem(wstring& blacklistItem)
	{
		filter_list.push_back(blacklistItem);
		bEnableLogFilter=TRUE;
	}
	wstring GetOutput(void)
	{
		size_t ssize = strlen(m_Output)+1;
		wchar_t* wBuf = new wchar_t[ssize];
		ZeroMemory(wBuf,ssize);
		MultiByteToWideChar(CP_ACP,0,m_Output,-1,wBuf,(int)ssize);
		strOutputString.assign(wBuf);
		delete []wBuf;
		return strOutputString;
	}

	void AddtoBuffer(LPCTSTR lpOutputString)
	{
		buffer_window.push_front(lpOutputString);
		buffer_window.resize(10);
	}

	void BufferString(wstring& sbuffer)
	{
		list<wstring>::iterator itr;

		for(itr=buffer_window.begin();itr != buffer_window.end();itr++)
		{
			sbuffer.append(*itr);
		}
		//remove any inserted \n from buffer as they interfere with
		//string search.
		sbuffer.erase(remove(sbuffer.begin(),sbuffer.end(), L'\n'),sbuffer.end());
		return;
	}

	BOOL FindSuccessString(vector<wstring>& success_strings)
	{
		vector<wstring>::iterator it_success;
		list<wstring>::iterator it_buffer;
		wstring stdbuffer;
		BufferString(stdbuffer);

		BOOL bSuccessRet = FALSE;

		for(it_success = success_strings.begin(); it_success != success_strings.end(); it_success++)
		{
			if(stdbuffer.find(*it_success,0) != wstring::npos)
			{
				bSuccessRet=TRUE;
			}
		}

		return bSuccessRet;
	}

	void WriteStdOut(LPCSTR pszMSIoutput)
	{
		if(bEnableLogging)
		{
			size_t ssize = strlen(pszMSIoutput)+1;
			wchar_t* wBuf = new wchar_t[ssize];
			MultiByteToWideChar(CP_ACP,0,pszMSIoutput,-1,wBuf,(int)ssize);
			if(bEnableLogFilter)
				WriteMSILogFiltered(hInstall,wBuf,filter_list);
			else
				WriteMSILog(hInstall, wBuf);

			AddtoBuffer(wBuf);
			delete []wBuf;
		}
		return;
	}
	void WriteStdError(LPCSTR pszMSIError)
	{
		if(bEnableLogging)
		{
			size_t ssize = strlen(pszMSIError)+1;
			wchar_t* wBuf = new wchar_t[ssize];
			MultiByteToWideChar(CP_ACP,0,pszMSIError,-1,wBuf,(int)ssize);
			WriteMSILog(hInstall, wBuf);
			delete []wBuf;
		}
		return;
	
	}
};

