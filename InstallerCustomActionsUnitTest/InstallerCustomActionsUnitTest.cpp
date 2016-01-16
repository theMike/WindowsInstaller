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

 https://github.com/dblock/codeproject/tree/master/UnitTestingCustomActions/Source/CustomActionTest

#include "stdafx.h"
#include "..\..\..\ThirdParty\WinUnit\1_2_0909_1\Include\WinUnit.h"
#include "msi.h"
#include "msiquery.h"
#include "..\InstallerCustomActions\InstallerCustomActions.h"
#include "..\InstallerCustomActions\InstallerCustomActionsUtility.h"
#include "UnitTestUtility.h"
#include <string>
#include "..\InstallerCustomActions\JavaPathBuilder.h"

//#define RUNCMDLINE
#define CADEBUG

#ifdef RUNCMDLINE
#define CUSTOM_ACTION_DLL L"InstallerCustomActions.dll"
#else
#define CUSTOM_ACTION_DLL L"..\\x64\\Debug\\InstallerCustomActions.dll"
#endif

using namespace std;

#ifndef CADEBUG
BEGIN_TEST(BASE64_DECODE)
{
	//Decode base64 strings.
	//Template stirng to test -> This is the target string - !@#$%^&*()_+{}|;',."<>?
	wstring base_string(L"This is the target string - !@#$%^&*()_+{}|;',.\"<>?");
	wstring encoded_string(L"VGhpcyBpcyB0aGUgdGFyZ2V0IHN0cmluZyAtICFAIyQlXiYqKClfK3t9fDsnLC4iPD4/");
	wstring decoded_string;

	wstring encode_string = base64_encode_w(base_string);

	decoded_string = base64_decode_w(encoded_string);
	//Test base string and decoded string are equal
	WIN_ASSERT_TRUE(base_string.compare(decoded_string) == 0 );
	//Test base encoded string and encoder output are equal
	WIN_ASSERT_TRUE(encode_string.compare(encoded_string) == 0 );
}
END_TEST

BEGIN_TEST(EXECUTE_SIMPLE_RunCmd_SUCCESS1)
{	
	//Test msi custom action RunCmd
	MSIUnit msiu;
	int nret;
	wstring action_data(L"cmd.exe /c dir C:\\Windows");
	msiu.SetActionData(action_data.c_str());
	nret=msiu.CallMSICA(CUSTOM_ACTION_DLL,"RunCmd");
	WIN_ASSERT_ZERO(nret);
}
END_TEST

BEGIN_TEST(EXECUTE_SIMPLE_RunCmd_FAIL1)
{
	//Test msi custom action RunCmd
	MSIUnit msiu;
	int nret;
	wstring action_data(L"cmd.exe /c dir X:\\FAILWindows");
	msiu.SetActionData(action_data.c_str());
	//This command will fail. 
	//Selecting 'Continue when the error message is displayed will fail this test"
	nret=msiu.CallMSICA(CUSTOM_ACTION_DLL,"RunCmd");
	WIN_ASSERT_NOT_ZERO(nret);
}
END_TEST

BEGIN_TEST(EXECUTE_SIMPLE_RunCmd_TIMEOUT)
{
	//Test msi custom action RunCmdTimeOut
	MSIUnit msiu;
	int nret;
	
	wstring action_data(L"-12;cmd.exe /c ping 127.0.0.1 -n 30");
	msiu.SetActionData(action_data.c_str());
	//This command will fail. 
	//Selecting 'Continue when the error message is displayed will fail this test"
	nret=msiu.CallMSICA(CUSTOM_ACTION_DLL,"RunCmdTimeOut");
	WIN_ASSERT_NOT_ZERO(nret);
}
END_TEST

BEGIN_TEST(EXECUTE_SIMPLE_RunCmdWithRetryTimeOut_TIMEOUT)
{
	//Test msi custom action RunCmdTimeOut
	MSIUnit msiu;
	int nret;
	
	wstring action_data(L"5000;cmd.exe /c ping 127.0.0.1 -n 30");
	msiu.SetActionData(action_data.c_str());
	//This command will fail. 
	//Selecting 'Continue when the error message is displayed will fail this test"
	nret=msiu.CallMSICA(CUSTOM_ACTION_DLL,"RunCmdWithRetryTimeOut");
	WIN_ASSERT_NOT_ZERO(nret);
}
END_TEST

BEGIN_TEST(VERSION_COMPARE)
{
	//      POT Version       Installer Version
	wstring verA1(L"1.2.3.4"),	verA2(L"1.2.3.4");		//This should pass
	wstring verB1(L"1.2.3.42"),	verB2(L"1.2.3.4");		//This should fail
	wstring verC1(L"1.2.3.42"),	verC2(L"1.2.3.42342");	//This should pass
	wstring verD1(L"1.2.3"),	verD2(L"1.2.3.4");		//This should fail
	wstring verE1(L"1.2.3.4"),	verE2(L"1.2.3");		//This should fail
	wstring verF1(L"1.2"),		verF2(L"1.2");			//This should pass

	WIN_ASSERT_TRUE(VersionCompareEqual(verA1, verA2));
	WIN_ASSERT_FALSE(VersionCompareEqual(verB1, verB2));
	WIN_ASSERT_TRUE(VersionCompareEqual(verC1, verC2));
	WIN_ASSERT_FALSE(VersionCompareEqual(verD1, verD2));
	WIN_ASSERT_FALSE(VersionCompareEqual(verE1, verE2));
	WIN_ASSERT_TRUE(VersionCompareEqual(verF1, verF2));
}
END_TEST

BEGIN_TEST(SLEEP_CUSTOM_ACTION)
{
	MSIUnit msiu;
	DWORD dwBuf = MAX_PATH;
	TCHAR szCWD[1024];
	TCHAR szTest[MAX_PATH]={0};
	GetCurrentDirectory(1024,szCWD);
	//msiu.SetActionData("5000");
	MsiSetProperty(msiu.hproduct, L"CustomActionData",L"5000");
	msiu.CallMSICA(CUSTOM_ACTION_DLL,	"InstallSleep");
	WIN_TRACE("End Test");
}
END_TEST

BEGIN_TEST(TEST_CA_RBMQCLEANUP_MSG)
{
	MSIUnit msiu;

	DWORD dwBuf = MAX_PATH;
	TCHAR szCWD[1024];
	TCHAR szTest[MAX_PATH]={0};
	GetCurrentDirectory(1024,szCWD);
	wstring action_data2(L"");
	msiu.SetActionData(action_data2.c_str());

	msiu.CallMSICA(CUSTOM_ACTION_DLL, "CleanUpRabbitMQ");
	
	WIN_TRACE("End Test");
}
END_TEST

BEGIN_TEST(TEST_CA_CONFIG_READ)
{
	MSIUnit msiu;

	DWORD dwBuf = MAX_PATH;
	TCHAR szCWD[1024];
	TCHAR szTest[MAX_PATH]={0};
	GetCurrentDirectory(1024,szCWD);
	wstring action_data2(L"\\AdvancedInstaller\\InstallerCustomActions\\CA Test.ini");
	msiu.SetActionData(action_data2.c_str());

	msiu.CallMSICA(CUSTOM_ACTION_DLL, "ReadInstallerConfigurationFile");
	
	WIN_TRACE("End Test");
}
END_TEST


BEGIN_TEST(TEST_CA_FQDN)
{
	MSIUnit msiu;

	DWORD dwBuf = MAX_PATH;
	TCHAR szCWD[1024];
	TCHAR szTest[MAX_PATH]={0};
	GetCurrentDirectory(1024,szCWD);
	wstring action_data2(L"");

	msiu.CallMSICA(CUSTOM_ACTION_DLL, "AuthenticateWindowsServiceAccount");
	
	WIN_TRACE("End Test");
}
END_TEST
#else

BEGIN_TEST(TEST_CA_READCONFIGFILE)
{

	DWORD dwBuf = MAX_PATH;
	TCHAR szCWD[1024];
	TCHAR szTest[MAX_PATH]={0};
	GetCurrentDirectory(1024,szCWD);
	wstring action_data(L"c:\\test\\;c:\\test-to;*.log");
	wstring rnd = RandNumberStr(16);
	rnd = RandNumberStr(16);
	rnd = RandNumberStr(16);
	rnd = RandNumberStr(16);
	rnd = RandNumberStr(16);
	rnd = RandNumberStr(16);
	rnd = RandNumberStr(16);
	rnd = RandNumberStr(16);
	rnd = RandNumberStr(16);
	rnd = RandNumberStr(16);

	WIN_TRACE("End Test");
}
END_TEST

#endif