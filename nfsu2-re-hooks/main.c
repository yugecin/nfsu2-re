/* vim: set filetype=c ts=8 noexpandtab: */

#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <windows.h>

#define STATIC_ASSERT(E) typedef char __static_assert_[(E)?1:-1]
#define EXPECT_SIZE(S,SIZE) STATIC_ASSERT(sizeof(S)==(SIZE))
#define MEMBER_OFFSET(S,M) (int)&((S*)NULL)->M
#define ASSERT_OFFSET(STRUCT,MEMBER,OFFSET) STATIC_ASSERT(MEMBER_OFFSET(STRUCT,MEMBER)==OFFSET)

static int base;
static FILE *logfile;
static char buf[4096], buf2[4096];

#define LOG_TO_LOGFILE
//#define LOG_TO_DEBUGSTRING

static char newline = 10;

static
void log(char *msg, int len)
{
#ifdef LOG_TO_LOGFILE
	fwrite(msg, len, 1, logfile);
	fwrite(&newline, 1, 1, logfile);
	fflush(logfile);
#endif
#ifdef LOG_TO_DEBUGSTRING
	OutputDebugString(msg);
#endif
}

#pragma pack(push,1)

struct DialogInfo {
	char text[768];
	int unused300;
	int unused304;
	int unused308;
	int unused30C;
	int unused310;
	int unused314;
	int unused318;
	int unused31C;
	int something320; /*read in 5580EB*/
	char *pTypeName;
	int unused_328;
	char unused_32C;
	char isHelpDialog;
	char unused_32E;
	char something32F; /*set to 0 in 55806D*/
	int something330; /*set to 0 in 558074*/
	int unused334;
};
EXPECT_SIZE(struct DialogInfo, 0x338);
ASSERT_OFFSET(struct DialogInfo, pTypeName, 0x324);
ASSERT_OFFSET(struct DialogInfo, isHelpDialog, 0x32D);

struct CarModelInfo {
	char brand[0x20];
	char brand2[0x20];
	char geometry_bin[0x20];
	char geometry_lzc[0x20];
	char unk80[0x20];
	char unkA0[0x20];
	char manufacturer[0x40];
	char pad[0x790];
};
EXPECT_SIZE(struct CarModelInfo, 0x890);
ASSERT_OFFSET(struct CarModelInfo, manufacturer, 0xC0);

typedef int (fnginithandler)(struct FNGShowData *msg);

struct FNGData {
	char *name;
	fnginithandler *initializeHandler;
	int field_8;
	int field_C;
	int field_10;
	int field_14;
	int field_18;
};
EXPECT_SIZE(struct FNGData, 0x1C);

struct FNGShowData {
	char *fngname;
	int arg1;
	int fngdata_field8;
	int fngdata_fieldC;
	int arg2;
};
EXPECT_SIZE(struct FNGShowData, 0x14);

struct FNGInfo {
	int field_0;
	int field_4;
	int field_8;
	int field_C;
	int field_10;
	int field_14;
	int field_18;
	int controlMask;
};

struct MouseData {
	void /*DInputDevice8*/ *dinputdevice;
	int cursorX; // on 640x480 canvas
	int cursorY; // on 640x480 canvas
	int previousCursorX; // on 640x480 canvas
	int previousCursorY; // on 640x480 canvas
	int deltaCursorX; // on 640x480 canvas
	int deltaCursorY; // on 640x480 canvas
	int mousestate_lZ; // scrollwheel data
	char areMouseButtonsSwapped; // result of GetSystemMetrics(SM_SWAPBUTTON);
	char button0State; // left mouse button
	char button0JustPressed;
	char button0JustReleased;
	char button1State; // right mouse button
	char button1JustPressed;
	char button1JustReleased;
	char button2State; // middle mouse button
	char button2JustPressed;
	char button2JustReleased;
};

struct MouseData *_mouseData = (struct MouseData*) 0x8763D0;
struct CarModelInfo **_car_model_infos = (void*) 0x8A1CCC;
struct FNGData *fngdata = (struct FNGData*) 0x7F7DC8;
int *_game_region = (int*) 0x864F24;
int *_current_loaded_language = (int*) 0x7F70D0;
HWND *hwnd = (HWND*) 0x870990;

#pragma pack(pop,1)

/***********************************************************************************************
UTILS
*/

static
void mkjmp(int at, void *to)
{
	/*at -= 0x400000; at += base;*/
	*((unsigned char*) at) = 0xE9;
	at++;
	*((int*) at) = (int) to - (at + 4);
}

static
int isprocstaticmem(int loc)
{
	return base < loc && loc < base + 0x4BB000;
}

static
__declspec(naked) int nfsu2_stricmp(char *a, char *b)
{
	_asm {
		mov eax, 0x43DCC0
		jmp eax
	}
}

static
__declspec(naked) char *nfsu2_GetLanguageString(int hash)
{
	_asm {
		mov eax, 0x4FFA80
		jmp eax
	}
}

static
int cshash(char *input)
{
	unsigned int result;
	char c;
	int i, j;

	if (input == NULL) {
		return -1;
	}
	result = -1;
	i = strlen(input);
	j = 0;
	while (j < i) {
		result *= 33;
		result += input[j++];
	}
	return result;
}

static
int cihash(char *input)
{
	unsigned int result;
	char c;
	int i, j;

	if (input == NULL) {
		return -1;
	}
	result = -1;
	i = strlen(input);
	j = 0;
	while (j < i) {
		result *= 33;
		c = input[j++];
		if (c < 'a' || 'z' < c) {
			result += c;
		} else {
			result += c - 0x20;
		}
	}
	return result;
}

static
void hash_func_log(char *arg, int result, int address)
{
	if (arg != NULL) {
		log(buf, sprintf(buf, "hash\t%s\t%p\t%X", arg, result, address));
	}
}

static
void logvalue(char *lineprefix, char *varname, int value)
{
	float floatval;
	int derefval;

	floatval = *((float*) &value);
	log(buf, sprintf(buf, "%s   value '%s'", lineprefix, varname));
	log(buf, sprintf(buf, "%s     char %d 0x%X short 0x%d %X int %d 0x%X float %.1f",
		lineprefix, value & 0xFF, value & 0xFF, value & 0xFFFF, value & 0xFFFF,
		value, value, floatval));
	if (!IsBadReadPtr((void*) value, 4)) {
		log(buf, sprintf(buf, "%s      is valid pointer to:", lineprefix));
		derefval = *((int*) value);
		floatval = *((float*) &derefval);
		log(buf, sprintf(buf, "%s        char %d 0x%X short %d 0x%X int %d 0x%X float %.1f",
			lineprefix, value & 0xFF, value & 0xFF, value & 0xFFFF, value & 0xFFFF,
			value, value, floatval));
	}
	if (!IsBadStringPtrA((char*) value, sizeof(buf) - 10)) {
		log(buf, sprintf(buf, "%s      strptr: '%s'", lineprefix, value));
	}
}

/***********************************************************************************************
Short reimplemented functions that are not replaced
*/

/*570A40*/
static
int UseCarUKNames()
{
	if (*_game_region == 1) {
		return 1;
	}
	if (*_game_region <= 4) {
		return 0;
	}
	if (*_game_region <= 9) {
		return 1;
	}
	return 0;
}

/**********************/

static
void stub()
{
}
#define INIT_FUNC stub

/*note: hash hooks can get called A LOT so this may slow down the game*/

#define WIDESCREEN_MOD /*define when external widescreen mod is active*/

#include "runwindowed.c" // must be on top for NFSU2_RUN_WINDOWED define, needed for mouse stuff
#include "faux-enable-console.c"
//#include "hook-43DB50-hash-cs.c"
//#include "hook-440B96-CreatePool.c"
//#include "hook-505450-hash-ci.c"
//#include "hook-50B9C0-hash-ci.c"
//#include "hook-55DC20-SendSomethingToFNG.c"
//#include "hook-57CAC0-SomethingWithABinFile.c"
#include "hook-fileapi.c"
//#include "hook-realcore-filesystem.c"
//#include "replace-50B790-ShowFNG.c"
//#include "replace-50D510-DebugPrint.c"
//#include "replace-511E60-GetLogoForCarModel.c"
//#include "replace-526C40-GetFNGForDialog.c"
#include "replace-5BF750-UpdateCursorPosition.c"
#include "replace-5BF860-UpdateMouseState.c"
#include "replace-5BF940-HaveCursorActivity.c"
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