#include "stdafx.h"
#include "JavaPathBuilder.h"

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
JavaPathBuilder::JavaPathBuilder(void)
{
	quotechar='"';
	javaExec.assign(L"java.exe");
}

JavaPathBuilder::JavaPathBuilder(wstring& java_home_path, wstring& java_lib_path)
{
	javaHomePath=java_home_path;
	javaLibPath=java_lib_path;
}

JavaPathBuilder::JavaPathBuilder(wstring& java_home_path, wstring& java_lib_path,wstring& nsislib_jar)
{
	JavaPathBuilder(java_home_path, java_lib_path);
	javaLib = nsislib_jar;
}

JavaPathBuilder::~JavaPathBuilder(void)
{
}

wstring JavaPathBuilder::BuildJavaCmdLine(wstring& java_home_path, wstring& java_lib_path,wstring& nsislib_jar, const wstring& java_cmd)
{
	javaCmdLineToUse << quotechar
						<< java_home_path
						<< javaExec
						<< quotechar
						<< L" "
						<< L"-classpath "
						<< java_lib_path
						<< L"\\*"
						<< L" "
						<< L"-Djava.library.path="
						<< java_lib_path
						<< L";"
						<< java_lib_path
						<< nsislib_jar
						<< L" "
						<< java_cmd
						<< L"\0\0";
						
	return javaCmdLineToUse.str();
}

wstring JavaPathBuilder::BuildJavaCmdLine(wstring& nsislib_jar, wstring& java_cmd)
{
	javaCmdLineToUse << quotechar
						<< javaHomePath
						<< javaExec
						<< quotechar
						<< L" "
						<< L"-classpath "
						<< javaLibPath << L"\\*"
						<< L" "
						<< L"-Djava.library.path="
						<< javaLibPath
						<< L";"
						<< javaLibPath
						<< nsislib_jar
						<< L" "
						<< java_cmd
						<< L"\0\0";
						
	return javaCmdLineToUse.str();
}

void JavaPathBuilder::BuildJavaCmdLine(LPCTSTR szJavaHomePath,LPCTSTR szJavaLibPath, LPCTSTR szNsisLib,LPCTSTR szJavaCmd)
{
	javaCmdLineToUse << quotechar
						<< szJavaHomePath
						<< javaExec
						<< quotechar
						<< L" "
						<< L"-classpath "
						<< szJavaLibPath << L"\\*"
						<< L" "
						<< L"-Djava.library.path="
						<< szNsisLib
						<< L";"
						<< szJavaLibPath
						<< szNsisLib
						<< L" "
						<< szJavaCmd
						<<'\n'<<'\0';				
}

wstring JavaPathBuilder::GetJavaCmdLine(void)
{
	return javaCmdLineToUse.str();
}

wstring JavaPathBuilder::BuildJavaCmdLine(wstring& java_cmd)
{

	javaCmdLineToUse << quotechar
						<< javaHomePath
						<< javaExec
						<< quotechar
						<< L" "
						<< L"-classpath "
						<< javaLibPath << L"\\*"
						<< L" "
						<< L"-Djava.library.path="
						<< javaLibPath
						<< L";"
						<< javaLibPath
						<< javaLib
						<< L" "
						<< java_cmd;
						
	return javaCmdLineToUse.str();
}