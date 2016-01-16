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
#include <cstdarg>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <TlHelp32.h>
#include "Shlobj.h"
#include "JavaPathBuilder.h"
#include "Shellapi.h"
#include <time.h>

#define DEBUG 0
#define BUFREAD 4096

#pragma comment(lib,"shell32.lib")

using namespace std;

inline wstring FormatLogMessage(const wchar_t* fmt, ... );

int MsiMessageBox(MSIHANDLE hInstall, const TCHAR* szString, DWORD dwDlgFlags, BOOL bYesNo)
{
	
	PMSIHANDLE newHandle = MsiCreateRecord(2);
	MsiRecordSetString(newHandle, 0, szString);
	
	if (bYesNo)
		return (MsiProcessMessage(hInstall, INSTALLMESSAGE((INSTALLMESSAGE_USER + dwDlgFlags)|MB_YESNO), newHandle));
	else
		return (MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_USER + dwDlgFlags), newHandle));
}

int MsiMessageBoxEx(MSIHANDLE hInstall, const TCHAR* szString, UINT unMBType)
{
	PMSIHANDLE newHandle = MsiCreateRecord(2);
	MsiRecordSetString(newHandle, 0, szString);
	return (MsiProcessMessage(hInstall, INSTALLMESSAGE(unMBType), newHandle));
}
int WriteMSILog(MSIHANDLE hInstall, LPCTSTR lpString)
{
	
	PMSIHANDLE hRecord = MsiCreateRecord(0);
	MsiRecordSetString(hRecord, 0, lpString);
	return MsiProcessMessage(hInstall, INSTALLMESSAGE_INFO, hRecord); 

}

int WriteMSILogFiltered(MSIHANDLE hInstall, LPCTSTR lpString, vector<wstring>& blacklist)
{
	vector<wstring>::iterator itr;
	wstring basestring(lpString);
	wstring replacestring(L"*******");
	if(blacklist.size()>0)
	{
		for(itr=blacklist.begin(); itr!=blacklist.end(); itr++)
		{
			string_replace(basestring,*itr,replacestring);
		}
	}
	PMSIHANDLE hRecord = MsiCreateRecord(0);
	MsiRecordSetString(hRecord, 0, basestring.c_str());
	return MsiProcessMessage(hInstall, INSTALLMESSAGE_INFO, hRecord); 

}

int WriteMSILogEx(MSIHANDLE hInstall, LPCTSTR lpFormat, ...)
{
	assert(lpFormat != NULL);
	int size = 1024;
	wchar_t* buffer = NULL;
	buffer = new wchar_t[size];
	va_list vl;
	va_start(vl, lpFormat);
	int nsize = _vsnwprintf_s(buffer, size, size, lpFormat, vl);
	if(size <= nsize)
	{
		delete[] buffer;
		buffer = 0;
		buffer = new wchar_t[nsize+1];
		nsize = _vsnwprintf_s(buffer, nsize+1,nsize+1, lpFormat,vl);
	}
	wstring write_tolog(buffer);
	delete[] buffer;
	
	PMSIHANDLE hRecord = MsiCreateRecord(0);
	MsiRecordSetString(hRecord, 0, write_tolog.c_str());
	return MsiProcessMessage(hInstall, INSTALLMESSAGE_INFO, hRecord); 
}

inline wstring FormatLogMessage(const wchar_t* fmt, ... )
{
	assert(fmt != NULL);
	int size = 1024;
	wchar_t* buffer = NULL;
	buffer = new wchar_t[size];
	va_list vl;
	va_start(vl, fmt);
	int nsize = _vsnwprintf_s(buffer, sizeof(buffer), size, fmt, vl);
	if(size <= nsize)
	{
		delete[] buffer;
		buffer = 0;
		buffer = new wchar_t[nsize+1];
		nsize = _vsnwprintf_s(buffer, sizeof(buffer),size, fmt,vl);
	}
	wstring ret(buffer);
	delete[] buffer;
	return ret;
}

void LogSystemMessage(MSIHANDLE hInstall,DWORD dwRC)
{
	LPTSTR lpMsgBuf;
	TCHAR szBuf[TMAX_PATH*3];
	DWORD rc;

	rc = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|
					FORMAT_MESSAGE_FROM_SYSTEM|
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, dwRC,
					MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
					reinterpret_cast<LPTSTR>(&lpMsgBuf),0,NULL);
	wsprintf(szBuf,_T("SYSTEM ERROR CODE: %ld -  %s \n"),dwRC, lpMsgBuf);
	WriteMSILog(hInstall, szBuf);

	LocalFree(lpMsgBuf);
}

int AskContinueOnError(MSIHANDLE hInstall, DWORD dwErr)
{
	int nret = ERROR_INSTALL_FAILURE;
	LogSystemMessage(hInstall,dwErr);
	wstring onfailmessage(L"The installer encountered a problem while executing a task.\nDo you wish to continue?");
	if( 0 == dwErr )
	{
		nret= ERROR_SUCCESS;
	}
	else
	{
		int nRet = MsiMessageBox(hInstall,onfailmessage.c_str(),	0,TRUE);
		
		if(nRet == IDYES)
			nret= ERROR_SUCCESS;
		else
			nret= ERROR_INSTALL_FAILURE;
	}
	
	return nret;
}
int AskRetryOnError(MSIHANDLE hInstall, DWORD dwErr)
{
	int nret = ERROR_INSTALL_FAILURE;
	LogSystemMessage(hInstall,dwErr);
	wstring onfailmessage(L"The installer encountered a problem while executing a task.\nDo you wish to Cancel, Try Again, or Continue?");
	if( 0 == dwErr )
	{
		nret= ERROR_SUCCESS;
	}
	else
	{
		nret = MsiMessageBoxEx(hInstall,onfailmessage.c_str(),	INSTALLMESSAGE_ERROR|MB_ABORTRETRYIGNORE|MB_ICONWARNING);
		if(nret == IDIGNORE)
			nret = ERROR_SUCCESS;
	}
	
	return nret;
}
int AskRetryOnErrorEx(MSIHANDLE hInstall, DWORD dwErr, LPCTSTR issuemessage)
{
	int nret = ERROR_INSTALL_FAILURE;
	LogSystemMessage(hInstall,dwErr);
	wstring onfailmessage(L"The installer encountered a problem while executing a task.\nDo you wish to Cancel, Try Again, or Continue?\n");
	onfailmessage.append(L"------------------------------------------------------------\n");
	onfailmessage.append(issuemessage);
	if( 0 == dwErr )
	{
		nret= ERROR_SUCCESS;
	}
	else
	{
		nret = MsiMessageBoxEx(hInstall,onfailmessage.c_str(),	INSTALLMESSAGE_ERROR|MB_ABORTRETRYIGNORE|MB_ICONWARNING);
		if(nret == IDIGNORE)
			nret = ERROR_SUCCESS;
	}
	
	return nret;
}
std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len)
{
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--)
	{
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; (i <4) ; i++)
			ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if(i)
	{
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while((i++ < 3))
			ret += '=';

	}
	return ret;
}

std::string base64_decode(std::string const& encoded_string)
{
	int in_len = (int)encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i ==4)
		{
			for (i = 0; i <4; i++)
			char_array_4[i] = (unsigned char)base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
			ret += char_array_3[i];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j <4; j++)
			char_array_4[j] = 0;

		for (j = 0; j <4; j++)
			char_array_4[j] = (unsigned char)base64_chars.find((size_t)char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
			ret += char_array_3[j];
	}

  return ret;
}

std::string wstrtostr(const std::wstring &wstr)
{
	std::string strTo;
    char *szTo = new char[wstr.length() + 1];
    szTo[wstr.size()] = '\0';
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, szTo, (int)wstr.length(), NULL, NULL);
    strTo = szTo;
    delete[] szTo;
    return strTo;
}

std::wstring strtowstr(const std::string &str)
{
    std::wstring wstrTo;
    wchar_t *wszTo = new wchar_t[str.length() + 1];
    wszTo[str.size()] = L'\0';
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wszTo, (int)str.length());
    wstrTo = wszTo;
    delete[] wszTo;
    return wstrTo;
}
/* Split a delimited string into a string vector */
void split(wstring& text, wstring& separators, vector<wstring>& words)
{
	size_t n = text.length();
	size_t start, stop;

	start = text.find_first_not_of(separators);
	while ((start >= 0) && (start < n))
	{
		stop = text.find_first_of(separators, start);
		if ((stop < 0) || (stop > n)) stop = n;
		words.push_back(text.substr(start, stop - start));
		start = text.find_first_not_of(separators, stop+1);
	}
}
/* Split a delimied string into a string vector but preserve null sections
   eg. item1;item2;;item3;;;item4
   will create a vector of
   "item1","item2","","item3","","","item4"
*/
void splitEx(wstring& text, wstring& separators, vector<wstring>& words)
{
	size_t n = text.length();
	size_t start, stop, next;

	start = text.find_first_not_of(separators);
	while ((start >= 0) && (start < n))
	{
		stop = text.find_first_of(separators, start);
		next = text.find_first_of(separators,stop+1);
		if ((stop < 0) || (stop > n)) stop = n;
		words.push_back(text.substr(start, stop - start));
		if(next == stop+1)
		{
			start = next;
			continue;
		}
		else
			start = text.find_first_not_of(separators, stop+1);
	}
}
wstring get_host_from_fqdn(wstring& fqdnname)
{
	//wstring hostname;
	wstring sep(L".");
	vector<wstring> split_fqdn;	
	split(fqdnname,sep,split_fqdn);
	return split_fqdn[0];
}

wstring base64_decode_w(wstring const& encoded_string)
{
	string return_buf;
	return_buf = base64_decode(wstrtostr(encoded_string));
	//There is an issue in the installer vbscript base64 encoder
	//that would insert NULLs into strings due to converting ANSI to Unicode
	//Make sure they are stripped out before continuing.
	return_buf.erase(std::remove(return_buf.begin(), return_buf.end(), '\0'),return_buf.end());
	return strtowstr(return_buf);

}

wstring base64_encode_w(wstring const& plain_string)
{
	string input_buf;
	//wstring wreturn_buf;
	string return_buf;

	input_buf=wstrtostr(plain_string);
	return_buf=base64_encode((const unsigned char*)input_buf.c_str(),(unsigned int)input_buf.length());

	return strtowstr(return_buf);
}

wstring BuildJavaCmdLineEx(LPCTSTR java_home_path,LPCTSTR java_lib_path, LPCTSTR nsislib_jar, LPCTSTR java_cmd)
{
	wstring return_buf;
	wstringstream java_cmd_line_to_use;
	wchar_t quotechar = '"';

	java_cmd_line_to_use << quotechar
						<< java_home_path
						<< L"\\bin\\java.exe"
						<< quotechar
						<< L" "
						<< L"-classpath "
						<< java_lib_path << L"\\*"
						<< L" "
						<< L"-Djava.library.path="
						<< java_lib_path
						<< L";"
						<< java_lib_path
						<< nsislib_jar
						<< L" "
						<< java_cmd;
						
	return java_cmd_line_to_use.str();
}
wstring BuildJavaCmdLine(wstring& java_home_path, wstring& java_lib_path, wstring& nsislib_jar, wstring& java_cmd)
{
	wstring return_buf;
	wstringstream java_cmd_line_to_use;
	wchar_t quotechar = '"';

	java_cmd_line_to_use << quotechar
						<< java_home_path
						<< L"\\bin\\java.exe"
						<< quotechar
						<< L" "
						<< L"-classpath "
						<< java_lib_path << L"\\*"
						<< L" "
						<< L"-Djava.library.path="
						<< java_lib_path
						<< L";"
						<< java_lib_path
						<< nsislib_jar
						<< L" "
						<< java_cmd;
						
	return java_cmd_line_to_use.str();
}


wstring update_property_file(wstring& erlang_cookie_file, wstring& property_file)
{
	wofstream outf;
	wifstream inf;
	wstring erlangcookie;
	wstring propertyline(L"erlang.cookie=");
	
	inf.open(erlang_cookie_file);
	outf.open(property_file, ios_base::app);
	getline(inf,erlangcookie);
	if(inf.is_open() && outf.is_open())
	{
		if(erlangcookie.size()>0)
		{
			propertyline.append(erlangcookie);
			outf << propertyline;
		}
		inf.close();
		outf.close();
	}
	return erlangcookie;
}

wstring choppa(const wstring &t, const wstring &ws)
{
    wstring str = t;
    size_t found;
    found = str.find_last_not_of(ws);
    if (found != string::npos)
    	str.erase(found+1);
    else
    	str.clear();  //str is all whitespace

    return str;
}

BOOL VersionCompareEqual(wstring& version_pot, wstring& version_installer)
{
	BOOL bret=TRUE;
	vector<wstring> verpot,verinstall;
	wstring sep(L".");
	split(version_pot,sep,verpot);
	split(version_installer,sep,verinstall);
	if(verpot.size() == verinstall.size())
	{
		int i;
		for(i=0;i<verpot.size()-1;i++)
		{
			if(_wtoi(verpot[i].c_str()) != _wtoi(verinstall[i].c_str()))
			{
				bret = FALSE;
				break;
			}

		}
		//The last digit is a special case.
		//The installer will have a build number appneded
		//during dev/QA builds.
		//Here compare the MSDs of the two versions and the
		//product version should be a subset of the installer
		//version. 
		if(0 != verinstall[i].find(verpot[i]))
		{
			bret = FALSE;
		}
	}
	else
		bret = FALSE;
	return bret;
}

void string_replace(wstring& base, const wstring& search, const wstring& replace) 
{
    size_t pos = 0;
	//OutputDebugString(base.c_str());
		
    while((pos = base.find(search, pos)) != std::string::npos)
	{
         base.replace(pos, search.length(), replace);
         pos += replace.length();
    }
	OutputDebugString(base.c_str());
}

BOOL LogFileFilter(wstring &source, wstring &target, vector<wstring> &filter_list)
{
	const DWORD dwRead=BUFREAD;
	HANDLE hlogin, hlogout;
	BOOL bret = TRUE;
    TCHAR logbuffer[BUFREAD]={0};
	DWORD readBytes,writeBytes = BUFREAD;
	//DWORD bufbytes2;
	BOOL bresult=FALSE;
	vector<wstring>::iterator list_pos;
	wstring pwdreplace(L"************");

	//Use Win32 apis for file reading and writing.
    hlogin = CreateFile(source.c_str(),GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    hlogout = CreateFile(target.c_str(),GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
	if((hlogin && hlogout) !=NULL)
	{
		do
		{
			bresult=ReadFile(hlogin,logbuffer,dwRead,&readBytes,NULL);
			wstring templog(logbuffer);
			if(filter_list.size() > 0)
			{
				for(list_pos= filter_list.begin();list_pos!=filter_list.end();list_pos++)
				{
					string_replace(templog,*list_pos,pwdreplace);
				}
			}		
			WriteFile(hlogout,templog.c_str(),dwRead,&writeBytes,NULL);

		}while(!((bresult && readBytes)==0)); //This is how EOF is determined using ReadFile

		CloseHandle(hlogout);
		CloseHandle(hlogin);
	}
	else
		bret = FALSE;
	return bret;
}

BOOL IsNetWorkShare(LPCTSTR lpRootPath)
{
	BOOL bret=TRUE;
	UINT unDrive = GetDriveType(lpRootPath);

	switch(unDrive)
	{
	case DRIVE_UNKNOWN:
	case DRIVE_REMOTE:
		bret = FALSE;
		break;
	default:
		bret = TRUE;
	}

	return bret;
}

void KillProcess(LPCTSTR processname)
{
	HANDLE hWin32SnapShot;
	PROCESSENTRY32 pEntry;
	BOOL hRes;
	HANDLE hProcess;

	hWin32SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL,NULL);
	pEntry.dwSize = sizeof(pEntry);
	hRes = Process32First(hWin32SnapShot, &pEntry);
	while(hRes)
	{
		if(0==lstrcmp(pEntry.szExeFile,processname))
		{
			hProcess=OpenProcess(PROCESS_TERMINATE, 0, (DWORD)pEntry.th32ProcessID);
			if(hProcess != NULL)
			{
				TerminateProcess(hProcess,9);
				CloseHandle(hProcess);
			}
		}
		hRes = Process32Next(hWin32SnapShot, &pEntry);
	}
	CloseHandle(hWin32SnapShot);
}

void DeleteFolder( const TCHAR *szFolderPath)
{
    wstring strFileFilter;
    strFileFilter = szFolderPath;
    strFileFilter += L"\\*.*";
    WIN32_FIND_DATA win32FindData; 
    HANDLE hFile = FindFirstFile(strFileFilter.c_str(), &win32FindData);
    while( FindNextFile(hFile, &win32FindData) )
    {
        wstring strFilePath;
        wstring strFileName; 
 
        strFilePath = szFolderPath;
        strFilePath += L"\\";
        strFilePath += win32FindData.cFileName;
        strFileName = win32FindData.cFileName;
 
        if( strFileName == L"." ||
            strFileName == L"..")
        { continue; }
 
        int iFindDot = (int)strFileName.find(L".");

        if( iFindDot < 0 )
        {
            DeleteFolder( strFilePath.c_str() );
        }
         DeleteFile(strFilePath.c_str());
    }
     FindClose( hFile ); //release handle otherwise dir cannot be removed
     RemoveDirectory( szFolderPath );
}

BOOL DeleteRegNode(HKEY hKeyRoot, LPTSTR lpSubNode)
{
	BOOL bret=FALSE;
	HKEY hKey;
	LPTSTR lpEnd;
	LONG lResult;
	DWORD dwSize;
	TCHAR szNodeName[MAX_PATH]={0};
	TCHAR lpNode[MAX_PATH]={0};
	FILETIME ftWrite;
	lstrcpy(lpNode,lpSubNode);

	//First try to delete the key if its already empty
	lResult=RegDeleteKey(hKeyRoot, lpNode);
	if(ERROR_SUCCESS == lResult)
		return TRUE;

	lResult = RegOpenKeyEx(hKeyRoot, lpNode, 0 ,KEY_READ, &hKey);
	if(ERROR_SUCCESS != lResult)
	{
		if(ERROR_FILE_NOT_FOUND == lResult)
			return TRUE;
		else
			return FALSE;
	}
	lpEnd = lpNode + lstrlen(lpNode);

    if (*(lpEnd - 1) != _T('\\')) 
    {
        *lpEnd =  _T('\\');
        lpEnd++;
        *lpEnd =  _T('\0');
    }
	lResult = RegEnumKeyEx(hKey, 0, szNodeName, &dwSize, NULL, NULL, NULL,&ftWrite);
	if(ERROR_SUCCESS == lResult)
	{
		do{
			lstrcpy(lpEnd, szNodeName);
			
			if(!DeleteRegNode(hKeyRoot, lpNode))
			{
				break;
			}
			dwSize = MAX_PATH;
			lResult = RegEnumKeyEx(hKey, 0 , szNodeName, &dwSize, NULL, NULL, NULL, &ftWrite);
		}
		while(ERROR_SUCCESS==lResult);
	}
	lpEnd--;
	*lpEnd = _T('\0');
	RegCloseKey(hKey);
	lResult = RegDeleteKey(hKeyRoot, lpNode);
	if(ERROR_SUCCESS == lResult)
		return TRUE;

	return bret;
}

void ReadConfigToInstallProperty(MSIHANDLE hInstall,LPCTSTR szSection, LPCTSTR szKeyProperty, LPCTSTR szConfigFile)
{
	DWORD dwBuf = MAX_PATH*2;
	TCHAR szResult[MAX_PATH*2];

	if(GetPrivateProfileString(szSection, szKeyProperty, L"", szResult, dwBuf, szConfigFile))
	{
		WriteMSILogEx(hInstall,L"GetPrivateProfileString: %s - %s \n",szSection, szKeyProperty);
		MsiSetProperty(hInstall,szKeyProperty,szResult);
	}
	else{
		WriteMSILogEx(hInstall,L"GetPrivateProfileString FAILED: %s - %s \n",szSection, szKeyProperty);
		MsiSetProperty(hInstall,szKeyProperty,L"");
	}
}



wstring GetInstallerTempJavaRTLocation(wstring& productVersion)
{
	TCHAR szAppDataPath[MAX_PATH*3];
	wstring tempJavaRTPath;

	SHGetFolderPath(NULL,CSIDL_LOCAL_APPDATA,NULL,0,szAppDataPath);

	tempJavaRTPath.assign(szAppDataPath);
	tempJavaRTPath.append(L"\\");
	tempJavaRTPath.append(L"-");
	tempJavaRTPath.append(productVersion);

	return tempJavaRTPath;
}

BOOL SetFolderPermission(MSIHANDLE hInstall,wstring& folder, wstring& domain, wstring& user)
{
	BOOL bret = FALSE;

	wstringstream cacl_cmd_line;

	cacl_cmd_line<< L"icacls "
		<< "\"" << folder
		<< L"\""
		<< L" /grant "
		<< L"\""
		<< domain<< "\\"<<user
		<< L"\""
		<< L":(OI)(CI)F "<<"\0\0";

	MSIExecuteProcess ep(hInstall);
	TCHAR cacl_cmd[WIN_MAX_PATH];
	lstrcpy(cacl_cmd,cacl_cmd_line.str().c_str());

	ep.Execute(cacl_cmd);

	if(ep.GetExitCode() == 0)
	{
		bret = TRUE;
	}
	return bret;
}
BOOL DirectoryBackup(wstring& from, wstring& to,wstring& wildcards)
{
	BOOL bret = FALSE;
	SYSTEMTIME st;
	vector<wstring> searchPatterns;
	vector<wstring>::iterator ita;
	wstring sep(L",");
	TCHAR szTo[TMAX_PATH]={0};
	GetSystemTime(&st);
	INT nret;
	
	wsprintf(szTo,L"%s\\%d-%d-%d-%d-%d-%d",to.c_str(),st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
	wstring archiveDir(szTo);
	if(wildcards.empty())
		wildcards=L"*.*";

	size_t slshfound = from.find_last_not_of(L"\\");

	if(slshfound!=wstring::npos)
	{
		from.erase(slshfound+1);
	}

	split(wildcards,sep,searchPatterns);
	for(ita=searchPatterns.begin();ita != searchPatterns.end(); ++ita)
	{
		nret = CopyDirectory(from, archiveDir,*ita);
	}

	if(ERROR_SUCCESS == nret)
		bret = TRUE;

	return bret;

}
BOOL DirectoryExists(LPCTSTR szPath)
{
  DWORD dwAttrib = GetFileAttributes(szPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
         (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

int WildCmp(const TCHAR *pattern, const TCHAR *string) {

	const TCHAR *cp = NULL, *mp = NULL;

	while ((*string) && (*pattern != L'*'))
	{
		if ((*pattern != *string) && (*pattern != L'?'))
		{
			return 0;
		}
		pattern++;
		string++;
	}

	while (*string)
	{
		if (*pattern == L'*')
		{
			if (!*++pattern)
			{
				return 1;
			}
			mp = pattern;
			cp = string+1;
		}
		else if ((*pattern == *string) || (*pattern == L'?'))
		{
			pattern++;
			string++;
		}
		else
		{
			pattern = mp;
			string = cp++;
		}
	}
	while (*pattern == L'*')
	{
		pattern++;
	}
  return !*pattern;
}

wstring RandNumberStr(int length)
{
	Sleep(1000);
	srand((unsigned)GetTickCount());
	int random=0;
	wstringstream random_number;

	random_number.str(L"");

	for(int i=0; i<length; i++)
	{

		random_number<<rand()%10;
	}
	return random_number.str();
}

int CopyDirectory(const wstring& SourceDirectory, const wstring& DestinationDirectory, const wstring& CopyPattern)
{
	wstring     strSource;				
	wstring     strDestination;		
	wstring     strPattern;				
	HANDLE      hFile;               
	WIN32_FIND_DATA FileInformation={0}; 
	DWORD dwRet;
	static DWORD dwFileCount;

	strPattern = SourceDirectory + L"\\*.*";

	if(!DirectoryExists(DestinationDirectory.c_str()))
	{
		if(CreateDirectory(DestinationDirectory.c_str(), 0) == ERROR_ALREADY_EXISTS)
			return GetLastError();
	}
	hFile = FindFirstFile(strPattern.c_str(), &FileInformation);

	if(hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(FileInformation.cFileName[0] != L'.')
			{
				strSource.erase();
				strDestination.erase();

				strSource      = SourceDirectory + L"\\" + FileInformation.cFileName;
				strDestination = DestinationDirectory + L"\\" + FileInformation.cFileName;

				if(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// Copy subdirectory
					dwRet=CopyDirectory(strSource, strDestination,CopyPattern);
					// return dwRet;
				}
				else
				{
					// Copy file
					if(WildCmp(CopyPattern.c_str(), strSource.c_str()))
					{
						if(::CopyFile(strSource.c_str(), strDestination.c_str(), TRUE) == FALSE)
							return ::GetLastError();
						else
							dwFileCount++;
					}
				}
			}
		}
		while(::FindNextFile(hFile, &FileInformation) == TRUE);
		// Close handle
		::FindClose(hFile);

		DWORD dwError = ::GetLastError();
		if(dwError != ERROR_NO_MORE_FILES)
		return dwError;
	}

	return 0;
}
