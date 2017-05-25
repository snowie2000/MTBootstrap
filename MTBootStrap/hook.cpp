#include "hook.h"

#ifdef _M_IX86
#ifdef MHOOK
#pragma comment (lib, "mhook.lib")
#else
#pragma comment (lib, "easyhk32.lib")
#endif
#else
#ifdef MHOOK
#pragma comment (lib, "mhook64.lib")
#else
#pragma comment (lib, "easyhk64.lib")
#endif
#endif

#ifdef MHOOK

#define HOOK_MANUALLY(rettype, name, argtype) \
	rettype (WINAPI * ORIG_##name) argtype;
#include "hooklist.h"	//declare variables
#undef  HOOK_MANUALLY

#define FORCE(expr) {if(!(expr)) goto ERROR_ABORT;}
#define HOOK_MANUALLY(rettype, name, argtype) \
	LONG hook_demand_##name(bool bForce = false){ \
	if (&ORIG_##name) {	\
	FORCE(Mhook_SetHook((PVOID*)&ORIG_##name, IMPL_##name)); \
	return NOERROR; } \
	ERROR_ABORT: \
	return 1; \
}

#include "hooklist.h"	//init hook functions

#undef HOOK_MANUALLY

#define HOOK_MANUALLY(rettype, name, argtype) \
	Mhook_Unhook((PVOID*)&ORIG_##name);
#pragma optimize("s", on)
LONG hook_term()
{
#include "hooklist.h"
	return 0;
}
#pragma optimize("", on)
#undef HOOK_MANUALLY


#else

#define HOOK_MANUALLY(rettype, name, argtype) \
	HOOK_TRACE_INFO HOOK_##name = {0};	//init hook struct
#include "hooklist.h"
#undef  HOOK_MANUALLY

#define HOOK_MANUALLY(rettype, name, argtype) \
	rettype (WINAPI * ORIG_##name) argtype;
#include "hooklist.h"	//declare variables
#undef  HOOK_MANUALLY

#define FORCE(expr) {if(!SUCCEEDED(NtStatus = (expr))) goto ERROR_ABORT;}
#define HOOK_MANUALLY(rettype, name, argtype) \
	LONG hook_demand_##name(bool bForce = false){ \
	NTSTATUS NtStatus; \
	ULONG ACLEntries[1] = { 0 }; \
	if (bForce) {  \
		memset((void*)&HOOK_##name, 0, sizeof(HOOK_TRACE_INFO));  \
			}  \
	if (&ORIG_##name) {	\
		__try{ \
		FORCE(LhInstallHook((PVOID&)ORIG_##name, IMPL_##name, (PVOID)0, &HOOK_##name)); \
		*(void**)&ORIG_##name =  (void*)HOOK_##name.Link->OldProc; \
		FORCE(LhSetExclusiveACL(ACLEntries, 0, &HOOK_##name)); \
						} \
		__except(EXCEPTION_EXECUTE_HANDLER) { \
			return 1; \
				} \
	} \
	return NOERROR; \
	ERROR_ABORT: \
	return 1; \
				}

	#include "hooklist.h"	//init hook functions

#undef HOOK_MANUALLY

#define HOOK_MANUALLY(rettype, name, argtype) \
	ORIG_##name = name;
#pragma optimize("s", on)
LONG hook_term()
{
	#include "hooklist.h"
	LhUninstallAllHooks();
	return LhWaitForPendingRemovals();
}
#pragma optimize("", on)
#undef HOOK_MANUALLY

#endif