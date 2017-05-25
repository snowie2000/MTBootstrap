#pragma once

//嶲徠
//http://www.catch22.net/tuts/undoc01.asp

extern BOOL WINAPI GdippInjectDLL(const PROCESS_INFORMATION* ppi);
extern LPWSTR WINAPI GdippEnvironment(DWORD& dwCreationFlags, LPVOID lpEnvironment);

static wstring GetExeName(LPCTSTR lpApp, LPTSTR lpCmd)
{
// 	HANDLE logfile = CreateFile(_T("C:\\mt.log"), FILE_ALL_ACCESS, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, NULL, NULL);
// 	SetFilePointer(logfile,0,NULL, FILE_END);

	wstring ret;
// 	DWORD aa=0;
// 	if (GetFileSize(logfile, NULL)==0)
// 		WriteFile(logfile, "\xff\xfe", 2, &aa, NULL);
	LPTSTR vlpApp = (LPTSTR)lpApp;	//变成可以操作的参数
	if (lpApp)
	{
		do 
		{
// 			WriteFile(logfile, L"lpApp=", 12, &aa, NULL);
// 			WriteFile(logfile, lpApp, _tcslen(lpApp)*2, &aa, NULL);
// 			WriteFile(logfile, _T("\n"), 2, &aa, NULL);
			vlpApp = _tcsstr(vlpApp+1, _T(" "));	//获得第一个空格所在的位置
			ret.assign(lpApp);
			if (vlpApp)
				ret.resize(vlpApp-lpApp);
// 			WriteFile(logfile, ret.c_str(), ret.length()*2, &aa, NULL);
// 			WriteFile(logfile, _T("\n"), 2, &aa, NULL);
			DWORD fa = GetFileAttributes(ret.c_str()); 
			if (fa!=INVALID_FILE_ATTRIBUTES && fa!=FILE_ATTRIBUTE_DIRECTORY)	//文件是否存在
			{		
				int p = ret.find_last_of(_T("\\"));
				if (p!=-1)
					ret.erase(0, p+1);	//如果有路径就删掉路径
// 				WriteFile(logfile, ret.c_str(), ret.length()*2, &aa, NULL);
// 				WriteFile(logfile, _T("\n"), 2, &aa, NULL);
// 				WriteFile(logfile, _T("==========\n"), 24, &aa, NULL);
// 				CloseHandle(logfile);
				return ret;
			}
			else
			{
				ret+=_T(".exe");	//加上.exe扩展名再试
				DWORD fa = GetFileAttributes(ret.c_str()); 
				if (fa!=INVALID_FILE_ATTRIBUTES && fa!=FILE_ATTRIBUTE_DIRECTORY)
				{		
					int p = ret.find_last_of(_T("\\"));
					if (p!=-1)
						ret.erase(0, p+1);	//如果有路径就删掉路径
// 					WriteFile(logfile, ret.c_str(), ret.length()*2, &aa, NULL);
// 					WriteFile(logfile, _T("\n"), 2, &aa, NULL);
// 					WriteFile(logfile, _T("==========\n"), 24, &aa, NULL);
// 					CloseHandle(logfile);
					return ret;
				}
			}
		} while (vlpApp);
	}

	if (lpCmd)
	{
// 		WriteFile(logfile, L"lpCmd=", 10, &aa, NULL);
// 		WriteFile(logfile, lpCmd, _tcslen(lpCmd)*2, &aa, NULL);
		ret.assign(lpCmd);
		int p=0;
		if ((*lpCmd)==_T('\"'))
		{
			ret.erase(0,1);	//删除第一个引号
			p=ret.find_first_of(_T("\""));	//查找下一个引号
		}
		else
			p=ret.find_first_of(_T(" "));
		if (p>0)
			ret.resize(p);	//获得Cmd里面的文件名
// 		WriteFile(logfile, ret.c_str(), ret.length()*2, &aa, NULL);
// 		WriteFile(logfile, _T("\n"), 2, &aa, NULL);
		p = ret.find_last_of(_T("\\"));
		if (p>0)
			ret.erase(0, p+1);	//如果有路径就删掉路径
// 		WriteFile(logfile, ret.c_str(), ret.length()*2, &aa, NULL);
// 		WriteFile(logfile, _T("\n"), 2, &aa, NULL);
// 		WriteFile(logfile, _T("==========\n"), 24, &aa, NULL);
// 		CloseHandle(logfile);
		return ret;
	}
// 	WriteFile(logfile, ret.c_str(), ret.length()*2, &aa, NULL);
// 	WriteFile(logfile, _T("\n"), 2, &aa, NULL);
// 	WriteFile(logfile, _T("==========\n"), 24, &aa, NULL);
// 	CloseHandle(logfile);
	return ret;
}

template <class _Function>
BOOL _CreateProcessInternalW(HANDLE hToken, LPCTSTR lpApp, LPTSTR lpCmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL bInherit, \
							 DWORD dwFlags, LPVOID lpEnv, LPCTSTR lpDir, LPSTARTUPINFO psi, LPPROCESS_INFORMATION ppi , PHANDLE hNewToken, _Function fn)
{
#ifdef _GDIPP_RUN_CPP
	const bool hookCP = true;
	const bool runGdi = true;
#else
	const bool hookCP = !!g_HookChildProc;
	const bool runGdi = false;
#endif

	wstring exe_name = GetExeName(lpApp, lpCmd);
	if (!hookCP || (!lpApp && !lpCmd) || (dwFlags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS)) || IsExeUnload(exe_name.c_str())) {
		return fn(hToken, lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi, hNewToken);
	}
	LPWSTR pEnvW = GdippEnvironment(dwFlags, lpEnv);
	if (pEnvW) {
		lpEnv = pEnvW;
	}
	
	if (!fn(hToken, lpApp, lpCmd, NULL, NULL, bInherit, dwFlags | CREATE_SUSPENDED, lpEnv, lpDir, psi, ppi, hNewToken)) {
		free(pEnvW);
		return FALSE;
	}
	GdippInjectDLL(ppi);
	if (!(dwFlags & CREATE_SUSPENDED)) {
		ResumeThread(ppi->hThread);
	}
	free(pEnvW);

	return TRUE;
}