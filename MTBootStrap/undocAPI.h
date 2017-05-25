#pragma once

typedef struct _UNICODE_STRING2 {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING2;
typedef UNICODE_STRING2 *PUNICODE_STRING2;
typedef const UNICODE_STRING2 *PCUNICODE_STRING2;

typedef BOOL (__stdcall *PFNCreateProcessInternalW) 
( 
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
 LPPROCESS_INFORMATION lpProcessInformation , 
 PHANDLE hNewToken 
 ); 
typedef BOOL(WINAPI * PFNSetProcessMitigationPolicy)(
	_In_ PROCESS_MITIGATION_POLICY MitigationPolicy,
	_In_ PVOID                     lpBuffer,
	_In_ SIZE_T                    dwLength);

typedef BOOL(WINAPI * PFNUpdateProcThreadAttribute)(
	_Inout_ LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
	_In_ DWORD dwFlags,
	_In_ DWORD_PTR Attribute,
	_In_reads_bytes_opt_(cbSize) PVOID lpValue,
	_In_ SIZE_T cbSize,
	_Out_writes_bytes_opt_(cbSize) PVOID lpPreviousValue,
	_In_opt_ PSIZE_T lpReturnSize
	);

typedef LONG (WINAPI * PFNLdrLoadDll)(
						 IN PWCHAR               PathToFile OPTIONAL,
						 IN ULONG                Flags OPTIONAL,
						 IN UNICODE_STRING2*      ModuleFileName,
						 OUT HANDLE*             ModuleHandle 
						 );

static PFNCreateProcessInternalW CreateProcessInternalW_KernelBase = (PFNCreateProcessInternalW)GetProcAddress(GetModuleHandle(TEXT("kernelbase.dll")),"CreateProcessInternalW");
static PFNCreateProcessInternalW CreateProcessInternalW = CreateProcessInternalW_KernelBase ? CreateProcessInternalW_KernelBase : (PFNCreateProcessInternalW)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "CreateProcessInternalW");
static PFNLdrLoadDll LdrLoadDll=(PFNLdrLoadDll)GetProcAddress(LoadLibrary(L"ntdll.dll"), "LdrLoadDll");
//static PFNSetProcessMitigationPolicy MySetProcessMitigationPolicy = (PFNSetProcessMitigationPolicy)GetProcAddress(LoadLibrary(L"kernelbase.dll"), "SetProcessMitigationPolicy");
static PFNUpdateProcThreadAttribute MyUpdateProcThreadAttribute = (PFNUpdateProcThreadAttribute)GetProcAddress(LoadLibrary(L"kernelbase.dll"), "UpdateProcThreadAttribute");