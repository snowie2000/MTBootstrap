#ifndef _GDIPP_EXE
#include "hook.h"
#include "undocAPI.h"
#include "wow64ext.h"
#include <vector>
#include <VersionHelpers.h>
using namespace std;

// win2k以降
//#pragma comment(linker, "/subsystem:windows,5.0")
#ifndef _WIN64
#ifdef _DEBUG
#pragma comment(lib, "wow64ext_dbg.lib")
#else
#pragma comment(lib, "wow64ext.lib")
#endif
#endif


EXTERN_C LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam)
{
	//何もしない
	return CallNextHookEx(NULL, code, wParam, lParam);
}
#endif	//!_GDIPP_EXE

extern LONG interlock;
extern LONG g_bHookEnabled;
extern HMODULE g_hMacTypeDll;

typedef void (WINAPI *procCreateControlCenter)(void** ret);
typedef void(WINAPI *procReloadConfig)();


HMODULE GetSelfModuleHandle()
{
	MEMORY_BASIC_INFORMATION mbi;

	return ((::VirtualQuery(GetSelfModuleHandle, &mbi, sizeof(mbi)) != 0) 
		? (HMODULE) mbi.AllocationBase : NULL);
}


EXTERN_C void WINAPI CreateControlCenter(void** ret)
{
	if (!g_hMacTypeDll) {
		*ret = NULL;
	}
	else {
		procCreateControlCenter ccc = (procCreateControlCenter)GetProcAddress(g_hMacTypeDll, "CreateControlCenter");
		if (ccc) {
			ccc(ret);
		}
	}
}

EXTERN_C void WINAPI ReloadConfig()
{
	if (!g_hMacTypeDll) {
		return;
	}
	else {
		procReloadConfig rc = (procReloadConfig)GetProcAddress(g_hMacTypeDll, "ReloadConfig");
		if (rc) {
			rc();
		}
	}
}

/*

extern HINSTANCE g_dllInstance;
EXTERN_C void SafeUnload()
{
	static BOOL bInited = false;
	if (bInited)
		return;	//ｷﾀﾖﾘﾈ�
	bInited = true;
	while (CThreadCounter::Count())
		Sleep(0);
	CCriticalSectionLock * lock = new CCriticalSectionLock;
	BOOL last;
	if (last=InterlockedExchange(&g_bHookEnabled, FALSE)) {
		if (hook_term()!=NOERROR)
		{
			InterlockedExchange(&g_bHookEnabled, last);
			bInited = false;
			delete lock;
			ExitThread(ERROR_ACCESS_DENIED);
		}
	}
	delete lock;
	while (CThreadCounter::Count())
		Sleep(10);
	Sleep(0);
	do 
	{
		Sleep(10);
	} while (CThreadCounter::Count());	//double check for xp
		
	bInited = false; 
	FreeLibraryAndExitThread(g_dllInstance, 0);
}*/

#include "helper.h"
#include <strsafe.h>
#include "dll.h"

//kernel32専用GetProcAddressモドキ
FARPROC K32GetProcAddress(LPCSTR lpProcName)
{
#ifndef _WIN64
	//序数渡しには対応しない
	//Assert(!IS_INTRESOURCE(lpProcName));

	//kernel32のベースアドレス取得
	LPBYTE pBase = (LPBYTE)GetModuleHandleA("kernel32.dll");

	//この辺は100%成功するはずなのでエラーチェックしない
	PIMAGE_DOS_HEADER pdosh = (PIMAGE_DOS_HEADER)pBase;
	//Assert(pdosh->e_magic == IMAGE_DOS_SIGNATURE);
	PIMAGE_NT_HEADERS pnth = (PIMAGE_NT_HEADERS)(pBase + pdosh->e_lfanew);
	//Assert(pnth->Signature == IMAGE_NT_SIGNATURE);

	const DWORD offs = pnth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	const DWORD size = pnth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	if (offs == 0 || size == 0) {
		return NULL;
	}

	PIMAGE_EXPORT_DIRECTORY pdir = (PIMAGE_EXPORT_DIRECTORY)(pBase + offs);
	DWORD*	pFunc = (DWORD*)(pBase + pdir->AddressOfFunctions);
	WORD*	pOrd  = (WORD*)(pBase + pdir->AddressOfNameOrdinals);
	DWORD*	pName = (DWORD*)(pBase + pdir->AddressOfNames);

	for(DWORD i=0; i<pdir->NumberOfFunctions; i++) {
		for(DWORD j=0; j<pdir->NumberOfNames; j++) {
			if(pOrd[j] != i)
				continue;

			if(strcmp((LPCSTR)pBase + pName[j], lpProcName) != 0)
				continue;

			return (FARPROC)(pBase + pFunc[i]);
		}
	}
	return NULL;
#else
	//Assert(!IS_INTRESOURCE(lpProcName));

	//kernel32のベースアドレス取得
	WCHAR sysdir[MAX_PATH];
	GetWindowsDirectory(sysdir, MAX_PATH);
	if (GetModuleHandle(TEXT("kernelbase.dll")))	//ｲ鯀ｴﾗﾔｼｺﾊﾇｷ�ｼﾓﾔﾘﾁﾋKernelbase.dllﾎﾄｼ�｣ｬｴ贇ﾚﾔ�ﾋｵﾃ�ﾊﾇwin7ﾏｵﾍｳ
		wcscat(sysdir, L"\\SysWow64\\kernelbase.dll");
	else
		wcscat(sysdir, L"\\SysWow64\\kernel32.dll");	//ｲｻｴ贇ﾚｾﾍﾊﾇvista
	HANDLE hFile = CreateFile(sysdir, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;
	DWORD dwSize = GetFileSize(hFile, NULL);
	BYTE* pMem = new BYTE[dwSize];	//ｷﾖﾅ萋ﾚｴ�
	ReadFile(hFile, pMem, dwSize, &dwSize, NULL);//ｶﾁﾈ｡ﾎﾄｼ�
	CloseHandle(hFile);

	CMemLoadDll MemDll;
	MemDll.MemLoadLibrary(pMem, dwSize, false, false);
	delete[] pMem;
	return FARPROC((DWORD_PTR)MemDll.MemGetProcAddress(lpProcName)-MemDll.GetImageBase());	//ｷｵｻﾘﾆｫﾒﾆﾖｵ

#endif
}

typedef struct _UNICODE_STRING64 {
	USHORT Length;
	USHORT MaximumLength;
	DWORD64  Buffer;
} UNICODE_STRING64, *PUNICODE_STRING64;

#include <pshpack1.h>
class opcode_data {
private:
	BYTE	code[0x100];

	//注: dllpathをWORD境界にしないと場合によっては正常に動作しない
	WCHAR	dllpath[MAX_PATH];
	UNICODE_STRING64 uniDllPath;
	DWORD64 hDumyDllHandle;

public:
	opcode_data()
	{
		//int 03hで埋める
		FillMemory(this, sizeof(*this), 0xcc);
	}
	bool initWow64(LPDWORD remoteaddr, LONG orgEIP)	//Wow64ｳ�ﾊｼｻｯ
	{
		//WORD境界チェック
		C_ASSERT((offsetof(opcode_data, dllpath) & 1) == 0);

		register BYTE* p = code;

#define emit_(t,x)	*(t* UNALIGNED)p = (t)(x); p += sizeof(t)
#define emit_db(b)	emit_(BYTE, b)
#define emit_dw(w)	emit_(WORD, w)
#define emit_dd(d)	emit_(DWORD, d)

		//なぜかGetProcAddressでLoadLibraryWのアドレスが正しく取れないことがあるので
		//kernel32のヘッダから自前で取得する
		FARPROC pfn = K32GetProcAddress("LoadLibraryExW");
		if(!pfn)
			return false;

		emit_db(0x60);		//pushad

		/*
			mov eax,fs:[0x30]
			mov eax,[eax+0x0c]
			mov esi,[eax+0x1c]
			lodsd
			move ax,[eax+$08]//ﾕ篋�ﾊｱｺ�eaxﾖﾐｱ｣ｴ豬ﾄｾﾍﾊﾇk32ｵﾄｻ�ﾖｷﾁﾋ
			ﾔﾚwin7ｻ�ｵﾃｵﾄﾊﾇKernelBase.dllｵﾄｵﾘﾖｷ
		*/
		emit_db(0x64); 
		emit_db(0xA1); 
		emit_db(0x30); 
		emit_db(00); 
		emit_db(00); 
		emit_db(00); 
		emit_db(0x8B); 
		emit_db(0x40); 
		emit_db(0x0C); 
		emit_db(0x8B); 
		emit_db(0x70); 
		emit_db(0x1C); 
		emit_db(0xAD); 
		emit_db(0x8B); 
		emit_db(0x40);
		emit_db(0x08);		//use assemble to fetch kernel base

		emit_dw(0x006A);	//push 0
		emit_dw(0x006A);	//push 0
		emit_db(0x68);		//push dllpath
		emit_dd((LONG)remoteaddr + offsetof(opcode_data, dllpath));
		emit_db(0x05);		//add eax, LoadLibraryExW offset
		emit_dd(pfn);
		emit_dw(0xD0FF);	//call eax

		emit_db(0x61);		//popad
		emit_db(0xE9);		//jmp original_EIP
		emit_dd(orgEIP - (LONG)remoteaddr - (p - code) - sizeof(LONG));

		// gdi++.dllのパス
		int nSize = GetModuleFileNameW(g_inst, dllpath, MAX_PATH);
		if (nSize) {
			ChangeFileName(dllpath, nSize, L"MacType.dll");
		}
		return !!nSize;
	}
	bool init32(LPDWORD remoteaddr, LONG orgEIP)	//32ﾎｻｳﾌﾐ�ｳ�ﾊｼｻｯ
	{
		//WORD境界チェック
		C_ASSERT((offsetof(opcode_data, dllpath) & 1) == 0);

		register BYTE* p = code;

#define emit_(t,x)	*(t* UNALIGNED)p = (t)(x); p += sizeof(t)
#define emit_db(b)	emit_(BYTE, b)
#define emit_dw(w)	emit_(WORD, w)
#define emit_dd(d)	emit_(DWORD, d)

		//なぜかGetProcAddressでLoadLibraryWのアドレスが正しく取れないことがあるので
		//kernel32のヘッダから自前で取得する
		FARPROC pfn = K32GetProcAddress("LoadLibraryW");
		if(!pfn)
			return false;

		emit_db(0x60);		//pushad
		/*
#if _DEBUG
emit_dw(0xC033);	//xor eax, eax
emit_db(0x50);		//push eax
emit_db(0x50);		//push eax
emit_db(0x68);		//push dllpath
emit_dd((LONG)remoteaddr + offsetof(opcode_data, dllpath));
emit_db(0x50);		//push eax
emit_db(0xB8);		//mov eax, MessageBoxW
emit_dd((LONG)MessageBoxW);
emit_dw(0xD0FF);	//call eax
#endif*/

		emit_db(0x68);		//push dllpath
		emit_dd((LONG)remoteaddr + offsetof(opcode_data, dllpath));
		emit_db(0xB8);		//mov eax, LoadLibraryW
		emit_dd(pfn);
		emit_dw(0xD0FF);	//call eax

		emit_db(0x61);		//popad
		emit_db(0xE9);		//jmp original_EIP
		emit_dd(orgEIP - (LONG)remoteaddr - (p - code) - sizeof(LONG));

		// gdi++.dllのパス
		int nSize = GetModuleFileNameW(g_inst, dllpath, MAX_PATH);
		if (nSize) {
			ChangeFileName(dllpath, nSize, L"MacType.dll");
		}
		return !!nSize;
	}
	bool init64From32(DWORD64 remoteaddr, DWORD64 orgEIP)
	{
		C_ASSERT((offsetof(opcode_data, dllpath) & 1) == 0);

		register BYTE* p = code;

#define emit_(t,x)	*(t* UNALIGNED)p = (t)(x); p += sizeof(t)
#define emit_db(b)	emit_(BYTE, b)
#define emit_dw(w)	emit_(WORD, w)
#define emit_dd(d)	emit_(DWORD, d)
#define emit_ddp(dp) emit_(DWORD64, dp)

		//なぜかGetProcAddressでLoadLibraryWのアドレスが正しく取れないことがあるので
		//kernel32のヘッダから自前で取得する
		WCHAR x64Addr[30] = { 0 };
		if (!GetEnvironmentVariable(L"MACTYPE_X64ADDR", x64Addr, 29)) return false;
		DWORD64 pfn = wcstoull(x64Addr, NULL, 10);
		//DWORD64 pfn = getenv("MACTYPE_X64ADDR"); //GetProcAddress64(GetModuleHandle64(L"kernelbase.dll"), "LoadLibraryW");
		if (!pfn)
			return false;

		emit_db(0x50);		//push rax
		emit_db(0x51);		//push rcx
		emit_db(0x52);		//push rdx
		emit_db(0x53);		//push rbx
		emit_dd(0x28ec8348);	//sub rsp,28h

		emit_db(0x48);		//mov rcx, dllpath
		emit_db(0xB9);
		emit_ddp((DWORD64)remoteaddr + offsetof(opcode_data, dllpath));
		emit_db(0x48);		//mov rsi, LoadLibraryW
		emit_db(0xBE);
		emit_ddp(pfn);
		//emit_db(0x48);
		emit_db(0xFF);	//call rdi
		emit_db(0xD6);

		emit_dd(0x28c48348);	//add rsp,28h
		emit_db(0x5B);
		emit_db(0x5A);
		emit_db(0x59);
		emit_db(0x58);		//popad		

		emit_db(0x48);		//mov rdi, orgRip
		emit_db(0xBE);
		emit_ddp(orgEIP);
		emit_db(0xFF);		//jmp rdi
		emit_db(0xE6);

		// gdi++.dllのパス

		int nSize = GetModuleFileNameW(g_inst, dllpath, MAX_PATH);
		if (nSize) {
			ChangeFileName(dllpath, nSize, L"MacType64.dll");
		}
		return !!nSize;
	}

	bool init64From32(DWORD64 remoteaddr, DWORD64 orgEIP, DWORD dwLoaderOffset)
	{
		C_ASSERT((offsetof(opcode_data, dllpath) & 1) == 0);

		int nSize = GetModuleFileNameW(g_inst, dllpath, MAX_PATH);
		if (nSize) {
			ChangeFileName(dllpath, nSize, L"MacType64.dll");
		}
		if (!nSize)
			return false;
		uniDllPath.Length = wcslen(dllpath)*sizeof(WCHAR);
		uniDllPath.MaximumLength = uniDllPath.Length+2;
		uniDllPath.Buffer = remoteaddr + (DWORD64)offsetof(opcode_data, dllpath);	//prepare PUNICODE_STRING for remote process
		register BYTE* p = code;

#define emit_(t,x)	*(t* UNALIGNED)p = (t)(x); p += sizeof(t)
#define emit_db(b)	emit_(BYTE, b)
#define emit_dw(w)	emit_(WORD, w)
#define emit_dd(d)	emit_(DWORD, d)
#define emit_ddp(dp) emit_(DWORD64, dp)

//get ntdll.dll imagebase
//credit to http://www.52pojie.cn/thread-162625-1-1.html
/*asm:
	mov rsi, [gs:60h]   ;     peb from teb
	mov rsi, [rsi+18h]    ;_peb_ldr_data from peb
	mov rsi, [rsi+30h]   ;InInitializationOrderModuleList.Flink, ntdll.dll
	;mov rsi, [rsi]  ;kernelbase.dll
	;mov rsi, [rsi]      ;kernel32.dll (not used for win7+)
	mov rsi, [rsi+10h]
*/

// emit_db(0xEB);
// emit_db(0xFE);	// make a dead loop
		emit_db(0x65);
		emit_db(0x48);
		emit_db(0x8B);
		emit_db(0x34);
		emit_db(0x25);
		emit_db(0x60);
		emit_db(0x00);
		emit_db(0x00);
		emit_db(0x00);
		emit_db(0x48);
		emit_db(0x8B);
		emit_db(0x76);
		emit_db(0x18);
		emit_db(0x48);
		emit_db(0x8B);
		emit_db(0x76);
		emit_db(0x30);
// 		emit_db(0x48);
// 		emit_db(0x8B);
// 		emit_db(0x36);
		emit_db(0x48);
		emit_db(0x8B);
		emit_db(0x76);
		emit_db(0x10);
//rsi = ntdll.dll baseaddress

		emit_db(0x50);		//push rax
		emit_db(0x51);		//push rcx
		emit_db(0x52);		//push rdx
		emit_db(0x53);		//push rbx
		emit_dd(0x28ec8348);	//sub rsp,28h
		emit_db(0x48);
		emit_db(0x31);
		emit_db(0xc9);	//xor rcx, rcx
		emit_db(0x48);
		emit_db(0x31);
		emit_db(0xd2);	//xor rdx, rdx
		emit_db(0x49);		
		emit_db(0xB8);
		emit_ddp((DWORD64)remoteaddr + offsetof(opcode_data, uniDllPath));//mov r8, uniDllPath
		emit_db(0x49);
		emit_db(0xB9);
		emit_ddp((DWORD64)remoteaddr + offsetof(opcode_data, hDumyDllHandle));//mov r9, hDumyDllHandle
		//emit_db(0x48);		//mov rsi, LdrLoadDll
		//emit_db(0xBE);
		emit_db(0x48);
		emit_db(0x81);
		emit_db(0xC6);	//add rsi, offset LdrLoadDll
		emit_dd(dwLoaderOffset);
		//emit_db(0x48);
		emit_db(0xFF);	//call rsi
		emit_db(0xD6);

		emit_dd(0x28c48348);	//add rsp,28h
		emit_db(0x5B);
		emit_db(0x5A);
		emit_db(0x59);
		emit_db(0x58);		//popad		

		emit_db(0x48);		//mov rdi, orgRip
		emit_db(0xBE);
		emit_ddp(orgEIP);
		emit_db(0xFF);		//jmp rdi
		emit_db(0xE6);

		// gdi++.dllのパス

		return !!nSize;
	}

	bool init(DWORD_PTR* remoteaddr, DWORD_PTR orgEIP)
	{
		//WORD境界チェック
		C_ASSERT((offsetof(opcode_data, dllpath) & 1) == 0);

		register BYTE* p = code;
#undef emit_ddp

#define emit_(t,x)	*(t* UNALIGNED)p = (t)(x); p += sizeof(t)
#define emit_db(b)	emit_(BYTE, b)
#define emit_dw(w)	emit_(WORD, w)
#define emit_dd(d)	emit_(DWORD, d)
#define emit_ddp(dp) emit_(DWORD_PTR, dp)

		//なぜかGetProcAddressでLoadLibraryWのアドレスが正しく取れないことがあるので
		//kernel32のヘッダから自前で取得する
		FARPROC pfn = (FARPROC)((INT_PTR)CDllHelper::MyGetProcAddress(GetModuleHandle(L"kernel32.dll"), L"LoadLibraryW") - (INT_PTR)GetModuleHandle(L"kernel32.dll"));
		/*WCHAR msg[500] = { 0 };
		wsprintf(msg, L"API paddr: 0x%I64x\r\nOffset: %x\r\nAPI addr: 0x%I64x\r\nKernel32.dll: 0x%I64x\r\nKernelBase: 0x%I64x", (DWORD_PTR)pfn, *(PDWORD)pfn, *(PDWORD)pfn + (DWORD_PTR)GetModuleHandle(L"kernel32.dll"),
			(DWORD_PTR)GetModuleHandle(L"kernel32.dll"), (DWORD_PTR)GetModuleHandle(L"kernelbase.dll"));
		MessageBoxW(NULL, msg, NULL, MB_OK);*/
		//if(!pfn)
		//	return false;
		//emit_db(0xEB);
		//emit_db(0xFE);	// make a dead loop

		emit_db(0x50);		//push rax
		emit_db(0x51);		//push rcx
		emit_db(0x52);		//push rdx
		emit_db(0x53);		//push rbx
		/*
#ifdef DEBUG
		emit_dd(0x28ec8348);
		emit_db(0x48);
		emit_db(0x31);
		emit_db(0xD0);	//xor rax,rax
		emit_db(0x48);
		emit_db(0x31);
		emit_db(0xC9);	//xor rcx,rcx
		emit_db(0x48);
		emit_db(0x31);
		emit_db(0xD2);	//xor rdx,rdx
		emit_db(0x45);
		emit_db(0x31);
		emit_db(0xC0);	//xor r8d,r8d
		emit_db(0x45);
		emit_db(0x31);
		emit_db(0xC9);	//xor r9d,r9d

		emit_db(0x48);		//mov rsi, MessageBoxW
		emit_db(0xBE);
		emit_ddp((DWORD_PTR)MessageBoxW);
		emit_db(0xFF);
		emit_db(0xD6);
		emit_dd(0x28c48348);
#endif*/
/*	
//Debug function2, Sleep for 10sec.
		emit_dd(0x28ec8348);
		emit_db(0x48);		//mov rsi, MessageBoxW
		emit_db(0xBE);
		emit_ddp((DWORD_PTR)Sleep);
		emit_db(0x48); emit_db(0xc7); emit_db(0xc1); emit_db(0x10); emit_db(0x27); emit_db(0x00); emit_db(0x00);
		emit_db(0xFF);
		emit_db(0xD6);
		emit_dd(0x28c48348);
*/

//shellcode to find imagebase of kernel32.dll (under x64)
//rax will store the imagebase of kernel32.dll
/*
| 65 48 8B  | mov rax,qword ptr gs:[60]                                                  |
| 48 8B 40  | mov rax,qword ptr ds:[rax+18]                                              |
| 48 8B 40  | mov rax,qword ptr ds:[rax+30]                                              |
| 48 8B 00  | mov rax,qword ptr ds:[rax]                                                 |
| 48 8B 00  | mov rax,qword ptr ds:[rax]                                                 |
| 48 8B 40  | mov rax,qword ptr ds:[rax+10]                                              |
*/
		emit_db(0x65);
		emit_db(0x48);
		emit_db(0x8B);
		emit_db(0x04);
		emit_db(0x25);
		emit_db(0x60);
		emit_db(0x00);
		emit_db(0x00);
		emit_db(0x00);
		emit_db(0x48);
		emit_db(0x8B);
		emit_db(0x40);
		emit_db(0x18);
		emit_db(0x48);
		emit_db(0x8B);
		emit_db(0x40);
		emit_db(0x30);
		emit_db(0x48);
		emit_db(0x8B);
		emit_db(0x00);
		emit_db(0x48);
		emit_db(0x8B);
		emit_db(0x00);
		emit_db(0x48);
		emit_db(0x8B);
		emit_db(0x40);
		emit_db(0x10);
// === end of shellcode ===

		emit_dd(0x28ec8348);	//sub rsp,28h
		emit_db(0x48);		//mov rcx, dllpath
		emit_db(0xB9);
		emit_ddp((DWORD_PTR)remoteaddr + offsetof(opcode_data, dllpath));
		
		emit_db(0x48);	// mov rdx, rax
		emit_db(0x89);
		emit_db(0xC2);

		emit_db(0x48);	// add rax, offset of LoadLibrary IAT 
		emit_db(0x05);
		emit_dd(pfn);

/*  __asm:
		mov eax,dword ptr ds:[rax]
		add rdx,rax
		call rdx
*/
		emit_db(0x8B);
		emit_db(0x00);
		emit_db(0x48);
		emit_db(0x01);
		emit_db(0xC2);
		emit_db(0xFF);
		emit_db(0xD2);

		emit_dd(0x28c48348);	//add rsp,28h
		emit_db(0x5B);	
		emit_db(0x5A);	
		emit_db(0x59);	
		emit_db(0x58);		//popad		
		
		emit_db(0x48);		//mov rdi, orgRip
		emit_db(0xBE);
		emit_ddp(orgEIP);
		emit_db(0xFF);		//jmp rdi
		emit_db(0xE6);

		// gdi++.dllのパス
		int nSize = GetModuleFileNameW(g_inst, dllpath, MAX_PATH);
		if (nSize) {
			ChangeFileName(dllpath, nSize, L"MacType64.dll");
		}
		return !!nSize;
	}

};
#include <poppack.h>

// ｰｲﾈｫｵﾄﾈ｡ｵﾃﾕ賁ｵﾏｵﾍｳﾐﾅﾏ｢
VOID SafeGetNativeSystemInfo(__out LPSYSTEM_INFO lpSystemInfo)
{
	if (NULL == lpSystemInfo)    return;
	typedef VOID(WINAPI *LPFN_GetNativeSystemInfo)(LPSYSTEM_INFO lpSystemInfo);
	LPFN_GetNativeSystemInfo fnGetNativeSystemInfo = (LPFN_GetNativeSystemInfo)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetNativeSystemInfo");;
	if (NULL != fnGetNativeSystemInfo)
	{
		fnGetNativeSystemInfo(lpSystemInfo);
	}
	else
	{
		GetSystemInfo(lpSystemInfo);
	}
}

// ｻ�ﾈ｡ｲﾙﾗ�ﾏｵﾍｳﾎｻﾊ�
int GetSystemBits()
{
	SYSTEM_INFO si;
	SafeGetNativeSystemInfo(&si);
	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
		si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
	{
		return 64;
	}
	return 32;
}

static bool bIsOS64 = GetSystemBits() == 64;	// check if running in a x64 system.

#ifdef _M_IX86
// 止めているプロセスにLoadLibraryするコードを注入
BOOL WINAPI GdippInjectDLL(const PROCESS_INFORMATION* ppi)
{
	BOOL bIsX64Proc = false;
	if (bIsOS64 && IsWow64Process(ppi->hProcess, &bIsX64Proc) && !bIsX64Proc)
	{
		//x86 process launches a x64 process
		_CONTEXT64 ctx = { 0 };
		ctx.ContextFlags = CONTEXT_CONTROL;
		if (!GetThreadContext64(ppi->hThread, &ctx))
			return false;
		static bool bTryLoadDll64 = false;
		static DWORD dwLoaderOffset = 0;
		if (!bTryLoadDll64) {
			bTryLoadDll64 = true;
			GetEnvironmentVariable(L"MACTYPE_X64ADDR", NULL, 0);
			if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
				DWORD64 hNtdll = 0;
				hNtdll = GetModuleHandle64(L"ntdll.dll");
				if (hNtdll) {
					DWORD64 pfnLdrAddr = GetProcAddress64(hNtdll, "LdrLoadDll");
					if (pfnLdrAddr) {
						dwLoaderOffset = (DWORD)(pfnLdrAddr - hNtdll);
					}
				}
			}
		}

		opcode_data local;
		DWORD64 remote = VirtualAllocEx64(ppi->hProcess, NULL, sizeof(opcode_data), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (!remote)
			return false;
		bool basmIniter = dwLoaderOffset ? local.init64From32(remote, ctx.Rip, dwLoaderOffset) : local.init64From32(remote, ctx.Rip);
		if (!basmIniter	|| !WriteProcessMemory64(ppi->hProcess, remote, &local, sizeof(opcode_data), NULL)) {
			VirtualFreeEx64(ppi->hProcess, remote, 0, MEM_RELEASE);
			return false;
		}

		//FlushInstructionCache64(ppi->hProcess, remote, sizeof(opcode_data));
		//FARPROC a=(FARPROC)remote;
		//a();
		ctx.Rip = (DWORD64)remote;
		return !!SetThreadContext64(ppi->hThread, &ctx);
	}
	else {
		CONTEXT ctx = { 0 };
		ctx.ContextFlags = CONTEXT_CONTROL;
		if (!GetThreadContext(ppi->hThread, &ctx))
			return false;

		opcode_data local;
		opcode_data* remote = (opcode_data*)VirtualAllocEx(ppi->hProcess, NULL, sizeof(opcode_data), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (!remote)
			return false;

		if (!local.init32((LPDWORD)remote, ctx.Eip)
			|| !WriteProcessMemory(ppi->hProcess, remote, &local, sizeof(opcode_data), NULL)) {
			VirtualFreeEx(ppi->hProcess, remote, 0, MEM_RELEASE);
			return false;
		}

		FlushInstructionCache(ppi->hProcess, remote, sizeof(opcode_data));
		ctx.Eip = (DWORD)remote;
		return !!SetThreadContext(ppi->hThread, &ctx);
	}
}
#else
BOOL WINAPI GdippInjectDLL(const PROCESS_INFORMATION* ppi)
{
	BOOL bWow64 = false;
	IsWow64Process(ppi->hProcess, &bWow64);
	if (bWow64)
	{
		WOW64_CONTEXT ctx = { 0 };
		ctx.ContextFlags = CONTEXT_CONTROL;
		//CREATE_SUSPENDEDなので基本的に成功するはず
		if(!Wow64GetThreadContext(ppi->hThread, &ctx))
			return false;

		opcode_data local;
		LPVOID remote = VirtualAllocEx(ppi->hProcess, NULL, sizeof(opcode_data), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if(!remote)
			return false;

		if(!local.initWow64((LPDWORD)remote, ctx.Eip)
			|| !WriteProcessMemory(ppi->hProcess, remote, &local, sizeof(opcode_data), NULL)) {
				VirtualFreeEx(ppi->hProcess, remote, 0, MEM_RELEASE);
				return false;
		}

		FlushInstructionCache(ppi->hProcess, remote, sizeof(opcode_data));
		//FARPROC a=(FARPROC)remote;
		//a();
		ctx.Eip = (DWORD)remote;
		return !!Wow64SetThreadContext(ppi->hThread, &ctx);
	}
	else
	{
		CONTEXT ctx = { 0 };
		ctx.ContextFlags = CONTEXT_CONTROL;
		//CREATE_SUSPENDEDなので基本的に成功するはず
		if(!GetThreadContext(ppi->hThread, &ctx))
			return false;

		opcode_data local;
		LPVOID remote = VirtualAllocEx(ppi->hProcess, NULL, sizeof(opcode_data), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if(!remote)
			return false;

		if(!local.init((DWORD_PTR*)remote, ctx.Rip)
			|| !WriteProcessMemory(ppi->hProcess, remote, &local, sizeof(opcode_data), NULL)) {
				VirtualFreeEx(ppi->hProcess, remote, 0, MEM_RELEASE);
				return false;
		}

		FlushInstructionCache(ppi->hProcess, remote, sizeof(opcode_data));
		//FARPROC a=(FARPROC)remote;
		//a();
		ctx.Rip = (DWORD_PTR)remote;
		return !!SetThreadContext(ppi->hThread, &ctx);
	}
}

#endif

template <typename _TCHAR>
int strlendb(const _TCHAR* psz)
{
	const _TCHAR* p = psz;
	while (*p) {
		for (; *p; p++);
		p++;
	}
	return p - psz + 1;
}

template <typename _TCHAR>
_TCHAR* strdupdb(const _TCHAR* psz, int pad)
{
	int len = strlendb(psz);
	_TCHAR* p = (_TCHAR*)calloc(sizeof(_TCHAR), len + pad);
	if(p) {
		memcpy(p, psz, sizeof(_TCHAR) * len);
	}
	return p;
}



bool MultiSzToArray(LPWSTR p, vector<LPWSTR>& arr)
{
	for (; *p; ) {
		LPWSTR cp = _wcsdup(p);
		arr.push_back(cp);
		for (; *p; p++);
		p++;
	}
	return true;
}

LPWSTR ArrayToMultiSz(vector<LPWSTR>& arr)
{
	size_t cch = 1;
	for (int i=0; i<arr.size(); i++) {
		cch += wcslen(arr[i]) + 1;
	}

	LPWSTR pmsz = (LPWSTR)calloc(sizeof(WCHAR), cch);
	if (!pmsz)
		return NULL;

	LPWSTR p = pmsz;
	for (int i=0; i<arr.size(); i++) {
		StringCchCopyExW(p, cch, arr[i], &p, &cch, STRSAFE_NO_TRUNCATION);
		p++;
	}
	*p = 0;
	return pmsz;
}

bool AddPathEnv(vector<LPWSTR>& arr, LPWSTR dir, int dirlen)
{
	for (int i=0; i<arr.size(); i++) {
		LPWSTR env = arr[i];
		if (_wcsnicmp(env, L"PATH=", 5)) {
			continue;
		}

		LPWSTR p = env + 5;
		LPWSTR pp = p;
		for (; ;) {
			for (; *p && *p != L';'; p++);
			int len = p - pp;
			if (len == dirlen && !_wcsnicmp(pp, dir, dirlen)) {
				return false;
			}
			if (!*p)
				break;
			pp = p + 1;
			p++;
		}

		size_t cch = wcslen(env) + MAX_PATH + 4;
		env = (LPWSTR)realloc(env, sizeof(WCHAR) * cch);
		if(env) {
			StringCchCatW(env, cch, L";");
			StringCchCatW(env, cch, dir);
			arr[i] = env;
			return true;
		}
		return false;
	}

	size_t cch = dirlen + sizeof("PATH=") + 1;
	LPWSTR p = (LPWSTR)calloc(sizeof(WCHAR), cch);
	if(p) {
		StringCchCopyW(p, cch, L"PATH=");
		StringCchCatW(p, cch, dir);
		arr.push_back(p);
		return true;
		/*if (arr.Add(p)) {
			return true;
		}*/
		free(p);
	}
	return false;
}

bool AddX64Env(vector<LPWSTR>& arr)
{
	FARPROC k32 = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
	WCHAR szAddr[20] = { 0 };
	_ui64tow((DWORD64)k32, szAddr, 10);
	//wsprintf(szAddr, L"%Ld", (DWORD_PTR)k32);
	size_t cch = wcslen(szAddr) + sizeof("MACTYPE_X64ADDR=") + 1;
	LPWSTR p = (LPWSTR)calloc(sizeof(WCHAR), cch);
	if (p) {
		StringCchCopyW(p, cch, L"MACTYPE_X64ADDR=");
		StringCchCatW(p, cch, szAddr);
		arr.push_back(p);
		return true;
		/*if (arr.Add(p)) {
		return true;
		}*/
		free(p);
	}
	return false;
}

LPWSTR WINAPI GdippEnvironment(DWORD& dwCreationFlags, LPVOID lpEnvironment)
{
#ifndef _WIN64
	return NULL;
#endif
	GetEnvironmentVariableW(L"MACTYPE_X64ADDR", NULL, 0);
	if (GetLastError() != ERROR_ENVVAR_NOT_FOUND)
		return NULL;

	TCHAR dir[MAX_PATH];
	int dirlen = GetModuleFileName(g_inst, dir, MAX_PATH);
	LPTSTR lpfilename=dir+dirlen;
	while (lpfilename>dir && *lpfilename!=TEXT('\\') && *lpfilename!=TEXT('/')) --lpfilename;
	*lpfilename = 0;
	dirlen = wcslen(dir);

	LPWSTR pEnvW = NULL;
	if (lpEnvironment) {
		if (dwCreationFlags & CREATE_UNICODE_ENVIRONMENT) {
			pEnvW = strdupdb((LPCWSTR)lpEnvironment, MAX_PATH + 1);
		} else {
			int alen = strlendb((LPCSTR)lpEnvironment);
			int wlen = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)lpEnvironment, alen, NULL, 0) + 1;
			pEnvW = (LPWSTR)calloc(sizeof(WCHAR), wlen + MAX_PATH + 1);
			if (pEnvW) {
				MultiByteToWideChar(CP_ACP, 0, (LPCSTR)lpEnvironment, alen, pEnvW, wlen);
			}
		}
	} else {
		LPWSTR block = (LPWSTR)GetEnvironmentStringsW();
		if (block) {
			pEnvW = strdupdb(block, MAX_PATH + 1);
			FreeEnvironmentStrings(block);
		}
	}

	if (!pEnvW) {
		return NULL;
	}

	vector<LPWSTR> envs;
	bool ret = MultiSzToArray(pEnvW, envs);
	free(pEnvW);
	pEnvW = NULL;
	
	/*if (ret) {
		ret = AddPathEnv(envs, dir, dirlen);
	}*/
#ifdef _WIN64
	{
		GetEnvironmentVariableW(L"MACTYPE_X64ADDR", NULL, 0);
		if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
			ret = AddX64Env(envs);
		}
	}
#endif
	if (ret) {
		pEnvW = ArrayToMultiSz(envs);
	}

	for (int i=0; i<envs.size(); free(envs[i++]));

	if (!pEnvW) {
		return NULL;
	}

/*
#ifdef _DEBUG
	{
		LPWSTR tmp = strdupdb(pEnvW, 0);
		LPWSTR tmpe = tmp + strlendb(tmp);
		PathRemoveFileSpec(dir);
		for (LPWSTR z=tmp; z<tmpe; z++)if(!*z)*z=L'\n';
		StringCchCatW(dir,MAX_PATH,L"\\");
		StringCchCatW(dir,MAX_PATH,L"gdienv.txt");
		HANDLE hf = CreateFileW(dir,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
		if(hf) {
			DWORD cb;
			WORD w = 0xfeff;
			WriteFile(hf,&w, sizeof(WORD), &cb, 0);
			WriteFile(hf,tmp, sizeof(WCHAR) * (tmpe - tmp), &cb, 0);
			SetEndOfFile(hf);
			CloseHandle(hf);
			free(tmp);
		}
	}
#endif*/

	dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
	return pEnvW;
}
