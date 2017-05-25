HOOK_MANUALLY(BOOL, CreateProcessInternalW, (\
	HANDLE hToken, \
	LPCTSTR lpApplicationName, \
	LPTSTR lpCommandLine, \
	LPSECURITY_ATTRIBUTES lpProcessAttributes, \
	LPSECURITY_ATTRIBUTES lpThreadAttributes, \
	BOOL bInheritHandles, \
	DWORD dwCreationFlags, \
	LPVOID lpEnvironment, \
	LPCTSTR lpCurrentDirectory, \
	LPSTARTUPINFO lpStartupInfo, \
	LPPROCESS_INFORMATION lpProcessInformation, \
	PHANDLE hNewToken \
	));

HOOK_MANUALLY(LONG, LdrLoadDll, (IN PWCHAR  PathToFile OPTIONAL,
	IN ULONG                Flags OPTIONAL,
	IN UNICODE_STRING2*      ModuleFileName,
	OUT HANDLE*             ModuleHandle));

HOOK_MANUALLY(BOOL, MyUpdateProcThreadAttribute, (_Inout_ LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
	_In_ DWORD dwFlags,
	_In_ DWORD_PTR Attribute,
	_In_reads_bytes_opt_(cbSize) PVOID lpValue,
	_In_ SIZE_T cbSize,
	_Out_writes_bytes_opt_(cbSize) PVOID lpPreviousValue,
	_In_opt_ PSIZE_T lpReturnSize
	));