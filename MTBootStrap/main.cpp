#include <windows.h>
#include <parseini.h>
#include <set>
#include "undocAPI.h"
#include "hook.h"
#include <xstring>
#include "supinfo.h"
#include "helper.h"
#include "wow64ext.h"

#ifdef _WIN64
#define MTDLL TEXT("MacType64.Core.dll")
#define EASYHKDLL TEXT("Easyhk64.dll")
#ifdef _DEBUG
#pragma comment(lib, "iniparser64_dbg.lib")
#else
#pragma comment(lib, "iniparser64.lib")
#endif
#else
#define MTDLL TEXT("MacType.Core.dll")
#define EASYHKDLL TEXT("Easyhk32.dll")
#ifdef _DEBUG
#pragma comment(lib, "iniparser_dbg.lib")
#else
#pragma comment(lib, "iniparser.lib")
#endif
#endif


HMODULE g_inst = NULL;
typedef set<wstring>	ModuleHashMap;
ModuleHashMap g_UnloadList, g_ExcludeList, g_IncludeList;
BOOL g_HookChildProc = false;
BOOL g_bDisableSign = false;
BOOL g_bUseInclude = false;
HMODULE g_hMacTypeDll = NULL;

//simple helper functions
void ChangeFileName(LPWSTR lpSrc, int nSize, LPCWSTR lpNewFileName) {
	for (int i = nSize; i > 0; --i){
		if (lpSrc[i] == L'\\') {
			lpSrc[i + 1] = L'\0';
			break;
		}
	}
	wcscat(lpSrc, lpNewFileName);
}

LPWSTR ExtractFileName(LPWSTR lpszFileName, int nSize) {
	for (int i = nSize; i > 0; --i){
		if (lpszFileName[i] == L'\\') {
			return lpszFileName + i+1;
		}
	}
	return lpszFileName;
}

BOOL WINAPI PathRemoveFileSpec(LPTSTR pszPath)
{
	if (!pszPath) {
		return FALSE;
	}
	LPTSTR p = pszPath + _tcslen(pszPath);

	while (p >= pszPath) {
		switch (*p) {
		case _T('\\'):
		case _T('/'):
			if (p > pszPath) {
				//c:\aaa -> c:\   <
				//c:\    -> c:\   <
				switch (*(p - 1)) {
				case _T('\\'):
				case _T('/'):
				case _T(':'):
					break;
				default:
					goto END;
				}
			}
		case _T(':'):
			// c:aaa -> c:
			p++;
			goto END;
		}
		if (p <= pszPath) {
			break;
		}
		--p;// CharPrev(NULL, p);
	}

END:
	if (*p) {
		*p = _T('\0');
		return TRUE;
	}
	return FALSE;
}

BOOL WINAPI PathIsRelative(LPCTSTR pszPath)
{
	if (!pszPath || !*pszPath) {
		return FALSE;
	}

	const TCHAR ch1 = pszPath[0];
	const TCHAR ch2 = pszPath[1];
	if (ch1 == _T('\\') && ch2 == _T('\\')) {
		//UNC
		return FALSE;
	}
	if (ch1 == _T('\\') || (ch1 && ch2 == _T(':'))) {
		//愨懳僷僗
		return FALSE;
	}
	return TRUE;
}

LPTSTR WINAPI PathCombine(LPTSTR pszDest, LPCTSTR pszDir, LPCTSTR pszFile)
{
	int nLen = wcslen(pszDir);
	if (nLen > MAX_PATH - 2) return NULL;
	WCHAR szTemp[MAX_PATH] = { 0 };
	wcscpy_s(szTemp, pszDir);
	if (szTemp[nLen] != L'\\') {
		szTemp[nLen] = L'\\';
		szTemp[nLen + 1] = L'\0';
	}
	wcscat_s(szTemp, pszFile);
	wcscpy_s(pszDest, MAX_PATH, szTemp);
	return pszDest;
}

void LoadIniSection(CParseIni &ini, LPCWSTR lpszSection, std::set<wstring>& list) {
	LPTSTR p = const_cast<LPWSTR>((LPCWSTR)ini[lpszSection]);
	while (*p) {
		bool b = false;
		set<wstring>::const_iterator it = list.find(p);
		if (it == list.end())
			list.insert(p);
		for (; *p; p++);	//next line
		p++;
	}
}

//profile parser
void ParseConfig() {
	WCHAR szFileName[MAX_PATH] = { 0 };
	int nSize = GetModuleFileName(g_inst, szFileName, MAX_PATH);
	if (nSize) {
		ChangeFileName(szFileName, nSize, TEXT("MacType.ini"));
		CParseIni ini;
		ini.LoadFromFile(szFileName);
		if (ini.IsPartExists(L"UnloadDll"))
			LoadIniSection(ini, L"UnloadDll", g_UnloadList);
		if (ini.IsPartExists(L"ExcludeModule"))
			LoadIniSection(ini, L"ExcludeModule", g_ExcludeList);
		if (ini.IsPartExists(L"IncludeModule"))
			LoadIniSection(ini, L"IncludeModule", g_IncludeList);
		g_HookChildProc = ini[L"General"][L"HookChildProcesses"].ToInt(0);
		g_bDisableSign = ini[L"Experimental"][L"DisableSign"].ToInt(0);
		g_bUseInclude = ini[L"General"][L"UseInclude"].ToInt(0);
		LPCWSTR lpAlter = (LPCWSTR)ini[L"General"][L"AlternativeFile"];
		if (lpAlter) {
			TCHAR szAlter[MAX_PATH] = { 0 };
			wcscpy_s(szAlter, lpAlter);
			CParseIni iniAlter;
			if (PathIsRelative(lpAlter)) {
				TCHAR szDir[MAX_PATH];
				wcsncpy(szDir, szFileName, MAX_PATH);
				PathRemoveFileSpec(szDir);
				PathCombine(szAlter, szDir, szAlter);
			}			
			iniAlter.LoadFromFile(szAlter);
			g_HookChildProc = iniAlter[L"General"][L"HookChildProcesses"].ToInt(0);
			g_bDisableSign = iniAlter[L"Experimental"][L"DisableSign"].ToInt(0);
			g_bUseInclude = iniAlter[L"General"][L"UseInclude"].ToInt(0);
			if (iniAlter.IsPartExists(L"UnloadDll")) 
				LoadIniSection(iniAlter, L"UnloadDll", g_UnloadList);
			if (iniAlter.IsPartExists(L"ExcludeModule"))
				LoadIniSection(iniAlter, L"ExcludeModule", g_ExcludeList);
			if (iniAlter.IsPartExists(L"IncludeModule"))
				LoadIniSection(iniAlter, L"IncludeModule", g_IncludeList);
			
		}
	}
}

bool IsProcessUnload() {
	ParseConfig();
	GetEnvironmentVariableW(L"MACTYPE_FORCE_LOAD", NULL, 0);	//env found, load anyway.
	if (GetLastError() != ERROR_ENVVAR_NOT_FOUND)
		return false;
	if (g_bUseInclude)
	{
		// include mode, only enable mactype for specified modules.
		ModuleHashMap::const_iterator it = g_IncludeList.begin();
		for (; it != g_IncludeList.end(); ++it) {
			if (GetModuleHandle(it->c_str())) {
				return false;
			}
		}
		return true;
	}
	else
	{
		// exclude mode, disable mactype in Unload list
		ModuleHashMap::const_iterator it = g_UnloadList.begin();
		for (; it != g_UnloadList.end(); ++it) {
			if (GetModuleHandle(it->c_str())) {
				return true;
			}
		}
	}
	return false;
}

bool IsProcessExclude() {
	//Excluded processes will load MacType.dll and hook its child processes, but won't load mactype.core.dll
	GetEnvironmentVariableW(L"MACTYPE_FORCE_LOAD", NULL, 0);	//env found, load anyway.
	if (GetLastError() != ERROR_ENVVAR_NOT_FOUND)
		return false;
	ModuleHashMap::const_iterator it = g_ExcludeList.begin();
	for (; it != g_ExcludeList.end(); ++it) {
		if (GetModuleHandle(it->c_str())) {
			return true;
		}
	}
	return false;
}

bool IsExeUnload(LPCTSTR lpApp)	//检查是否在黑名单列表内
{
	return false;
	GetEnvironmentVariableW(L"MACTYPE_FORCE_LOAD", NULL, 0);
	if (GetLastError() != ERROR_ENVVAR_NOT_FOUND)
		return false;
	if (g_bUseInclude)
	{
		// include mode, only enable mactype for specified modules.
		ModuleHashMap::const_iterator it = g_IncludeList.begin();
		for (; it != g_UnloadList.end(); ++it) {
			if (!lstrcmpi(lpApp, it->c_str())) {
				return false;
			}
		}
		return true;
	}
	else
	{
		// exclude mode, disable mactype in Unload list
		ModuleHashMap::const_iterator it = g_UnloadList.begin();
		for (; it != g_UnloadList.end(); ++it) {
			if (!lstrcmpi(lpApp, it->c_str())) {
				return true;
			}
		}
	}
	return false;
}

bool LoadMacType() {
	if (g_hMacTypeDll) return true;	// do nothing if already loaded.
	WCHAR dllpath[MAX_PATH];
	int nSize = 0;
	if (nSize = GetModuleFileName(g_inst, dllpath, MAX_PATH)) {
		ChangeFileName(dllpath, nSize, MTDLL);
		g_hMacTypeDll=LoadLibraryExW(dllpath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
		return !!g_hMacTypeDll;
	}
	else{
		return false;
	}
}

bool LoadEasyHook() {
	WCHAR dllpath[MAX_PATH];
	int nSize = 0;
	if (nSize = GetModuleFileName(g_inst, dllpath, MAX_PATH)) {
		ChangeFileName(dllpath, nSize, EASYHKDLL);
		return !!LoadLibraryExW(dllpath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	}
	else{
		return false;
	}
}

class CCriticalSectionLock
{
private:
	static CRITICAL_SECTION m_cs;
	int m_index;
public:
	CCriticalSectionLock()
	{
		::EnterCriticalSection(&m_cs);
	}
	~CCriticalSectionLock()
	{
		::LeaveCriticalSection(&m_cs);
	}
	static void Init()
	{
		::InitializeCriticalSection(&m_cs);
	}
	static void Term()
	{
		::DeleteCriticalSection(&m_cs);
	}
};

CRITICAL_SECTION CCriticalSectionLock::m_cs;

class CThreadCounter
{
private:
	static LONG interlock;
public:
	CThreadCounter()
	{
		InterlockedIncrement(&interlock);
	}
	~CThreadCounter()
	{
		InterlockedDecrement(&interlock);
	}
	static void Init()
	{
		interlock = 0;
	}
	static int Count()
	{
		return InterlockedExchange(&interlock, interlock);
	}
};

LONG CThreadCounter::interlock = 0;


int __stdcall DllMain(HMODULE hDllHandle, DWORD dwReason, LPVOID lpreserved)
{
	static bool g_bIsUnload = false;
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
	{
		//Sleep(30 * 1000);
		g_inst = hDllHandle;
		DisableThreadLibraryCalls(hDllHandle);
		if (GetModuleHandle(MTDLL) != 0) {	//always load if Mactype is loaded
			return true;
		}
		if (g_bIsUnload = IsProcessUnload()) return false;	//do not load if process is marked as unload
#ifndef MHOOK
		if (!LoadEasyHook()) return false;
#endif

/*
		if (/ *g_bDisableSign &&* / NULL != MyUpdateProcThreadAttribute) {
			ORIG_MyUpdateProcThreadAttribute = MyUpdateProcThreadAttribute;
			hook_demand_MyUpdateProcThreadAttribute(false);
		}*/
		if (g_HookChildProc)
		{
#ifndef _WIN64
			InitWow64ext();
#endif
			ORIG_CreateProcessInternalW = CreateProcessInternalW;
			hook_demand_CreateProcessInternalW(false);

			/*if (g_bDisableSign && NULL != MySetProcessMitigationPolicy) {
				ORIG_MySetProcessMitigationPolicy = MySetProcessMitigationPolicy;
				hook_demand_MySetProcessMitigationPolicy(false);
			}*/
		}
		if (!IsProcessExclude()) {	// do not load mactype core when excluded
			if (GetModuleHandle(L"gdi32.dll") == 0) {
				ORIG_LdrLoadDll = LdrLoadDll;
				hook_demand_LdrLoadDll(false);	//monitor gdi32.dll loading
			}
			else{
				return LoadMacType();
			}
		}
		return true;
	}
	case DLL_PROCESS_DETACH:
	{
		if (!g_bIsUnload) {
			if (g_hMacTypeDll && !lpreserved) {	// only freelibrary when freelibrary is called.
				while (GetModuleHandle(MTDLL) == g_hMacTypeDll)
					FreeLibrary(g_hMacTypeDll);	// free mactype.core.dll completely
			}
			hook_term();
		}
		return true;
	}

	default:	
		return true;
	}
}

bool bHook = true;

LONG WINAPI IMPL_LdrLoadDll(IN PWCHAR  PathToFile OPTIONAL,
	IN ULONG                Flags OPTIONAL,
	IN UNICODE_STRING2*      ModuleFileName,
	OUT HANDLE*             ModuleHandle)
{
	static bool bLoaded = false;
	LONG r = ORIG_LdrLoadDll(PathToFile, Flags, ModuleFileName, ModuleHandle);
	//LPWSTR szDll = ExtractFileName(ModuleFileName->Buffer, ModuleFileName->Length);
/*
	if (bHook) {
		bHook = false;
		MessageBoxW(0, szDll, szDll, 0);
		bHook = true;
	}*/
		
	//if (!bLoaded && (_wcsicmp(szDll, L"gdi32.dll") == 0 || _wcsicmp(szDll, L"gdi32") == 0)) {
		//unhook_LdrLoadDll();
	if (!bLoaded && GetModuleHandleW(L"gdi32.dll") != 0) {
		bLoaded = true;
		LoadMacType();
	}
	return r;
}

BOOL __stdcall IMPL_CreateProcessInternalW(
	HANDLE hToken,
	LPCTSTR lpApplicationName,
	LPTSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCTSTR lpCurrentDirectory,
	LPSTARTUPINFO lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation,
	PHANDLE hNewToken
	){
	return _CreateProcessInternalW(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation, hNewToken, ORIG_CreateProcessInternalW);
}

BOOL WINAPI IMPL_MyUpdateProcThreadAttribute(_Inout_ LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList, _In_ DWORD dwFlags, _In_ DWORD_PTR Attribute, _In_reads_bytes_opt_(cbSize) PVOID lpValue, _In_ SIZE_T cbSize, _Out_writes_bytes_opt_(cbSize) PVOID lpPreviousValue, _In_opt_ PSIZE_T lpReturnSize)
{
	if (Attribute == PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY)
		return true;
		//*PDWORD_PTR(lpValue) = *PDWORD_PTR(lpValue) & ~PROCESS_CREATION_MITIGATION_POLICY_BLOCK_NON_MICROSOFT_BINARIES_ALWAYS_ON | PROCESS_CREATION_MITIGATION_POLICY_BLOCK_NON_MICROSOFT_BINARIES_ALWAYS_OFF;
	return ORIG_MyUpdateProcThreadAttribute(lpAttributeList, dwFlags, Attribute, lpValue, cbSize, lpPreviousValue, lpReturnSize);
}