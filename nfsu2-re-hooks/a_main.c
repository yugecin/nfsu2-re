#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <windows.h>
#pragma pack(push,1)

#define STATIC_ASSERT(E) typedef char __static_assert_[(E)?1:-1]
#define EXPECT_SIZE(S,SIZE) STATIC_ASSERT(sizeof(S)==(SIZE))
#define MEMBER_OFFSET(S,M) (int)&((S*)NULL)->M
#define ASSERT_OFFSET(STRUCT,MEMBER,OFFSET) STATIC_ASSERT(MEMBER_OFFSET(STRUCT,MEMBER)==OFFSET)

static int base;

#include "a_log.c"
#include "a_structs.c"
#include "a_fields.c"
#include "a_utils.c"
#include "a_funcs.c"

static void stub() {}
#define INIT_FUNC stub

/*note: hash hooks can get called A LOT so this may slow down the game*/
#define WIDESCREEN_MOD /*define when external widescreen mod is active*/

#include "runwindowed.c" // must be on top for NFSU2_RUN_WINDOWED define, needed for mouse stuff
#include "faux-enable-console.c"
//#include "hook-43DB50-hash-cs.c"
//#include "hook-440B96-CreatePool.c"
#include "replace-5149C0-GetSmsMessageSubject.c" /*required for GetSmsMessageSubjectHeader*/
#include "replace-497760-GetSmsMessageSubjectHeader.c"
//#include "hook-505450-hash-ci.c"
//#include "hook-50B9C0-hash-ci.c"
//#include "hook-511E60-GetLogoForCarModel.c"
//#include "hook-55DC20-SendSomethingToFNG.c"
//#include "hook-57CAC0-SomethingWithABinFile.c"
#include "hook-fileapi.c"
//#include "hook-realcore-filesystem.c"
//#include "replace-50B790-ShowFNG.c"
//#include "replace-50CD00-IsPointInRect.c"
//#include "replace-50CDB0-DidCursorPressInRect.c"
//#include "replace-50D510-DebugPrint.c"
//#include "replace-51D9F0-GetUIElementSize-Maybe-TODO.c"
#include "replace-526C40-GetFNGForDialog.c"
//#include "replace-5BF750-UpdateCursorPosition.c"
//#include "replace-5BF860-UpdateMouseState.c"
//#include "replace-5BF940-HaveCursorActivity.c"
//#include "replace-5C8600-MouseData__ctor.c"
#include "speedyboot.c"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason_for_call, LPVOID lpResrvd)
{
	DWORD oldvp;
	
	if (reason_for_call == DLL_PROCESS_ATTACH) {
		base = (int) GetModuleHandle(NULL);
		VirtualProtect((LPVOID) base, 280000, PAGE_EXECUTE_READWRITE, &oldvp);

		logfile = fopen("../SPEED2-LOG.TXT", "wb");
		log(buf, sprintf(buf, "\n\n\n\nATTACHED"));
		if (base != 0x400000) {
			log(buf, sprintf(buf, "base is not at 400000!"));
		}
		INIT_FUNC();
	} else if (reason_for_call == DLL_PROCESS_DETACH) {
		if (logfile) {
			fclose(logfile);
		}
	}

	return 1;
}