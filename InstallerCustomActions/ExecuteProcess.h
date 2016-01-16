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
#include <Windows.h>
#include <tchar.h>
#include <assert.h>
#define PROCESS_TIMEOUT 2000

class ExecuteProcess
{
private:
	DWORD m_dwExcTimeout;
public:
	ExecuteProcess(void);
	~ExecuteProcess(void);

	HANDLE m_childProcess;
	HANDLE m_stdOutRead;
	HANDLE m_stdInWrite;

	HANDLE m_hEvntStop;
	HANDLE m_hThread;
	DWORD m_dwThreadId;
	DWORD m_dwWaitTime;

	LPSTR m_StdOutString;
	LPCTSTR m_UserName;
	LPCTSTR m_Password;
	LPCTSTR m_Domain;
	BOOL m_UseLogonUser;

	DWORD m_lastReturnCode;
	char m_Output[1024*5];
	HANDLE m_hChildProcess;

	BOOL LaunchConsole(LPTSTR,
		HANDLE hStdOut,
		HANDLE hStdIn,
		HANDLE hStdErr);

	BOOL LaunchConsoleAsUser(LPTSTR,
		HANDLE hStdOut,
		HANDLE hStdIn,
		HANDLE hStdErr);

	VOID SetLogonUser(LPCTSTR lpszUser, LPCTSTR lpszDomain, LPCTSTR lpszPassWord);

	VOID SetProcessTimeout(DWORD dwTimeOut);

	BOOL Open(LPTSTR pszCmdLine);
	virtual DWORD Execute(LPTSTR pszCmdLine);

	static DWORD WINAPI OutputThread(LPVOID lpvThreadParam);
	VOID DestroyHandle(HANDLE& rhObject);
	VOID Close();
	DWORD GetExitCode()
	{ return m_lastReturnCode; }

	INT RedirectStdout();
	// overrides:
	virtual void WriteStdOut(LPCSTR pszOutput)  = 0;
	virtual void WriteStdError(LPCSTR pszError) = 0;

};

	