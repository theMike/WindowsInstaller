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
#include "InstallerCustomActions.h"
#include "InstallerCustomActionsUtility.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <map>
#include "Shlobj.h"
#include "MsiSession.h"
#include "JavaPathBuilder.h"
#pragma comment(lib,"shell32.lib")

using namespace std;
wstring FormatLogMessage(const wchar_t* fmt, ... );

UINT __stdcall TestCustomAction(MSIHANDLE hInstall)
{
	MsiMessageBox(hInstall, _T("Hello From CA DLL"), 0, FALSE);
	::MSIExecuteProcess ep(hInstall);
	TCHAR szCmd[] = _T("cmd.exe /c dir c:\\windows");
	WriteMSILog(hInstall,_T(">> TestCustomAction :"));  
	WriteMSILog(hInstall,szCmd);  
	ep.Execute(szCmd);

	WriteMSILog(hInstall,_T("<< TestCustomAction :"));  

	return ERROR_SUCCESS;
}

UINT __stdcall ExecuteCommand(MSIHANDLE hInstall)
{
	//test action.
	TCHAR szCmd[TMAX_PATH]={0};
	DWORD dwCmd = TMAX_PATH;
	DWORD dwRet;
	//MsiMessageBox(hInstall, _T("CA ExecuteCommand"), 0, FALSE);
	WriteMSILog(hInstall,_T(">> ExecuteCommand :"));  
	MsiGetProperty(hInstall,_T("CustomActionData"),szCmd, &dwCmd);
	WriteMSILog(hInstall,szCmd);
	wstring teststr(L"testuser");

	::MSIExecuteProcess ep(hInstall);

	ep.m_UseLogonUser = FALSE;
	ep.SetLogonUser(L"testuser",L"CA145",L"CA123");
	ep.Execute(szCmd);

	dwRet=ep.GetExitCode();
	WriteMSILogEx(hInstall,_T(" Retcode %d "), dwRet);

	return AskContinueOnError(hInstall, dwRet);
}

extern "C" UINT __stdcall RunCmd(MSIHANDLE hInstall)
{
	TCHAR szCmd[TMAX_PATH] = {0};
	DWORD dwCmd = TMAX_PATH;
	DWORD dwRet=0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_); 	
	MsiGetProperty(hInstall,_T("CustomActionData"),szCmd, &dwCmd);
	WriteMSILog(hInstall,szCmd);
	WriteMSILogEx(hInstall,L"***\n Run Command: %s \n***\n",szCmd);
	MSIExecuteProcess ep(hInstall);
	ep.Execute(szCmd);
	WriteMSILogEx(hInstall,L"***\n Last exit code is: %d \n***\n", ep.GetExitCode());
	dwRet = AskContinueOnError(hInstall, ep.GetExitCode());
	WriteMSILog(hInstall,_CAFOOTER_); 
	return dwRet;
}

extern "C" UINT __stdcall RunCmdTimeOut(MSIHANDLE hInstall)
{
	/* make sure we have at least 2 items passed.
	0 - Timeout value for command
	1 - Command to execute
	*/
	TCHAR szCmd[TMAX_PATH] = {0};
	TCHAR szBuf[TMAX_PATH] = {0};
	DWORD dwCmd = TMAX_PATH;
	DWORD dwRet=0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_); 	
	MsiGetProperty(hInstall,_T("CustomActionData"),szCmd, &dwCmd);
	WriteMSILog(hInstall,szCmd);
	WriteMSILogEx(hInstall,L"***\n Run Command: %s \n***\n",szCmd);
	MSIExecuteProcess ep(hInstall);

	wstring strCmd(szCmd);
	wstring tok(L";");
	vector<wstring>cmdItems;
	split(strCmd, tok,cmdItems);
	assert(cmdItems.size() >= 2);

	ep.SetProcessTimeout(std::wcstoul(cmdItems[0].data(),NULL,10));
	::lstrcpyW(szBuf,cmdItems[1].c_str());
	ep.Execute(szBuf);
	WriteMSILogEx(hInstall,L"***\n Last exit code is: %d \n***\n", ep.GetExitCode());
	dwRet = AskContinueOnError(hInstall, ep.GetExitCode());
	WriteMSILog(hInstall,_CAFOOTER_); 
	return dwRet;
}

extern "C" UINT __stdcall RunCmdWithRetry(MSIHANDLE hInstall)
{
	TCHAR szCmd[TMAX_PATH] = {0};
	DWORD dwCmd = TMAX_PATH;
	DWORD dwRet=0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_T("<:::::::::::::: RunCmdWithRetry Start :::::::::::::::>")); 	
	MsiGetProperty(hInstall,_T("CustomActionData"),szCmd, &dwCmd);
	WriteMSILog(hInstall,szCmd);
	WriteMSILogEx(hInstall,L"***\n Run Command: %s \n***\n",szCmd);
	MSIExecuteProcess ep(hInstall);
	do{
		ep.Execute(szCmd);
		WriteMSILogEx(hInstall,L"***\n Last exit code is: %d \n***\n", ep.GetExitCode());
		dwRet = AskRetryOnErrorEx(hInstall, ep.GetExitCode(),ep.GetOutput().c_str());
	}while(dwRet == IDRETRY);
	WriteMSILog(hInstall,_T("<::::::::::::::  RunCmdWithRetry End ::::::::::::::>")); 
	return dwRet;
}

extern "C" UINT __stdcall RunLBCmdWithRetry(MSIHANDLE hInstall)
{
	TCHAR szCmd[TMAX_PATH] = {0};
	DWORD dwCmd = TMAX_PATH;
	DWORD dwRet=0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_); 	
	MsiGetProperty(hInstall,_T("CustomActionData"),szCmd, &dwCmd);
	WriteMSILog(hInstall,szCmd);
	WriteMSILogEx(hInstall,L"***\n Run Command: %s \n***\n",szCmd);
	vector<wstring> liquid_success;
	//Stdout string that determines a successful execution.
	liquid_success.push_back(L"Liquibase Update Successful");
	liquid_success.push_back(L"Successful");
	MSIExecuteProcess ep(hInstall);
	do{
		ep.Execute(szCmd);
		WriteMSILogEx(hInstall,L"***\n Last exit code is: %d \n***\n", ep.GetExitCode());
		DWORD retcode=ep.GetExitCode();
		//make sure liquid base exited successfully
		if(retcode == 0)
		{
			if(!ep.FindSuccessString(liquid_success))
				retcode = 1003; //ERROR_CAN_NOT_COMPLETE
		}
		dwRet = AskRetryOnErrorEx(hInstall,retcode ,ep.GetOutput().c_str());
	}while(dwRet == IDRETRY);
	WriteMSILog(hInstall,_CAFOOTER_); 
	return dwRet;
}

extern "C" UINT __stdcall RunLBCmdAsUserB64WithRetry(MSIHANDLE hInstall)
{
	TCHAR szCmd[WIN_MAX_PATH] = {0};
	TCHAR szBuf[TMAX_PATH] = {0};
	DWORD dwCmd = WIN_MAX_PATH;
	DWORD dwRet = 0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_);
	MsiGetProperty(hInstall, _T("CustomActionData"), szCmd, &dwCmd);
	wstring strCmd(szCmd);
	wstring tok(L";");
	vector<wstring>cmdItems;
	split(strCmd, tok,cmdItems);
	/* make sure we have at least 4 items passed.
	0 - Command to execute
	1 - Windows User
	2 - Windows Domain
	3 - Windows Password
	*/
	assert(cmdItems.size() >= 4);

	MSIExecuteProcess ep(hInstall);
	wstring decUser,decDomain,decPassword;
	vector<wstring> liquid_success;
	//Stdout string that determines a successful execution.
	liquid_success.push_back(L"Liquibase Update Successful");
	liquid_success.push_back(L"Successful");

	decUser= base64_decode_w(cmdItems[1]);
	//Right now the domain is not encoded. This needs to be fixed
	//decDomain = base64_decode_w(cmdItems[2]);
	decDomain = cmdItems[2];
	decPassword= base64_decode_w(cmdItems[3]);

	ep.SetLogonUser(decUser.c_str(), decDomain.c_str(), decPassword.c_str());
	::lstrcpyW(szBuf,cmdItems[0].c_str());

	do{
		DWORD retcode=ep.Execute(szBuf);
		if(ERROR_SUCCESS != retcode)
		{
			WriteMSILogEx(hInstall,L"##Could not execute: %s",szBuf);
			WriteMSILogEx(hInstall,L"##          As user: %s\\%s",decDomain.c_str(),decUser.c_str());
			WriteMSILog(hInstall,_T("##Verify that the credentials are valid##"));
		}
		//DWORD retcode=ep.GetExitCode();
		//make sure liquid base exited successfully
		if(retcode == 0)
		{
			if(!ep.FindSuccessString(liquid_success))
			{
				retcode = 1003; //ERROR_CAN_NOT_COMPLETE
				WriteMSILogEx(hInstall,L"##Could not execute: %s",szBuf);
				WriteMSILogEx(hInstall,L"##          As user: %s\\%s",decDomain.c_str(),decUser.c_str());
				WriteMSILog(hInstall,_T("##Verify that the credentials are valid##"));
			}
		}
		WriteMSILogEx(hInstall,L"###\n Last exit code is: %d \n###\n", ep.GetExitCode());
		dwRet = AskRetryOnErrorEx(hInstall, retcode,ep.GetOutput().c_str());
	}while(dwRet == IDRETRY);
	WriteMSILog(hInstall,_CAFOOTER_); 
	return dwRet;
}


extern "C" UINT __stdcall RunCmdWithRetryFiltered(MSIHANDLE hInstall)
{
	TCHAR szCmd[WIN_MAX_PATH] = {0};
	TCHAR szBuf[WIN_MAX_PATH] = {0};
	DWORD dwCmd = WIN_MAX_PATH;
	DWORD dwRet=0;
	/*
	make sure we have at least 2 items passed.
	0 - List (double comma separated list ,,) of strings to prevent from being logged when stdout is captured
	1 - Command to execute
	*/

	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_); 	
	MsiGetProperty(hInstall,_T("CustomActionData"),szCmd, &dwCmd);

	wstring strCmd(szCmd);
	wstring tok(L";");
	wstring black_tok(L",,");
	vector<wstring>cmdItems;
	vector<wstring>blackList;
	//split custom action data items
	split(strCmd, tok,cmdItems);
	assert(cmdItems.size() >= 2);
	//get black list strings
	split(cmdItems[0],black_tok,blackList);

	MSIExecuteProcess ep(hInstall);
	ep.SetFilterLogList(blackList);
	::lstrcpyW(szBuf,cmdItems[1].c_str());
	WriteMSILogEx(hInstall,L"***\n Run Command: %s \n***\n",szBuf);
	do{
		ep.Execute(szBuf);
		WriteMSILogEx(hInstall,L"***\n Last exit code is: %d \n***\n", ep.GetExitCode());
		dwRet = AskRetryOnErrorEx(hInstall, ep.GetExitCode(),ep.GetOutput().c_str());
	}while(dwRet == IDRETRY);
	WriteMSILog(hInstall,_CAFOOTER_); 
	return dwRet;
}

extern "C" UINT __stdcall RunCmdWithRetryTimeOut(MSIHANDLE hInstall)
{
	TCHAR szCmd[TMAX_PATH] = {0};
	TCHAR szBuf[TMAX_PATH] = {0};
	DWORD dwCmd = TMAX_PATH;
	DWORD dwRet=0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_); 	
	MsiGetProperty(hInstall,_T("CustomActionData"),szCmd, &dwCmd);
	WriteMSILog(hInstall,szCmd);
	WriteMSILogEx(hInstall,L"####\n Run Command: %s \n#####\n",szCmd);
	MSIExecuteProcess ep(hInstall);

	wstring strCmd(szCmd);
	wstring tok(L";");
	vector<wstring>cmdItems;
	split(strCmd, tok,cmdItems);
	assert(cmdItems.size() >= 2);

	ep.SetProcessTimeout(std::wcstoul(cmdItems[0].data(),NULL,10));
	::lstrcpyW(szBuf,cmdItems[1].c_str());
	
	do{
		ep.Execute(szBuf);
		WriteMSILogEx(hInstall,L"####\n Last exit code is: %d \n#####\n", ep.GetExitCode());
		dwRet = AskRetryOnErrorEx(hInstall, ep.GetExitCode(),ep.GetOutput().c_str());
	}while(dwRet == IDRETRY);
	WriteMSILog(hInstall,_CAFOOTER_); 
	return dwRet;
}

extern "C" UINT __stdcall RunCmdAsUser(MSIHANDLE hInstall)
{
	TCHAR szCmd[WIN_MAX_PATH] = {0};
	TCHAR szBuf[TMAX_PATH] = {0};
	DWORD dwCmd = WIN_MAX_PATH;
	DWORD dwRet = 0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_T("< ################## RunCmdAsUser Start ################# >"));

	MsiGetProperty(hInstall, _T("CustomActionData"), szCmd, &dwCmd);
	wstring strCmd(szCmd);
	wstring tok(L";");
	vector<wstring>cmdItems;
	split(strCmd, tok,cmdItems);
	/* make sure we have at least 4 items passed.
	0 - Command to execute
	1 - Windows User
	2 - Windows Domain
	3 - Windows Password
	*/
	assert(cmdItems.size() >= 4);

	MSIExecuteProcess ep(hInstall);
	ep.SetLogonUser(cmdItems[1].c_str(), cmdItems[2].c_str(), cmdItems[3].c_str());
	::lstrcpyW(szBuf,cmdItems[0].c_str());
	ep.Execute(szBuf);

	dwRet = AskContinueOnError(hInstall, ep.GetExitCode());
	WriteMSILog(hInstall,_T("< ################## RunCmdAsUser End ################# >")); 
	return dwRet;
}

extern "C" UINT __stdcall RunCmdAsUserB64(MSIHANDLE hInstall)
{
	TCHAR szCmd[WIN_MAX_PATH] = {0};
	TCHAR szBuf[TMAX_PATH] = {0};
	DWORD dwCmd = WIN_MAX_PATH;
	DWORD dwRet = 0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_T("< ################## RunCmdAsUserB64 Start ################# >"));
	MsiGetProperty(hInstall, _T("CustomActionData"), szCmd, &dwCmd);
	wstring strCmd(szCmd);
	wstring tok(L";");
	vector<wstring>cmdItems;
	split(strCmd, tok,cmdItems);
	/* make sure we have at least 4 items passed.
	0 - Command to execute
	1 - Windows User
	2 - Windows Domain
	3 - Windows Password
	*/
	assert(cmdItems.size() >= 4);

	MSIExecuteProcess ep(hInstall);
	wstring decUser,decDomain,decPassword;

	decUser= base64_decode_w(cmdItems[1]);
	//Right now the domain is not encoded. This needs to be fixed
	decDomain = cmdItems[2];
	decPassword= base64_decode_w(cmdItems[3]);

	ep.SetLogonUser(decUser.c_str(), decDomain.c_str(), decPassword.c_str());
	::lstrcpyW(szBuf,cmdItems[0].c_str());
	if(!ep.Execute(szBuf))
	{
		WriteMSILogEx(hInstall,L"**Could not execute: %s",szBuf);
		WriteMSILogEx(hInstall,L"**As user          : %s\\%s",decDomain.c_str(),decUser.c_str());
		WriteMSILog(hInstall,_T("**Verify that the credentials are valid**"));
	}

	dwRet = AskRetryOnErrorEx(hInstall, ep.GetExitCode(),ep.GetOutput().c_str());
	WriteMSILog(hInstall,_T("< ################## RunCmdAsUserB64 Stop ################# >")); 
	return dwRet;
}

extern "C" UINT __stdcall RunCmdAsUserB64WithRetry(MSIHANDLE hInstall)
{
	TCHAR szCmd[WIN_MAX_PATH] = {0};
	TCHAR szBuf[TMAX_PATH] = {0};
	DWORD dwCmd = WIN_MAX_PATH;
	DWORD dwRet = 0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_);
	//MsiMessageBox(hInstall, _T("CA ExecuteCommand"), 0, FALSE);
	MsiGetProperty(hInstall, _T("CustomActionData"), szCmd, &dwCmd);
	wstring strCmd(szCmd);
	wstring tok(L";");
	vector<wstring>cmdItems;
	split(strCmd, tok,cmdItems);
	/* make sure we have at least 4 items passed.
	0 - Command to execute
	1 - Windows User
	2 - Windows Domain
	3 - Windows Password
	*/
	assert(cmdItems.size() >= 4);

	MSIExecuteProcess ep(hInstall);
	wstring decUser,decDomain,decPassword;

	decUser= base64_decode_w(cmdItems[1]);
	//Right now the domain is not encoded. This needs to be fixed
	//decDomain = base64_decode_w(cmdItems[2]);
	decDomain = cmdItems[2];
	decPassword= base64_decode_w(cmdItems[3]);

	ep.SetLogonUser(decUser.c_str(), decDomain.c_str(), decPassword.c_str());
	::lstrcpyW(szBuf,cmdItems[0].c_str());

	do{
		if(ERROR_SUCCESS != ep.Execute(szBuf))
		{
			WriteMSILogEx(hInstall,L"##Could not execute: %s",szBuf);
			WriteMSILogEx(hInstall,L"##As user          : %s\\%s",decDomain.c_str(),decUser.c_str());
			WriteMSILog(hInstall,_T("##Verify that the credentials are valid**"));
		}
		WriteMSILogEx(hInstall,L"###\n Last exit code is: %d \n###\n", ep.GetExitCode());
		dwRet = AskRetryOnErrorEx(hInstall, ep.GetExitCode(),ep.GetOutput().c_str());
	}while(dwRet == IDRETRY);
	
	WriteMSILog(hInstall,_CAFOOTER_); 
	return dwRet;
}

extern "C" UINT __stdcall EncodeInitialUserData(MSIHANDLE hInstall)
{
	TCHAR szCmd[WIN_MAX_PATH] = {0};
	TCHAR szBuf[WIN_MAX_PATH] = {0};
	DWORD dwCmd = WIN_MAX_PATH;
	DWORD dwRet = 0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_);

	WriteMSILog(hInstall,_CAHEADER_);
	dwCmd = WIN_MAX_PATH;
	MsiGetProperty(hInstall, L"INITIAL_EMAIL", szBuf, &dwCmd);
	MsiSetProperty(hInstall, L"INITIAL_EMAIL_B64",base64_encode_w(szBuf).c_str());
	
	dwCmd = WIN_MAX_PATH;
	MsiGetProperty(hInstall, L"INITIAL_FIRST_USERNAME", szBuf, &dwCmd);
	MsiSetProperty(hInstall, L"INITIAL_FIRST_USERNAME_B64",base64_encode_w(szBuf).c_str());

	dwCmd = WIN_MAX_PATH;
	MsiGetProperty(hInstall, L"INITIAL_FIRST_USERNAME", szBuf, &dwCmd);
	MsiSetProperty(hInstall, L"INITIAL_FIRST_USERNAME_B64",base64_encode_w(szBuf).c_str());

	dwCmd = WIN_MAX_PATH;
	MsiGetProperty(hInstall, L"INITIAL_LAST_USERNAME", szBuf, &dwCmd);
	MsiSetProperty(hInstall, L"INITIAL_LAST_USERNAME_B64",base64_encode_w(szBuf).c_str());

	dwCmd = WIN_MAX_PATH;
	MsiGetProperty(hInstall, L"INITIAL_PASSWORD", szBuf, &dwCmd);
	MsiSetProperty(hInstall, L"INITIAL_PASSWORD_B64",base64_encode_w(szBuf).c_str());

	WriteMSILog(hInstall,_CAFOOTER_);
	return dwRet;
}

extern "C" UINT __stdcall CreateInitialUserB64WithRetry(MSIHANDLE hInstall)
{
	TCHAR szCmd[WIN_MAX_PATH] = {0};
	TCHAR szBuf[WIN_MAX_PATH] = {0};
	DWORD dwCmd = WIN_MAX_PATH;
	DWORD dwRet = 0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_);
	//MsiMessageBox(hInstall, _T("CA ExecuteCommand"), 0, FALSE);
	MsiGetProperty(hInstall, _T("CustomActionData"), szCmd, &dwCmd);
	wstring strCmd(szCmd);
	wstring tok(L";");
	vector<wstring>cmdItems;
	splitEx(strCmd, tok,cmdItems);
	/* make sure we have at least 14 items passed.
	0 - Java Runtime path
	1 - Nsislib path
	2 - User Name B64
	3 - Domain Name B64
	4 - User Password B64
	5 - DB Host name
	6 - DB Port
	7 - SQL User name B64
	8 - SQL Password B64
	9 - DB Authentication Type
	10- Initial email B64
	11- Initial first name B64
	12- Initial last name B64
	13- Initial password B64
	*/
	assert(cmdItems.size() >= 12);

	MSIExecuteProcess ep(hInstall);
	wstring decUser,decDomain,decPassword;
	wstring decSqlUser,decSqlPassword;
	wstring decInitEmail,decInitFirstName,decInitPassword;
	wstring javaCommandLine;
	
	//decode account informaton	
	decUser= base64_decode_w(cmdItems[2]);
	decDomain = cmdItems[3];
	decPassword= base64_decode_w(cmdItems[4]);
	
	javaCommandLine = BuildInitialUserCmdLine(cmdItems);
	//MsiMessageBox(hInstall,javaCommandLine.c_str(), 0, FALSE);
	if( 0 == cmdItems[9].compare(L"windowsAuth"))
	{
		ep.SetLogonUser(decUser.c_str(), decDomain.c_str(), decPassword.c_str());
	}
	::lstrcpyW(szBuf,javaCommandLine.c_str());

	do{
		ep.Execute(szBuf);
		if(ERROR_SUCCESS != ep.GetExitCode())
		{
			WriteMSILogEx(hInstall,L"**Could not execute: %s",szBuf);
			WriteMSILogEx(hInstall,L"**As user          : %s\\%s",decDomain.c_str(),decUser.c_str());
			WriteMSILog(hInstall,_T("**Verify that the credentials are valid**"));
		}
		WriteMSILogEx(hInstall,L"***\n Last exit code is: %d \n***\n", ep.GetExitCode());
		dwRet = AskRetryOnErrorEx(hInstall, ep.GetExitCode(),ep.GetOutput().c_str());
	}while(dwRet == IDRETRY);
	WriteMSILog(hInstall,_CAFOOTER_); 
	return dwRet;
}



extern "C" UINT __stdcall ExecutePOTInstallationB64(MSIHANDLE hInstall)
{
	TCHAR szCmd[WIN_MAX_PATH] = {0};
	TCHAR szBuf[WIN_MAX_PATH] = {0};

	TCHAR szPOTFilePassWord[WIN_MAX_PATH] = {0};

	DWORD dwCmd = WIN_MAX_PATH;
	DWORD dwRet = 0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_);
	MsiGetProperty(hInstall, _T("CustomActionData"), szCmd, &dwCmd);
	wstring strCmd(szCmd);
	wstring tok(L";");
	vector<wstring>cmdItems;
	split(strCmd, tok,cmdItems);
	/* make sure we have at least 4 items passed.
	0 - Java Runtime path
	1 - Nsislib path
	2 - POT file location
	3 - POT password
	4 - Core root directory
	*/
	assert(cmdItems.size() >= 4);
	//Get the POT_PASSWORD directly from the MSI to avoid any issues with parsing
	//special characters in CustomActionData. If the pot file validates then
	//we base64 encode it to pass to the POT configuration action

	//Trim trailing '\' becuase nsislib has a problem dealing with it.
	cmdItems[4] = choppa(cmdItems[4],L"\\");
	wstring java_cmd_line;
	
	nsis_cmd.append(L" ");
	nsis_cmd.append(cmdItems[2]);
	nsis_cmd.append(L" ");
	nsis_cmd.append(cmdItems[3]);
	nsis_cmd.append(L" \"");
	nsis_cmd.append(cmdItems[4]);
	nsis_cmd.append(L"\"");

	java_cmd_line = BuildJavaCmdLineEx(cmdItems[0].c_str(),cmdItems[1].c_str(),L"NSISLIB.JAR",nsis_cmd.c_str());
	WriteMSILog(hInstall,java_cmd_line.c_str()); 
	
	MSIExecuteProcess ep(hInstall);
	DWORD dwlen = (DWORD)java_cmd_line.length();
	TCHAR javacmd[WIN_MAX_PATH]={0};
	lstrcpy(javacmd,java_cmd_line.c_str());

	do{
		ep.Execute(javacmd);
		if(ERROR_SUCCESS != ep.GetExitCode())
		{
			WriteMSILogEx(hInstall,L"**Could not execute: %s",javacmd);
		}
		WriteMSILogEx(hInstall,L"***\n Last exit code is: %d \n***\n", ep.GetExitCode());
		dwRet = AskRetryOnErrorEx(hInstall, ep.GetExitCode(),ep.GetOutput().c_str());
	}while(dwRet == IDRETRY);
	WriteMSILog(hInstall,_CAFOOTER_); 
	return dwRet;
}

extern "C" UINT __stdcall ExecuteDMZPotVerify(MSIHANDLE hInstall)
{
	TCHAR szCmd[WIN_MAX_PATH] = {0};
	TCHAR szBuf[WIN_MAX_PATH] = {0};

	TCHAR szPOTFilePassWord[WIN_MAX_PATH] = {0};

	DWORD dwCmd = WIN_MAX_PATH;
	DWORD dwRet = 0;
	assert(hInstall != NULL);

	WriteMSILog(hInstall,_CAHEADER_);
	MsiGetProperty(hInstall, L"POT_PASSWORD", szBuf, &dwCmd);
	wstring pot_password_b64 = base64_encode_w(szBuf);
	//encode the password before passing it to nsislib
	MsiSetProperty(hInstall, L"POT_PASSWORD_B64",base64_encode_w(szBuf).c_str());
	dwCmd = WIN_MAX_PATH;
	MsiGetProperty(hInstall, L"POT_FILE_PATH", szBuf, &dwCmd);
	wstring pot_file_path(szBuf);
	dwCmd = WIN_MAX_PATH;
	MsiGetProperty(hInstall, L"NSISLIBTEMPATH", szBuf, &dwCmd);
	wstring nsislib_path(szBuf);

	dwCmd = WIN_MAX_PATH;
	MsiGetProperty(hInstall, L"jre_1_Dir", szBuf, &dwCmd);
	wstring java_dir(szBuf);
	wstring java_cmd_line;

	nsis_cmd.append(L" \"");
	nsis_cmd.append(pot_file_path);
	nsis_cmd.append(L"\" ");
	nsis_cmd.append(pot_password_b64);

	wstring nsislib(L"Nsislib.jar");

	java_cmd_line = BuildJavaCmdLine2(java_dir,nsislib_path,nsislib,nsis_cmd);
	MSIExecuteProcess ep(hInstall);

	lstrcpy(szBuf,java_cmd_line.c_str());
	if(ERROR_SUCCESS != ep.Execute(szBuf))
	{
		if(ep.GetExitCode() == 0)
			MsiSetProperty(hInstall,L"NSISLIBRET",L"SUCCESS");
		else
			MsiSetProperty(hInstall,L"NSISLIBRET",L"FAIL");
	}
	else
	{
		MsiSetProperty(hInstall,L"NSISLIBRET",L"FAIL");
	}

	return dwRet;
}

extern "C" UINT __stdcall SetErlangCookieToProp(MSIHANDLE hInstall)
{
	TCHAR szCmd[WIN_MAX_PATH] = {0};
	TCHAR szBuf[WIN_MAX_PATH] = {0};

	TCHAR szPOTFilePassWord[WIN_MAX_PATH] = {0};

	DWORD dwCmd = WIN_MAX_PATH;
	DWORD dwRet = 0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_);
	//MsiMessageBox(hInstall, _T("SetErlangCookieToProp"), 0, FALSE);
	MsiGetProperty(hInstall, _T("CustomActionData"), szCmd, &dwCmd);
	//MsiMessageBox(hInstall, szCmd, 0, FALSE);
	wstring strCmd(szCmd);
	wstring tok(L";");
	vector<wstring>cmdItems;
	split(strCmd, tok,cmdItems);
	/* make sure we have at least 4 items passed.
	0 - Java Runtime path
	1 - Nsislib path
	2 - POT file location
	3 - POT password
	3 - Core root directory
	*/
	assert(cmdItems.size() >= 2);
	wstring erlangcookie;
	TCHAR szHomeDirBuf[WIN_MAX_PATH] = { 0 };
	HANDLE hToken =0;
	DWORD BufSize = WIN_MAX_PATH;
	do{
		WriteMSILog(hInstall,cmdItems[0].c_str());
		WriteMSILog(hInstall,cmdItems[1].c_str());
		erlangcookie=update_property_file(cmdItems[0], cmdItems[1]);
		//MsiMessageBox(hInstall, erlangcookie.c_str(), 0, FALSE);
		if(erlangcookie.size()<1)
		{
			dwRet = AskRetryOnErrorEx(hInstall, 1,L"Could not set Erlang cookie in property file.");
		}
		else
			dwRet = ERROR_SUCCESS;
	}while(dwRet == IDRETRY);

	return dwRet;
}

extern "C" UINT __stdcall ReadInstallationPropertyFile(MSIHANDLE hInstall)
{
	DWORD dwRet=0;
	TCHAR szCmd[WIN_MAX_PATH] = {0};
	//TCHAR szBuf[WIN_MAX_PATH] = {0};

	DWORD dwCmd = WIN_MAX_PATH;

	WIN32_FIND_DATA FileFindData;
	HANDLE hFind;
	Properties props;
	assert(hInstall != NULL);
	map<wstring,wstring> propmap;
	map<wstring,wstring>::iterator mapiter;

	WriteMSILog(hInstall,_CAHEADER_);

	MsiGetProperty(hInstall, _T("CustomActionData"), szCmd, &dwCmd);
	wstring strCmd(szCmd);

	if(strCmd.length() > 1)
	{
		hFind = FindFirstFile(szCmd, &FileFindData);
		if (hFind != INVALID_HANDLE_VALUE) 
		{
			props.Read(strCmd);
			propmap = props.GetMap();
			for(mapiter = propmap.begin(); mapiter != propmap.end(); mapiter++)
			{
				MsiSetProperty(hInstall,mapiter->first.c_str(),mapiter->second.c_str());
			}
		}
	}

	WriteMSILog(hInstall,_CAFOOTER_);
	return dwRet;
}

extern "C" UINT __stdcall CopyAndCleanLog(MSIHANDLE hInstall)
{

	DWORD dwRet=ERROR_INSTALL_FAILURE;
	TCHAR szCmd[WIN_MAX_PATH] = {0};

	assert(hInstall != NULL);

	WriteMSILog(hInstall,_CAHEADER_);

	wstring strCmd(szCmd);
	wstring tok(L";");
	vector<wstring>cmdItems;
	split(strCmd, tok,cmdItems);
	/* make sure we have at least 4 items passed.
	0 - Install log temp dir path
	1 - App install path
	2 - Password properties to look for and clean
	*/
	wstring tempfile(cmdItems[1]);
	WriteMSILogEx(hInstall,L"LOG TempFile %s",tempfile.c_str());
	tempfile.append(L".cpy");
	vector<wstring> blacklist;
	if(cmdItems.size()>2)
	{
		CopyFile(cmdItems[0].c_str(),tempfile.c_str(),FALSE);
		MsiMessageBox(hInstall, _T("CA CopyAndCleanLog: Copy File"), 0, FALSE);
		blacklist.assign(cmdItems.begin()+2,cmdItems.end());
		MsiMessageBox(hInstall, tempfile.c_str(), 0, FALSE);
		MsiMessageBox(hInstall, cmdItems[1].c_str(), 0, FALSE);

		if(LogFileFilter(tempfile,cmdItems[1],blacklist))
		{
			MsiMessageBox(hInstall, _T("CA CopyAndCleanLog: LogFileFilter success!"), 0, FALSE);
			dwRet = ERROR_SUCCESS;
		}
		else
			MsiMessageBox(hInstall, _T("CA CopyAndCleanLog: LogFileFilter fail!"), 0, FALSE);
			
	}

	DeleteFile(tempfile.c_str());
	WriteMSILog(hInstall,_CAFOOTER_);
	return dwRet;
}
extern "C" UINT __stdcall TestMessageBox(MSIHANDLE hInstall)
{
	DWORD dwRet=0;
	do{
		dwRet = AskRetryOnErrorEx(hInstall, 2,L"Action start 11:40:03: AI_CleanPrereq.");
	}while(dwRet == IDRETRY);
	return dwRet;
}

extern "C" UINT __stdcall HashPasswords(MSIHANDLE hInstall)
{
	TCHAR szCmd[WIN_MAX_PATH] = {0};
	
	DWORD dwCmd = WIN_MAX_PATH;
	DWORD dwRet = 0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_);
	MsiGetProperty(hInstall, _T("CustomActionData"), szCmd, &dwCmd);
	wstring strCmd(szCmd);
	wstring tok(L";");
	vector<wstring>cmdItems;
	split(strCmd, tok,cmdItems);
	/* make sure we have at least 2 items passed.
	0 - Java Runtime path
	1 - Nsislib path
	*/

	assert(cmdItems.size() >= 2);
	//Select the installer properties that we want hashed.
	const LPCTSTR property_passwords[]={L"USER_PASSWORD",
										L"USER_PASSWORD_B64",
										L"SQL_PASSWORD",
										L"SQL_PASSWORD_B64",                  
										L"BS_KEYSTORE_PASSWORD",
										L"DC_BOOTSTRAP_KEYSTORE_PASSWORD",
										L"EDS_BOOTSTRAP_KEYSTORE_PASSWORD",
										L"MC_BOOTSTRAP_KEYSTORE_PASSWORD",
										L"MNR_BOOTSTRAP_KEYSTORE_PASSWORD",
										L"PS_BOOTSTRAP_KEYSTORE_PASSWORD",
										L"KEYSTORE_PASSWORD",
										L"SZ_KEYSTORE_PASSWORD",
										L"RBMQ_PASSWORD",
										L"BOOTSTRAP_SHARED_KEY",
										L"BBIS_BOOTSTRAP_KEYSTORE_PASSWORD",
										L"BES_BOOTSTRAP_KEYSTORE_PASSWORD",
										L"API_BOOTSTRAP_KEYSTORE_PASSWORD",
										L"API_KEYSTORE_PASSWORD"};

	//create an initialized vector of properties we want to hash
	vector<wstring> plain_property_passwords(property_passwords,arrayend(property_passwords));
	vector<wstring>::const_iterator it;

	for(it=plain_property_passwords.begin();it != plain_property_passwords.end(); it++)
	{
		TCHAR szBuf[WIN_MAX_PATH] = {0};
		DWORD dwBuf = WIN_MAX_PATH;
		wstring hashedtext;
		MsiGetProperty(hInstall, (*it).c_str(), szBuf, &dwBuf);
		if(Hasher(cmdItems[0].c_str(),cmdItems[1].c_str(),szBuf,hashedtext,hInstall))
		{
			wstring hashed_property((*it).c_str());
			hashed_property.append(L"_HASH");
			WriteMSILogEx(hInstall,L"Hashing: %s : %s \n",hashed_property.c_str(),hashedtext.c_str());

			MsiSetProperty(hInstall,hashed_property.c_str(),hashedtext.c_str());
		}
		else
		{
			WriteMSILogEx(hInstall,L"Error Could not Hash: %s : %s \n",(*it).c_str(),hashedtext.c_str());
		}

	}
	WriteMSILog(hInstall,_CAFOOTER_);
	return dwRet;
}

extern "C" UINT __stdcall InstallSleep(MSIHANDLE hInstall)
{
	TCHAR szCmd[MAX_PATH] = {0};
	DWORD dwCmd = MAX_PATH;
	DWORD dwRet = 0;
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_);
	MsiGetProperty(hInstall, _T("CustomActionData"), szCmd, &dwCmd);

	WriteMSILogEx(hInstall,L"Sleep for: %d milliseconds \n",szCmd);
	Sleep((DWORD)_wcstoui64(szCmd,NULL,10));
	WriteMSILog(hInstall,_CAFOOTER_);
	return dwRet;
}



extern "C" UINT __stdcall FixServiceAccountProperties(MSIHANDLE hInstall)
{
	DWORD dwRet = ERROR_SUCCESS;
	//Initialize properties we that will be used in the Installer UI.
	//We want to initializie them to NULL which is illegal by default in 
	//MSI so we need to achive this through a CA
	//
	TCHAR useServiceAccount[MAX_PATH]={0};
	TCHAR dbAuthType[MAX_PATH]={0};
	TCHAR szDomainName[MAX_PATH]={0};
	TCHAR szUserName[MAX_PATH]={0};
	DWORD dwSize=MAX_PATH;
	WriteMSILog(hInstall,_CAHEADER_);

	MsiGetProperty(hInstall,L"USE_SERVICE_ACCOUNT",useServiceAccount,&dwSize);
	dwSize=MAX_PATH;
	MsiGetProperty(hInstall,L"DB_AUTH_TYPE",dbAuthType,&dwSize);
	if(useServiceAccount[0] != NULL)
	{
		if(wcscmp(useServiceAccount,L"LocalSystem") == 0)
		{
			//MsiMessageBox(hInstall, _T("LocalSystem"), 0, FALSE);
			//initialize properties needed for a Local System Account
			MsiSetProperty(hInstall,L"DOMAIN_NAME",L"");
			MsiSetProperty(hInstall,L"USER_NAME",L"");
			MsiSetProperty(hInstall,L"USER_PASSWORD",L"");
			MsiSetProperty(hInstall,L"WRAPPER_SERVICE_ACCOUNT",L"");
		}
		else if(wcscmp(useServiceAccount,L"ServiceAccount") == 0)
		{
			//MsiMessageBox(hInstall, _T("ServiceAccount"), 0, FALSE);
		
			MsiGetProperty(hInstall,L"DOMAIN_NAME",szDomainName,&dwSize);
			dwSize=MAX_PATH;
			MsiGetProperty(hInstall,L"USER_NAME",szUserName,&dwSize);
			TCHAR szWrapperAccount[MAX_PATH];
			//For the WRAPPER_SERVICE_ACCOUNT we use the old NetBIOS domain 
			//name
			wstring sdomain(szDomainName);
			wstring swrapper=get_host_from_fqdn(sdomain);
			lstrcpy(szDomainName,swrapper.c_str());
			wsprintf(szWrapperAccount,L"%s\\%s",swrapper.c_str(),szUserName);
			MsiSetProperty(hInstall,L"WRAPPER_SERVICE_ACCOUNT",szWrapperAccount);
			dwSize=MAX_PATH;
			MsiGetProperty(hInstall,L"DOMAIN_NAME",szDomainName,&dwSize);
		}
	}	
	if(dbAuthType[0] != NULL)
	{
		if(wcscmp(dbAuthType,L"windowsAuth") == 0)
		{

			MsiGetProperty(hInstall,L"DOMAIN_NAME",szDomainName,&dwSize);
			dwSize=MAX_PATH;
			MsiGetProperty(hInstall,L"USER_NAME",szUserName,&dwSize);
			TCHAR szWrapperAccount[MAX_PATH];
			wsprintf(szWrapperAccount,L"%s\\%s",szDomainName,szUserName);
			MsiSetProperty(hInstall,L"DB_USE_WINDOWS_AUTH",L"true");
			//MsiSetProperty(hInstall,L"USER_PASSWORD_B64",L"");
			MsiSetProperty(hInstall,L"SQL_USERNAME",L"");
			MsiSetProperty(hInstall,L"SQL_PASSWORD",L"");
			//MsiSetProperty(hInstall,L"SQL_PASSWORD_B64",L"");
		}
	
		if(wcscmp(dbAuthType,L"sqlAuth") == 0)
		{
			MsiSetProperty(hInstall,L"DB_USE_WINDOWS_AUTH",L"false");
		}
		else
		{
			MsiSetProperty(hInstall,L"SQL_PASSWORD",L"");
			MsiSetProperty(hInstall,L"DB_USE_WINDOWS_AUTH",L"true");
			MsiSetProperty(hInstall,L"SQL_USERNAME",L"");
		}
	}
	WriteMSILog(hInstall,_CAFOOTER_);

	return dwRet;
}

extern "C" UINT __stdcall ReadInstallerConfigurationFile(MSIHANDLE hInstall)
{
	DWORD dwRet = ERROR_SUCCESS;

	TCHAR szCmd[MAX_PATH] = {0};
	//TCHAR szInBuf[MAX_PATH] = {0};
	DWORD dwCmd = MAX_PATH;

	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_);
	MsiGetProperty(hInstall, _T("CustomActionData"), szCmd, &dwCmd);

	wstring strCmd(szCmd);
	wstring tok(L";");
	vector<wstring>cmdItems;
	split(strCmd, tok,cmdItems);

	ReadConfigToInstallProperty(hInstall,L"APP", L"JAVA_HOME", cmdItems[0].c_str());

	ReadConfigToInstallProperty(hInstall,L"ServiceAccount", L"USE_SERVICE_ACCOUNT", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"ServiceAccount", L"DOMAIN_NAME", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"ServiceAccount", L"USER_NAME", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"ServiceAccount", L"USER_PASSWORD", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"ServiceAccount", L"USER_PASSWORD_B64", cmdItems[0].c_str());

	ReadConfigToInstallProperty(hInstall,L"DB", L"DB_AUTH_TYPE", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"DB", L"DB_HOST", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"DB", L"DB_INSTANCE", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"DB", L"DB_NAME", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"DB", L"DB_PORT", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"DB", L"SQL_USERNAME", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"DB", L"SQL_PASSWORD", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"DB", L"SQL_PASSWORD_B64", cmdItems[0].c_str());

	ReadConfigToInstallProperty(hInstall,L"RBMQ", L"RBMQ_USERNAME", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"RBMQ", L"RBMQ_PASSWORD", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"RBMQ", L"INSTALL_CLUSTERED", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"RBMQ", L"RABBITMQ_EXTERNAL_NODE", cmdItems[0].c_str());
 
	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"CERTIFICATE_PATH", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"KEYSTORE_FQDN", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"KEYSTORE_INFO", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"KEYSTORE_PASSWORD", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"KEYSTORE_PATH", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"SZ_CERTIFICATE_PATH", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"SZ_KEYSTORE_PASSWORD", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"SZ_KEYSTORE_PATH", cmdItems[0].c_str());

	ReadConfigToInstallProperty(hInstall,L"BOOTSTRAP", L"BOOTSTRAP_SHARED_KEY", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"BOOTSTRAP", L"BS_CERTIFICATE_PATH", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"BOOTSTRAP", L"BS_KEYSTORE_PASSWORD", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"BOOTSTRAP", L"BS_KEYSTORE_PATH", cmdItems[0].c_str());

	ReadConfigToInstallProperty(hInstall,L"BOOTSTRAP", L"DC_BOOTSTRAP_KEYSTORE_PASSWORD", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"BOOTSTRAP", L"EDS_BOOTSTRAP_KEYSTORE_PASSWORD", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"BOOTSTRAP", L"PS_BOOTSTRAP_KEYSTORE_PASSWORD", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"BOOTSTRAP", L"MC_BOOTSTRAP_KEYSTORE_PASSWORD", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"BOOTSTRAP", L"MNR_BOOTSTRAP_KEYSTORE_PASSWORD", cmdItems[0].c_str());
	
	WriteMSILog(hInstall,_CAFOOTER_);
	return dwRet;
}

extern "C" UINT __stdcall CallNsisLibDBVerify(MSIHANDLE hInstall)
{
	DWORD dwRet = ERROR_SUCCESS;

	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_);
	MsiSession msi(hInstall);

	map<wstring,wstring> msiProps;

	msi.AddPropertyMap(L"INSTALL_CLUSTERED"	,msiProps);
	msi.AddPropertyMap(L"OLDPRODUCTS"		,msiProps);
	msi.AddPropertyMap(L"ProductVersion"	,msiProps);
	msi.AddPropertyMap(L"NSISLIB_1_Dir"		,msiProps);
	msi.AddPropertyMap(L"USER_NAME"			,msiProps);
	msi.AddPropertyMap(L"DOMAIN_NAME"		,msiProps);
	msi.AddPropertyMap(L"USER_PASSWORD"		,msiProps);
	
	msi.AddPropertyMap(L"DB_HOST"			,msiProps);
	msi.AddPropertyMap(L"DB_PORT"			,msiProps);
	msi.AddPropertyMap(L"DB_NAME"			,msiProps);
	msi.AddPropertyMap(L"SQL_USERNAME"		,msiProps);
	msi.AddPropertyMap(L"SQL_PASSWORD"		,msiProps);
	msi.AddPropertyMap(L"DB_AUTH_TYPE"		,msiProps);
	msi.AddPropertyMap(L"USE_SERVICE_ACCOUNT"		,msiProps);

	msi.AddPropertyMap(L"javaRT_TempPath"	,msiProps);
	msi.AddPropertyMap(L"NSISLIB_1_Dir"		,msiProps);

	wstring javaRTTempPath	= GetInstallerTempJavaRTLocation(msiProps[L"ProductVersion"]);

	wstring accountName(msiProps[L"DOMAIN_NAME"]+L"\\"+msiProps[L"USER_NAME"]);
	//Set java and nsislib directories to service account credentials
	if(msiProps[L"USE_SERVICE_ACCOUNT"].compare(L"ServiceAccount")==0)
	{
		SetFolderPermission(hInstall,msiProps[L"javaRT_TempPath"]	, msiProps[L"DOMAIN_NAME"], msiProps[L"USER_NAME"]);
		SetFolderPermission(hInstall,msiProps[L"NSISLIB_1_Dir"]		, msiProps[L"DOMAIN_NAME"], msiProps[L"USER_NAME"]);
	}
	//Base64 encode user information before validating.	
	wstring accountNameB64	= base64_encode_w(accountName);
	wstring userNameB64		= base64_encode_w(msiProps[L"USER_NAME"]);
	wstring userPassWordB64 = base64_encode_w(msiProps[L"USER_PASSWORD"]);
	wstring sqlUserNameB64	= base64_encode_w(msiProps[L"SQL_USERNAME"]);
	//wstring sqlPassWordB64	= base64_encode_w(msiProps[L"SQL_PASSWORD"]);

	msi.SetPropertyMap(L"USER_ACCOUNT_B64"	,accountNameB64,msiProps);
	msi.SetPropertyMap(L"USER_NAME_B64"		,userNameB64,msiProps);
	msi.SetPropertyMap(L"USER_PASSWORD_B64"	,userPassWordB64,msiProps);
	msi.SetPropertyMap(L"SQL_USER_B64"	,sqlUserNameB64,msiProps);
	if(msiProps[L"USE_SERVICE_ACCOUNT"].compare(L"LocalSystem")==0)
	{
		
		//msiProps[L"SQL_PASSWORD"].clear();
		//msiProps[L"SQL_PASSWORD_B64"].clear();
	}
	else{
		wstring sqlPassWordB64	= base64_encode_w(msiProps[L"SQL_PASSWORD"]);
		msi.SetPropertyMap(L"SQL_PASSWORD_B64"	,sqlPassWordB64,msiProps);
	}

	if(msiProps[L"SQL_PASSWORD"].length()>0)
	{
		wstring sqlPassWordB64	= base64_encode_w(msiProps[L"SQL_PASSWORD"]);
		msi.SetPropertyMap(L"SQL_PASSWORD_B64"	,sqlPassWordB64,msiProps);
	}

	DWORD dwNsisVerifyRet=CallNsisVerifyDatabaseConnectionWindows(hInstall,msiProps);
	
	switch(dwNsisVerifyRet)
	{
	case 2:
		MsiMessageBox(hInstall,L"The selected database is already populated. Please verify your settings", 0, FALSE);
		break;
	case 3:
		MsiMessageBox(hInstall,L"The selected database is empty. Please verify your settings", 0, FALSE);
		break;
	case 4:
		MsiMessageBox(hInstall,L"The selected database is contains no data. Please verify your settings", 0, FALSE);
		break;
	}

	if(dwNsisVerifyRet != 0)
	{
		msi.SetProperty(L"SqlConnectionTestResult",L"0");
		WriteMSILog(hInstall,L"NSIS database validation failed ");
	}
	else
	{
		msi.SetProperty(L"SqlConnectionTestResult",L"1");
	}

	return dwRet;
}

extern "C" UINT __stdcall AuthenticateWindowsServiceAccount(MSIHANDLE hInstall)
{
	DWORD dwRet = ERROR_SUCCESS;
	//MsiMessageBox(hInstall, _T("CA AuthenticateWindowsServiceAccount"), 0, FALSE);
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_);

	MsiSession msi(hInstall);

	map<wstring,wstring> msiProps;

	msi.AddPropertyMap(L"INSTALL_CLUSTERED"	,msiProps);
	msi.AddPropertyMap(L"OLDPRODUCTS"		,msiProps);
	msi.AddPropertyMap(L"ProductVersion"	,msiProps);
	msi.AddPropertyMap(L"NSISLIB_1_Dir"		,msiProps);
	msi.AddPropertyMap(L"USER_NAME"			,msiProps);
	msi.AddPropertyMap(L"DOMAIN_NAME"		,msiProps);
	msi.AddPropertyMap(L"USER_PASSWORD"		,msiProps);
	
	msi.AddPropertyMap(L"USE_SERVICE_ACCOUNT"		,msiProps);

	msi.AddPropertyMap(L"javaRT_TempPath"	,msiProps);
	msi.AddPropertyMap(L"NSISLIB_1_Dir"		,msiProps);

	wstring javaRTTempPath	= GetInstallerTempJavaRTLocation(msiProps[L"ProductVersion"]);

	wstring accountName(msiProps[L"DOMAIN_NAME"]+L"\\"+msiProps[L"USER_NAME"]);
	//Set java and nsislib directories to service account credentials

	wstring accountNameB64	= base64_encode_w(accountName);
	wstring userNameB64		= base64_encode_w(msiProps[L"USER_NAME"]);
	wstring userPassWordB64 = base64_encode_w(msiProps[L"USER_PASSWORD"]);

	msi.SetPropertyMap(L"USER_ACCOUNT_B64"	,accountNameB64,msiProps);
	msi.SetPropertyMap(L"USER_NAME_B64"		,userNameB64,msiProps);
	msi.SetPropertyMap(L"USER_PASSWORD_B64"	,userPassWordB64,msiProps);

	HANDLE hToken;
	BOOL bIsServiceLogin = LogonUser(msiProps[L"USER_NAME"].c_str(), msiProps[L"DOMAIN_NAME"].c_str(), msiProps[L"USER_PASSWORD"].c_str(),LOGON32_LOGON_SERVICE,LOGON32_PROVIDER_DEFAULT,&hToken);
	if(bIsServiceLogin)
	{
		WriteMSILogEx(hInstall, L"Logon user %s\\ %s LOGON32_LOGON_SERVICE Success", msiProps[L"DOMAIN_NAME"].c_str(),msiProps[L"USER_NAME"].c_str(),0);
	}

	BOOL bIsInteractiveLogin = LogonUser(msiProps[L"USER_NAME"].c_str(), msiProps[L"DOMAIN_NAME"].c_str(), msiProps[L"USER_PASSWORD"].c_str(),LOGON32_LOGON_INTERACTIVE,LOGON32_PROVIDER_DEFAULT,&hToken);
	if(bIsInteractiveLogin)
	{
		WriteMSILogEx(hInstall, L"Logon user %s\\ %s LOGON32_LOGON_INTERACTIVE Success", msiProps[L"DOMAIN_NAME"].c_str(),msiProps[L"USER_NAME"].c_str(),0);
	}

	if(!bIsServiceLogin && !bIsInteractiveLogin)
	{
		msi.SetProperty(L"AI_USER_EXISTS",L"No");
		WriteMSILogEx(hInstall, L"Logon user %s\\ %s failed with retcode %d", msiProps[L"DOMAIN_NAME"].c_str(),msiProps[L"USER_NAME"].c_str(),0);
	}
	else
	{
		msi.SetProperty(L"AI_USER_EXISTS",L"Yes");
		msi.SetProperty(L"AI_USER_VALID_PASSWORD",L"Yes");
		WriteMSILog(hInstall,L"AuthenticateWindowsServiceAccount Success.");
	}

	WriteMSILog(hInstall,_CAFOOTER_);
	return dwRet;
}

extern "C" UINT __stdcall GetLocalFQDN(MSIHANDLE hInstall)
{
	DWORD dwret = ERROR_SUCCESS;
	
	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_);
	//MsiMessageBox(hInstall, _T("CA GetLocalFQDN"), 0, FALSE);
	MsiSession msi(hInstall);
	map<wstring,wstring> msiProps;
	TCHAR szComputerName[MAX_PATH]={0};
	TCHAR szNetBIOSName[MAX_PATH]={0};
	TCHAR szUserName[MAX_PATH]={0};
	DWORD dwCname=MAX_PATH;
	//ComputerNameNetBIOS
	msi.AddPropertyMap(L"USER_NAME"			,msiProps);
	msi.AddPropertyMap(L"DOMAIN_NAME"		,msiProps);
	msi.AddPropertyMap(L"USER_PASSWORD"		,msiProps);
	//MsiMessageBox(hInstall, msiProps[L"USER_NAME"].c_str(), 0, FALSE);
	if(msiProps[L"USER_NAME"].length() == 0)
	{
		GetUserName(szUserName,&dwCname);
		//MsiMessageBox(hInstall, szUserName, 0, FALSE);
		msi.SetProperty(L"USER_NAME",szUserName);
	}
	if(msiProps[L"DOMAIN_NAME"].length() == 0)
	{
		GetComputerNameEx(COMPUTER_NAME_FORMAT::ComputerNameDnsDomain,szComputerName, &dwCname);
		dwCname=MAX_PATH;
		GetComputerNameEx(COMPUTER_NAME_FORMAT::ComputerNameNetBIOS,szNetBIOSName, &dwCname);
		dwCname=MAX_PATH;
		//MsiMessageBox(hInstall, szComputerName, 0, FALSE);
		msi.SetProperty(L"DOMAIN_NAME",szComputerName);
		
	}
	//MsiMessageBox(hInstall, szNetBIOSName, 0, FALSE);
	WriteMSILog(hInstall,_CAFOOTER_);
	return dwret;
}

extern "C" UINT __stdcall UnhashPasswords(MSIHANDLE hInstall)
{
	DWORD dwret = ERROR_SUCCESS;
	//MsiMessageBox(hInstall, L"UnhashPasswords", 0, FALSE);
	MsiSession msi(hInstall);
	TCHAR szCmd[MAX_PATH*3] = {0};
	//TCHAR szInBuf[MAX_PATH] = {0};
	DWORD dwCmd = MAX_PATH*3;

	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_);
	MsiGetProperty(hInstall, _T("CustomActionData"), szCmd, &dwCmd);

	wstring strCmd(szCmd);
	wstring tok(L";");
	vector<wstring>cmdItems;
	split(strCmd, tok,cmdItems);
	wstring decodedstring(L"");
	//Decode and update passwords.
	//MsiMessageBox(hInstall, L"UnhashPasswords", 0, FALSE);
	if(ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"DB", L"SQL_PASSWORD_B64", cmdItems[2].c_str()))
	{	
		wstring b64string = msi.GetProperty(L"SQL_PASSWORD_B64");
		if(!b64string.empty())
		{
			decodedstring = base64_decode_w(b64string);
			msi.SetProperty(L"SQL_PASSWORD",decodedstring);
		}
		else
			msi.SetProperty(L"SQL_PASSWORD",decodedstring);
	}

	if(ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"ServiceAccount", L"USER_PASSWORD_B64", cmdItems[2].c_str()))
	{	
		wstring b64string = msi.GetProperty(L"USER_PASSWORD_B64");
		if(!b64string.empty())
		{
			decodedstring = base64_decode_w(b64string);
			msi.SetProperty(L"USER_PASSWORD",decodedstring);
		}
		else
			msi.SetProperty(L"USER_PASSWORD",decodedstring);
	}


	ReadConfigToInstallProperty(hInstall,L"APP", L"JAVA_HOME", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"APP", L"APPDIR", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"ServiceAccount", L"USE_SERVICE_ACCOUNT", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"ServiceAccount", L"DOMAIN_NAME", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"ServiceAccount", L"USER_NAME", cmdItems[2].c_str());

	ReadConfigToInstallProperty(hInstall,L"DB", L"DB_AUTH_TYPE", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"DB", L"DB_HOST", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"DB", L"DB_NAMED_INSTANCE", cmdItems[2].c_str());
	//ReadConfigToInstallProperty(hInstall,L"DB", L"DB_NAME", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"DB", L"DB_PORT", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"DB", L"SQL_USERNAME", cmdItems[2].c_str());

	ReadConfigToInstallProperty(hInstall,L"RBMQ", L"RBMQ_USERNAME", cmdItems[2].c_str());

	ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"RBMQ", L"RBMQ_PASSWORD", cmdItems[2].c_str());

	ReadConfigToInstallProperty(hInstall,L"RBMQ", L"INSTALL_CLUSTERED", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"RBMQ", L"RABBITMQ_EXTERNAL_NODE", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"RBMQ", L"RBMQ_USERNAME", cmdItems[2].c_str());

	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"API_CERTIFICATE_PATH", cmdItems[2].c_str());
	ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"KEYSTORE", L"API_KEYSTORE_PASSWORD", cmdItems[2].c_str());

	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"API_KEYSTORE_PATH", cmdItems[0].c_str());
	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"SZ_CERTIFICATE_PATH", cmdItems[0].c_str());
	ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"KEYSTORE", L"API_KEYSTORE_PASSWORD", cmdItems[2].c_str());

	ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"KEYSTORE", L"SZ_KEYSTORE_PASSWORD", cmdItems[2].c_str());

	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"SZ_KEYSTORE_PATH", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"CERTIFICATE_PATH", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"KEYSTORE_INFO", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"KEYSTORE_PATH", cmdItems[2].c_str());

	ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"KEYSTORE", L"KEYSTORE_PASSWORD", cmdItems[2].c_str());

	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"KEYSTORE_PATH", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"KEYSTORE", L"KEYSTORE_FQDN", cmdItems[2].c_str());


	ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"BOOTSTRAP", L"API_BOOTSTRAP_KEYSTORE_PASSWORD", cmdItems[2].c_str());
	ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"BOOTSTRAP", L"PS_BOOTSTRAP_KEYSTORE_PASSWORD", cmdItems[2].c_str());
	ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"BOOTSTRAP", L"MNR_BOOTSTRAP_KEYSTORE_PASSWORD", cmdItems[2].c_str());
	ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"BOOTSTRAP", L"MC_BOOTSTRAP_KEYSTORE_PASSWORD", cmdItems[2].c_str());
	ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"BOOTSTRAP", L"EDS_BOOTSTRAP_KEYSTORE_PASSWORD", cmdItems[2].c_str());
	ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"BOOTSTRAP", L"DC_BOOTSTRAP_KEYSTORE_PASSWORD", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"BOOTSTRAP", L"BS_CERTIFICATE_PATH", cmdItems[2].c_str());
	ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"BOOTSTRAP", L"BS_KEYSTORE_PASSWORD", cmdItems[2].c_str());
	ReadConfigHashToInstallProperty(hInstall,cmdItems[0].c_str(), cmdItems[1].c_str(),L"BOOTSTRAP", L"BOOTSTRAP_SHARED_KEY", cmdItems[2].c_str());
	ReadConfigToInstallProperty(hInstall,L"BOOTSTRAP", L"BS_KEYSTORE_PATH", cmdItems[2].c_str());
	return dwret;
}

extern "C" UINT __stdcall BackupFilesOnUpgrade(MSIHANDLE hInstall)
{
	DWORD dwret = ERROR_SUCCESS;
	//MsiMessageBox(hInstall, L"UnhashPasswords", 0, FALSE);
	MsiSession msi(hInstall);
	TCHAR szCmd[MAX_PATH*3] = {0};
	DWORD dwCmd = MAX_PATH*3;

	assert(hInstall != NULL);
	WriteMSILog(hInstall,_CAHEADER_);
	MsiGetProperty(hInstall, _T("CustomActionData"), szCmd, &dwCmd);

	wstring strCmd(szCmd);
	wstring tok(L";");
	vector<wstring>cmdItems;
	split(strCmd, tok,cmdItems);
	/* make sure we have at least 3 items passed.
	0 - Source install path
	1 - Backup install path
	2 - Wildcard list of files to backup
	*/

	if(!DirectoryBackup(cmdItems[0], cmdItems[1],cmdItems[2]))
	{
		dwret = ERROR_INSTALL_FAILURE;
		WriteMSILog(hInstall,L"BackupFilesOnUpgrade FAILED");
	}

	return dwret;

	WriteMSILog(hInstall,_CAFOOTER_);
}

