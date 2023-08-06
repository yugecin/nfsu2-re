#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <windows.h>
#define log mathlog
#include <math.h>
#undef log
#include <d3d9.h>
#pragma pack(push,1)

#define STATIC_ASSERT(E) typedef char __static_assert_[(E)?1:-1]
#define EXPECT_SIZE(S,SIZE) STATIC_ASSERT(sizeof(S)==(SIZE))
#define MEMBER_OFFSET(S,M) (int)&((S*)NULL)->M
#define ASSERT_OFFSET(STRUCT,MEMBER,OFFSET) STATIC_ASSERT(MEMBER_OFFSET(STRUCT,MEMBER)==OFFSET)

static int base;

//#define LOG_TO_LOGFILE
//#define LOG_TO_DEBUGSTRING
#define LOG_TO_PRINTF

#include "a_log.c"
#include "a_structs.c"
#include "a_fields.c"
#include "a_funcs.c"
#include "a_utils.c"

static void stub() {}
#define INIT_FUNC stub
#define DEBUG_WMCHAR_FUNC stub
#define DEBUG_DOUI_FUNC stub
#define PRESENT_HOOK_FUNC stub

/*will only work when runwindowed.c is included*/
/*remember to enable/disable the widescreenfix asi script*/
//#define WINDOWED_RESOLUTION_X 960 /*4:3*/
#define WINDOWED_RESOLUTION_X 1280 /*16:9*/
#define WINDOWED_RESOLUTION_Y 720
//#define WINDOWED_RESOLUTION_X 640
//#define WINDOWED_RESOLUTION_Y 480

/*note: hash hooks can get called A LOT so this may slow down the game*/
#include "d3d9.c"
#include "runwindowed.c" // must be on top for NFSU2_RUN_WINDOWED define, needed for mouse stuff
#include "faux-enable-console.c"
//#include "hook-43DB50-hash-cs.c"
//#include "hook-440B40-AllocateAndInitPool.c"
//#include "hook-505450-hash-ci.c"
//#include "hook-50B9C0-hash-ci.c"
//#include "hook-511E60-GetLogoForCarModel.c"
//#include "hook-52CEF0-GetFNGInfoByName.c"
//#include "hook-535FE0-UILabel__setString.c"
//#include "hook-555D00-AddFNGToUIObject.c"
#include "hook-55B5C0-FNGObject__BaseNotify.c"
//#include "hook-55DC20-SendSomethingToFNG.c"
//#include "hook-57CAC0-SomethingWithABinFile.c"
#include "hook-74A6ED-recv.c"
//#include "hook-fileapi.c"
//#include "hook-realcore-filesystem.c"
//#include "replace-440BB0-Pool__Extend.c"
//#include "replace-440DF0-CreatePool.c"
//#include "replace-4EED10-CarSelectFNGObject__ChangeCategory.c"
//#include "replace-50B790-ShowFNG.c"
//#include "replace-50CD00-IsPointInRect.c"
//#include "replace-50CDB0-DidCursorPressInRect.c"
//#include "replace-50D510-DebugPrint.c"
//#include "replace-5149C0-GetSmsMessageSubject.c"
//#include "replace-497760-GetSmsMessageSubjectHeader.c" /*needs to be after GetSmsMessageSubject*/
//#include "replace-514B00-GetSmsMessageBody.c"
//#include "replace-514CB0-SmsMessageList__IsMessageAlreadyReceived.c" // dummy always ret 0 atm
//#include "replace-51D9F0-GetUIElementSize-Maybe-TODO.c"
//#include "replace-526C40-GetFNGForDialog.c"
//#include "replace-5BF750-UpdateCursorPosition.c"
//#include "replace-5BF860-UpdateMouseState.c"
//#include "replace-5BF940-HaveCursorActivity.c"
//#include "replace-5C8600-MouseData__ctor.c"
#include "speedyboot.c"
//#include "focusloss-nopause.c" // note that cause of this it will still accept keys while in bg
#include "replaceloadingscreen.c"
#include "skipinsertdisc2.c"
#include "ignorefocuslossintextboxandlist.c"
//#include "switch-joyids-in-career.c"

//#include "fun-car-customize-preset.c"
//#include "fun-car-customize-sponsor.c"
//#include "fun-destroy-garage-backdrop.c"

#include "dbgw_a_main.c"

//#include "generate-fngdata-list.c"
//#include "generate-joyname-list.c"

//#include "ol-replace-7583E0.c"
//#include "ol-replace-759B4A.c"

//#include "debug-custom-dump-markers.c"
//#include "debug-custom-dump-smsdata.c"
//#include "debug-custom-endoutrun.c"
//#include "debug-custom-find-nearest-marker.c"
//#include "debug-custom-markers-debug.c"
//#include "debug-custom-getkeyname.c"
//#include "debug-custom-iterate-carpresets.c"
//#include "debug-custom-iterate-races.c"
//#include "debug-custom-iterate-somethingui.c"
//#include "debug-custom-uielementvisitor.c"
//#include "debug-custom-send-instruction-sms.c"
//#include "debug-custom-printmailboxthings.c"
//#include "debug-custom-remove-all-received-engage-markers.c"
#include "debug-hook-wm_char.c" // needs to be after all "debug-custom-*" files
#include "debug-hook-doui.c" // needs to be after all "debug-custom-*" files
#include "debug-hook-beforepresent.c" // needs to be after all "debug-custom-*" files
#include "winconsole.c" // needs to be at the end (or anything else using printf might not show)

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