#include <windows.h>

extern HMODULE g_inst;
extern BOOL g_HookChildProc;

//simple helper functions
void ChangeFileName(LPWSTR lpSrc, int nSize, LPCWSTR lpNewFileName);

LPWSTR ExtractFileName(LPWSTR lpszFileName, int nSize);

bool IsExeUnload(LPCTSTR lpApp);

BOOL WINAPI PathIsRelative(LPCTSTR pszPath);
BOOL WINAPI PathRemoveFileSpec(LPTSTR pszPath);
LPTSTR WINAPI PathFindExtension(LPCTSTR pszPath);
LPTSTR WINAPI PathAddBackslash(LPTSTR pszPath);
LPTSTR WINAPI PathCombine(LPTSTR pszDest, LPCTSTR pszDir, LPCTSTR pszFile);