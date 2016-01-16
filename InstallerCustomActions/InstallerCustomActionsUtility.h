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
#include"InstallerCustomActions.h"
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <map>
#include <fstream>
#include <iostream>
#define NSISLIBJAR L"NSISLIB.JAR"
#define STER 1

int WriteMSILogEx(MSIHANDLE hInstall, LPCTSTR lpFormat, ...);
int WriteMSILog(MSIHANDLE hInstall, LPCTSTR lpString);

int MsiMessageBox(MSIHANDLE hInstall, const TCHAR* szString, DWORD dwDlgFlags, BOOL bYesNo);
int MsiMessageBoxEx(MSIHANDLE hInstall, const TCHAR* szString, UINT unMBType);
int AskContinueOnError(MSIHANDLE hInstall, DWORD dwErr);
int AskRetryOnError(MSIHANDLE hInstall, DWORD dwErr);
int AskRetryOnErrorEx(MSIHANDLE hInstall, DWORD dwErr, LPCTSTR issuemessage);

using namespace std;
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

template<typename T, size_t N>
T * arrayend(T (&ra)[N]) {
    return ra + N;
}
std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);
std::string base64_decode(std::string const& encoded_string);

std::string wstrtostr(const std::wstring &wstr);
wstring base64_encode_w(wstring const& plain_string);
wstring base64_decode_w(wstring const& encoded_string);
wstring BuildJavaCmdLine(wstring& java_home_path, wstring& java_lib_path, wstring& nsislib_jar, wstring& java_cmd);
wstring BuildJavaCmdLineEx(LPCTSTR java_home_path,LPCTSTR java_lib_path, LPCTSTR nsislib_jar, LPCTSTR java_cmd);
void split(wstring& text, wstring& separators, vector<wstring>& words);
void splitEx(wstring& text, wstring& separators, vector<wstring>& words);
wstring ReadCorePOTFile(LPCTSTR szPotFilePath);
BOOL UnHasher(LPCTSTR javaPath, LPCTSTR nsisLibPath, LPCTSTR hashText, LPCTSTR passKey, wstring& plaintext, MSIHANDLE hInstall);
BOOL Hasher(LPCTSTR javaPath, LPCTSTR nsisLibPath, LPCTSTR plainText, wstring& hashtext, MSIHANDLE hInstall);
BOOL VerifyInternetConnection(LPCTSTR javaPath, LPCTSTR nsisLibPath, LPCTSTR serverName, LPCTSTR port, wstring& plaintext, MSIHANDLE hInstall);
wstring choppa(const wstring &t, const wstring &ws);
wstring get_host_from_fqdn(wstring& fqdnname);
wstring update_property_file(wstring& erlang_cookie_file, wstring& property_file);
BOOL VersionCompareEqual(wstring& version_pot, wstring& version_installer);
wstring BuildInitialUserCmdLine(vector<wstring> &cmdItems);
wstring BuildJavaCmdLine2(wstring& java_home_path, wstring& java_lib_path, wstring& nsislib_jar, wstring& java_cmd);
BOOL LogFileFilter(wstring &source, wstring &target, vector<wstring> &filter_list);
void string_replace(wstring& base, const wstring& search, const wstring& replace);
int WriteMSILogFiltered(MSIHANDLE hInstall, LPCTSTR lpString, vector<wstring>& blacklist);
BOOL IsNetWorkShare(LPCTSTR lpRootPath);
void KillProcess(LPCTSTR processname);
BOOL DeleteRegNode(HKEY hKeyRoot, LPTSTR lpSubNode);
BOOL DeleteDirectory(LPCTSTR sPath);
void DeleteFolder( const TCHAR *szFolderPath);
void ReadConfigToInstallProperty(MSIHANDLE hInstall,LPCTSTR szSection, LPCTSTR szKeyProperty, LPCTSTR szConfigFile);
wstring GetInstallerTempJavaRTLocation(wstring& productVersion);
BOOL SetFolderPermission(MSIHANDLE hInstall,wstring& folder, wstring& domain, wstring& user);
DWORD CallNsisVerifyDatabaseConnectionWindows(MSIHANDLE hInstall,map<wstring,wstring>& mInstProps);
BOOL ReadConfigHashToInstallProperty(MSIHANDLE hInstall,LPCTSTR javaPath, LPCTSTR nsisLibPath,LPCTSTR szSection, LPCTSTR szKeyProperty, LPCTSTR szConfigFile);
BOOL DirectoryBackup(wstring& from, wstring& to,wstring& wildcards);
int WildCmp(const TCHAR *wild, const TCHAR *string);
wstring RandNumberStr(int length);

int CopyDirectory(const wstring& SourceDirectory, const wstring& DestinationDirectory, const wstring& CopyPattern);

class Properties {

public:
    Properties ()  {}
    bool Read (const wstring& strFile) {
        wifstream is(strFile.c_str());
        if (!is.is_open()) return false;
        while (!is.eof())
		{
            wstring strLine;
            getline(is,strLine);
            int nPos = (int)strLine.find('=');
            if (wstring::npos == nPos)
				continue; // no '=', invalid line;

            wstring strKey = strLine.substr(0,nPos);
            wstring strVal = strLine.substr(nPos + 1, strLine.length() - nPos + 1);
            m_map.insert(map<wstring,wstring>::value_type(strKey,strVal));
        }
        return true;
    }

    bool GetValue(const wstring& strKey, wstring& strValue) const {

        map<wstring,wstring>::const_iterator i;
        i = m_map.find(strKey);
        if (i != m_map.end())
		{
            strValue = i->second;
            return true;
        }

        return false;
    }
	map<wstring,wstring> GetMap(void)
	{
		return m_map;
	}

protected:
    map<wstring,wstring> m_map;
};
