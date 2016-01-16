
#include "stdafx.h"
#include "ExecuteProcess.h"

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

ExecuteProcess::ExecuteProcess(void):m_hThread(NULL),m_dwThreadId(0),m_dwWaitTime(100),
	m_childProcess(NULL),m_stdOutRead(NULL),m_stdInWrite(NULL),m_hEvntStop(NULL),
	m_hChildProcess(NULL),m_StdOutString(NULL),m_UseLogonUser(FALSE),m_UserName(NULL),
	m_Password(NULL),m_Domain(NULL),m_dwExcTimeout(INFINITE)
{	ZeroMemory(&m_Output,sizeof(m_Output)); }

ExecuteProcess::~ExecuteProcess(void)
{
}

BOOL ExecuteProcess::LaunchConsole(LPTSTR pszCmdline, HANDLE hStdOut, HANDLE hStdIn, HANDLE hStdErr)
{
	BOOL bret;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	assert( pszCmdline != NULL);

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.hStdError = hStdErr;
	si.hStdInput = hStdIn;
	si.hStdOutput = hStdOut;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

	if(!::CreateProcess(NULL,
		(LPTSTR)pszCmdline,
		NULL,
		NULL,
		TRUE,
		NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE|CREATE_UNICODE_ENVIRONMENT,
		NULL,
		NULL,
		&si,
		&pi))
		bret = FALSE;
	else
	{
		m_childProcess = pi.hProcess;
		bret = TRUE;
	}
	m_childProcess = pi.hProcess;
	
	for(int i=0; i<5; i++)
	{
		::GetExitCodeProcess(pi.hProcess, &m_lastReturnCode);
		if(m_lastReturnCode == STILL_ACTIVE)
			Sleep(PROCESS_TIMEOUT);
		else
			break;
	}
	if(pi.hThread != INVALID_HANDLE_VALUE && pi.hThread != NULL)
		CloseHandle(pi.hThread);
	return bret;
}

BOOL ExecuteProcess::LaunchConsoleAsUser(LPTSTR pszCmdline, HANDLE hStdOut, HANDLE hStdIn, HANDLE hStdErr)
{
	BOOL bret;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	assert( pszCmdline != NULL);

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.hStdError = hStdErr;
	si.hStdInput = hStdIn;
	si.hStdOutput = hStdOut;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	if(!::CreateProcessWithLogonW(m_UserName,m_Domain,m_Password,LOGON_WITH_PROFILE,NULL,
		(LPTSTR)pszCmdline,
		CREATE_UNICODE_ENVIRONMENT,
		NULL,
		NULL,
		&si,
		&pi))
		bret = FALSE;
	else
	{
		bret = TRUE;
	}
	m_childProcess = pi.hProcess;
	for(int i=0; i<5; i++)
	{
		::GetExitCodeProcess(pi.hProcess, &m_lastReturnCode);
		if(m_lastReturnCode == STILL_ACTIVE)
			Sleep(PROCESS_TIMEOUT);
		else
			break;
	}

	CloseHandle(pi.hThread);
	return bret;
}

//Basic run process method
BOOL ExecuteProcess::Open(LPTSTR pszCmdLine)
{
		
	HANDLE hStdOutReadTemp;					//parent stdout read handle
	HANDLE hStdOutWrite, hStdErrorWrite;	//child stdout write handle
	HANDLE hStdInWriteTemp;					//parent stdin write handle
	HANDLE hStdInRead;						//child stdin read handle
	SECURITY_ATTRIBUTES sa;
	BOOL bRet = FALSE;

	Close();
	hStdOutReadTemp=NULL;
	hStdOutWrite=NULL;
	hStdErrorWrite=NULL;
	hStdInWriteTemp=NULL;
	hStdInRead=NULL;
	sa.nLength=sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	HANDLE currentProcess = GetCurrentProcess();

	assert(pszCmdLine != NULL);

	if(CreatePipe(&hStdOutReadTemp,&hStdOutWrite, &sa, 0))
	{
		if(DuplicateHandle(	currentProcess,hStdOutWrite,currentProcess,	&hStdErrorWrite, 0, TRUE, DUPLICATE_SAME_ACCESS))
		{
			if(CreatePipe(&hStdInRead, &hStdInWriteTemp, &sa, 0))
			{
				if(DuplicateHandle(currentProcess,hStdOutReadTemp,currentProcess,&m_stdOutRead,0, FALSE, DUPLICATE_SAME_ACCESS))
				{
						if(DuplicateHandle(currentProcess,hStdInWriteTemp,currentProcess,&m_stdInWrite,	0,FALSE,DUPLICATE_SAME_ACCESS))
						{
							DestroyHandle(hStdOutReadTemp);
							DestroyHandle(hStdInWriteTemp);

							assert(pszCmdLine	!= NULL);
							assert(hStdOutWrite != NULL);
							assert(hStdInRead	!= NULL);

							if(!m_UseLogonUser)
								LaunchConsole(pszCmdLine, hStdOutWrite, hStdInRead, hStdErrorWrite);
							else
								LaunchConsoleAsUser(pszCmdLine, hStdOutWrite, hStdInRead, hStdErrorWrite);
								
							DestroyHandle(hStdOutWrite);
							DestroyHandle(hStdInRead);
							DestroyHandle(hStdErrorWrite);

							m_hEvntStop = CreateEvent(NULL, TRUE, FALSE, NULL);
							m_hThread = CreateThread(
								NULL, 0, 
								OutputThread,
								this,
								0,
								&m_dwThreadId);
							if(!m_hThread)
							{
								bRet=FALSE;
								return bRet;
							}
										
							bRet = TRUE;
						}
				}
			}
		}
	}
	return bRet;
}

VOID ExecuteProcess::SetProcessTimeout(DWORD dwTimeOut)
{
	if(dwTimeOut == 0 || dwTimeOut < 0)
		m_dwExcTimeout=INFINITE;
	else
		m_dwExcTimeout=dwTimeOut;
}

DWORD ExecuteProcess::Execute(LPTSTR pszCmdLine)
{
	DWORD bret = ERROR_PROCESS_ABORTED;
	DWORD dwReturnEvent;

	if(NULL != pszCmdLine)
	{
		if(Open(pszCmdLine))
		{
			HANDLE objList[2];
			DWORD dwListSize = sizeof(objList)/sizeof(HANDLE);
			objList[0] = m_childProcess;
			objList[1] = m_hThread;

			dwReturnEvent = WaitForMultipleObjects(dwListSize,objList,TRUE,m_dwExcTimeout);
			
			switch(dwReturnEvent)
			{
			case WAIT_OBJECT_0:
				GetExitCodeProcess(objList[0],  &m_lastReturnCode);
				//Test to make sure the process has properly ended
				//before attempting to get its error code.
				for(int i=0; i<5; i++)
				{
					::GetExitCodeProcess(objList[0], &m_lastReturnCode);
					if(m_lastReturnCode == STILL_ACTIVE)
						Sleep(PROCESS_TIMEOUT);
					else
						break;
				}
				//In the case of batch files that do not set error level. 
				//They will return NO MORE DATA error which can be 
				//a success
				if(ERROR_NO_MORE_ITEMS == m_lastReturnCode)
				{
					m_lastReturnCode=0;
				}
				bret = m_lastReturnCode;
			default:
				//something very bad happened here and we should log it.
				bret = ERROR_PROCESS_ABORTED;
			}
		}
	}
	return bret;
}

VOID ExecuteProcess::DestroyHandle(HANDLE& rhObject)
{
	if (rhObject != NULL)
	{
		::CloseHandle(rhObject);
		rhObject = NULL;
	}
}

VOID ExecuteProcess::Close()
{
	if (m_hThread != NULL)
	{
		// this function might be called from redir thread
		if (GetCurrentThreadId() != m_dwThreadId)
		{
			//ASSERT(m_hEvntStop != NULL);
			SetEvent(m_hEvntStop);
			//::WaitForSingleObject(m_hThread, INFINITE);
			if (WaitForSingleObject(m_hThread, 5000) == WAIT_TIMEOUT)
			{
				WriteStdError("The redir thread is dead\r\n");
				TerminateThread(m_hThread, ERROR_THREAD_1_INACTIVE);
			}
		}

		DestroyHandle(m_hThread);
	}

	DestroyHandle(m_hEvntStop);
	DestroyHandle(m_childProcess);
	DestroyHandle(m_stdInWrite);
	DestroyHandle(m_stdOutRead);
	m_dwThreadId = 0;
}

/*

return: 1: no more data, 0: child terminated, -1: os error

*/
INT ExecuteProcess::RedirectStdout()
{
	assert(m_stdOutRead != NULL);
	BOOL bContinue = TRUE;
	while (bContinue)
	{
		DWORD dwAvail = 0;
		if (!PeekNamedPipe(m_stdOutRead, NULL, 0, NULL,
			&dwAvail, NULL))			// Error
			break;

		if (!dwAvail)					// Not data available
			return 1;

		char szOutput[513]={0};
		DWORD dwRead = 0;
		if (!ReadFile(m_stdOutRead, szOutput, min(512, dwAvail),
			&dwRead, NULL) || !dwRead)	
			break;

		strcpy_s(m_Output,szOutput);
		szOutput[dwRead] = 0;
		WriteStdOut(szOutput);
	}

	DWORD dwError = GetLastError();
	if (dwError == ERROR_BROKEN_PIPE ||		// Pipe has been ended
		dwError == ERROR_NO_DATA ||			// Pipe closing in progress
		dwError == ERROR_NO_MORE_ITEMS)		
	{

		return 0;
	}

	WriteStdError("Read stdout pipe error\r\n");
	return -1;		
}

// Thread used to receive output of the child process
DWORD WINAPI ExecuteProcess::OutputThread(LPVOID lpvThreadParam)
{
	HANDLE aHandles[2];
	int nRet;
	ExecuteProcess* pRedir = (ExecuteProcess*) lpvThreadParam;

	assert(pRedir != NULL);
	aHandles[0] = pRedir->m_childProcess;
	aHandles[1] = pRedir->m_hEvntStop;

	for (;;)
	{
		// redirect stdout till no more data.
		nRet = pRedir->RedirectStdout();
		if (nRet <= 0)
			break;

		// check if child process has terminated.
		DWORD dwRc = WaitForMultipleObjects(2, aHandles, FALSE, pRedir->m_dwWaitTime);
		if (WAIT_OBJECT_0 == dwRc)		// the child process ended
		{
			nRet = pRedir->RedirectStdout();
			if (nRet > 0)
				nRet = 0;
			break;
		}
		if (WAIT_OBJECT_0+1 == dwRc)	// m_hEvtStop signalled
		{
			nRet = 1;	// Cancel
			break;
		}
	}
	// close handles
	pRedir->Close();
	return nRet;
}

VOID ExecuteProcess::SetLogonUser(LPCTSTR lpszUser, LPCTSTR lpszDomain, LPCTSTR lpszPassWord)
{
	assert(lpszUser != NULL);
	assert(lpszDomain != NULL);
	assert(lpszPassWord != NULL);

	m_UseLogonUser = TRUE;
	m_UserName = lpszUser;
	m_Domain = lpszDomain;
	m_Password = lpszPassWord;
}
