#pragma once

#ifndef _INC_SHLWAPI
#define NO_SHLWAPI_STRFCNS
#define NO_SHLWAPI_PATH
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_GDI
#include <shlwapi.h>
#endif

#include <vector>

#if !defined(_INC_SHLWAPI) || defined(NOSHLWAPI) || defined(NO_SHLWAPI_PATH)
BOOL WINAPI PathIsRelative(LPCTSTR pszPath);
BOOL WINAPI PathRemoveFileSpec(LPTSTR pszPath);
LPTSTR WINAPI PathFindExtension(LPCTSTR pszPath);
LPTSTR WINAPI PathAddBackslash(LPTSTR pszPath);
LPTSTR WINAPI PathCombine(LPTSTR pszDest, LPCTSTR pszDir, LPCTSTR pszFile);
#endif

/*
template <class T>
class CArray : public std::vector<T>
{
public:
	T* Begin() const
	{
		return m_aT;
	}
	T* End() const
	{
		return m_aT + m_nSize;
	}
};

template <class T>
class CValArray : public CSimpleValArray<T>
{
public:
	T* Begin() const
	{
		return m_aT;
	}
	T* End() const
	{
		return m_aT + m_nSize;
	}
};

template <class T>
class CPtrArray : public CValArray<T*>
{
};

template <class TKey, class TVal>
class CMap : public CSimpleMap<TKey, TVal>
{
};*/
