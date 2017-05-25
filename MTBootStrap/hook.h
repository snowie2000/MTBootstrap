#include <windows.h>
#ifdef MHOOK
#include "mhook.h"
#else
#include "easyhook.h"
#endif
#include "undocAPI.h"

#define HOOK_MANUALLY(rettype, name, argtype) \
	extern rettype (WINAPI * ORIG_##name) argtype; \
	extern rettype WINAPI IMPL_##name argtype; \
	extern LONG hook_demand_##name(bool bForce); \
	extern void unhook_##name();
#include "hooklist.h"
#undef HOOK_MANUALLY

LONG hook_term();

