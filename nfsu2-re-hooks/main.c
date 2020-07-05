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
static char buf[1024], buf2[1024];

/************
OPTS
*/

#define ENABLE_HASH_HOOK_505450
#define ENABLE_HASH_HOOK_43DB50
#define ENABLE_HASH_HOOK_50B9C0
#define HASH_HOOKS_TO_LOGFILE
/*note: hash hooks can get called A LOT so this may slow down the game*/
//#define HASH_HOOKS_TO_DEBUGSTRING

#define ENABLE_DEBUGSTRING_50D510
//#define DEBUGSTRING_TO_LOGFILE
//#define DEBUGSTRING_TO_DEBUGSTRING

#define REPLACE_PROC_DIALOG_GETFNG

//#define HOOK_CAR_MODEL_GET_LOGO

//#define REPLACE_SHOWFNG
//#define SHOWFNG_DEBUGPRINT

//************

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

struct CarModelInfo **_car_model_infos = (void*) 0x8A1CCC;
struct FNGData *fngdata = (struct FNGData*) 0x7F7DC8;
int *_game_region = (int*) 0x864F24;
int *_current_loaded_language = (int*) 0x7F70D0;

#pragma pack(pop,1)

/***********************************************************************************************
UTILS
*/

static
void mkjmp(int at, void *to)
{
	at += base;
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
void stub()
{
}
#define INIT_FUNC stub

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

/***********************************************************************************************
55DC20 MaybeSendMessageToFNG?
*/

static
int a(
      int a, int b, int c, int d,
      int e, int f, int g, int h,
      int ignore_ret_adds,
      char *fng, int message)
{
	sprintf(buf, "msg %s: %p", fng, message);
	OutputDebugString(buf);
	return 0;
}

static
__declspec(naked) void aa()
{
	_asm {
		pushad
		call a
		popad
		mov eax, [esp+4]
		push eax
		push 0x55DC25
		ret
	}
}

static
void aaa()
{
	mkjmp(0x15DC20, &aa);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC aaa
}

/***********************************************************************************************
50B790 ShowFNG
*/

static
int ShowFNG(int fnghash, int arg1, int arg2)
{
	struct FNGShowData data;
	struct FNGData *fng;
	int result;
	int i;

	fng = NULL;
	result = 0;
	for (i = 0; i < 180; i++) {
		if (cihash(fngdata[i].name) == fnghash) {
			fng = fngdata + i;
			if (fng->initializeHandler) {
				data.fngname = fngdata[i].name;
				data.arg1 = arg1;
				data.fngdata_field8 = fng->field_8;
				data.fngdata_fieldC = fng->field_C;
				data.arg2 = arg2;
				result = fng->initializeHandler(&data);
			}
			break;
		}
	}

#ifdef SHOWFNG_DEBUGPRINT
	sprintf(buf,
		"ShowFNG(%p (\"%s\"), %p, %p) = %p",
		fnghash,
		fng == NULL ? "" : fng->name,
		arg1,
		arg2,
		result);
	OutputDebugString(buf);
#endif
	return result;
}

static
void initReplaceShowFNG()
{
#ifdef REPLACE_SHOWFNG
	mkjmp(0x10B790, &ShowFNG);
#endif

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initReplaceShowFNG
}

/***********************************************************************************************
511E60 GetLogoForCarModel

why not
*/

static
void GetLogoForCarModel(int car_model_index)
{
	struct CarModelInfo *car_model_info;

	car_model_info = (*_car_model_infos) + car_model_index;
	sprintf(buf,
		"car idx %d at %p brand %s manufacturer %s",
		car_model_index,
		car_model_info,
		car_model_info->brand,
		car_model_info->manufacturer);
	OutputDebugString(buf);
}

static
__declspec(naked) int GetLogoForCarModelHook(int car_model_index, char get_manufacturer)
{
	_asm {
		push [esp+0x4]
		call GetLogoForCarModel
		add esp, 0x4
		mov edx, 0x8A1CCC
		mov edx, [edx]
		push 0x511E66
		ret
	}
}

static
void initReplaceGetLogoForCarModel()
{
#ifdef HOOK_CAR_MODEL_GET_LOGO
	mkjmp(0x111E60, &GetLogoForCarModelHook);
#endif

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initReplaceGetLogoForCarModel
}

/***********************************************************************************************
SPEEDYBOOT

speed
*/

static int zero = 0;
static
void speedyboot()
{
	*(int**) (0x7F65E8) = &zero; /*discerrorpc*/
	*(int**) (0x7F65EC) = &zero; /*??nothing*/
	//*(int**) (0x7F65F0) = &zero; /*mc_bootup*/
	*(int**) (0x7F65F4) = &zero; /*??nothing*/
	*(int**) (0x7F65F8) = &zero; /*??nothing*/
	*(int**) (0x7F65FC) = &zero; /*blankmovie*/
	*(int**) (0x7F6600) = &zero; /*ealogo*/
	*(int**) (0x7F6604) = &zero; /*??nothing*/
	*(int**) (0x7F6608) = &zero; /*thxmovie*/
	//*(int**) (0x7F660C) = &zero; /*psamovie*/
	*(int**) (0x7F6610) = &zero; /*introfmv*/
	*(int**) (0x7F6614) = &zero; /*splash*/
	//*(int**) (0x7F6618) = &zero; /*mc_background*/
	//*(int**) (0x7F661C) = &zero; /*ui_main*/

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC speedyboot
}

/***********************************************************************************************
526C40 GETFNGFORDIALOG

Decides the FNG to display for passed dialog.
*/

static
char* do526C40getFNGforDialog(struct DialogInfo *dialog)
{
	int num_lines;
	int i;
	int tmp;
	float text_width;

	if (dialog->pTypeName && dialog->pTypeName[0]) {
		if (strcmp(dialog->pTypeName, "animating") || strcmp(dialog->pTypeName, "3button")) {
			return dialog->pTypeName;
		}
	}

	/*526C80*/
	if (dialog->isHelpDialog) {
		if (dialog->text[0]) {
			num_lines = 1;
			i = 0;
nextchar:		{
				switch (dialog->text[i]) {
				case '\n':
				case '^': num_lines++;
				default:
					i++;
					goto nextchar;
				case 0: break;
				}
			}
			if (num_lines > 4) {
				return "HelpDialog_MED.fng";
			}
		}
		return "HelpDialog_SMALL.fng";
	}

	/*526CBF*/
	tmp = ((int(__cdecl *)(char*))0x43DB50)("CONDUITMDITC_TT21I"); /*SomeHashCS43DB50*/
	((void(__cdecl*)(int))0x51BD70)(tmp);
	_asm { mov eax, [eax+0x24] }
	_asm { mov text_width, eax }

	text_width *= strlen(dialog->text);

	if (strcmp(dialog->pTypeName, "3button") == 0) {
		return "GenericDialog_ThreeButton.fng";
	}

	/*526D07*/
	if (text_width < 2561.0f) {
		if (strcmp(dialog->pTypeName, "animating")) {
			return "GenericDialog_SMALL.fng";
		} else {
			return "GenericDialog_Animate_SMALL.fng";
		}
	}

	if (text_width < 5122.0f) {
		if (strcmp(dialog->pTypeName, "animating")) {
			return "GenericDialog_MED.fng";
		} else {
			return "GenericDialog_Animate_MED.fng";
		}
	}

	if (strcmp(dialog->pTypeName, "animating")) {
		return "GenericDialog_LARGE.fng";
	} else {
		return "GenericDialog_Animate_LARGE.fng";
	}
}

static
void initHook526C40getFNGforDialog()
{
#ifdef REPLACE_PROC_DIALOG_GETFNG
	mkjmp(0x126C40, &do526C40getFNGforDialog);
#endif

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHook526C40getFNGforDialog
}

/***********************************************************************************************
50D512 DEBUG PRINT

Some proc that seems to get debug string passed, sometimes with placeholders. The proc is now
a nullsub (xor eax, eax; retn), so sometimes non-strings get passed too.
*/

static
void do50D510Print(int *esp)
{
	int from;
	char *format;
	int b, c, d, e, f, g, h, i;
	int len;

	from = *esp;
	format = (char*) *(esp+1);
	b = *(esp+2);
	c = *(esp+3);
	d = *(esp+4);
	e = *(esp+5);
	f = *(esp+6);
	g = *(esp+7);
	h = *(esp+8);
	i = *(esp+9);
	if (isprocstaticmem((int) format)) {
		sprintf(buf2, format, b, c, d, e, f, g, h, i);
		len = strlen(buf2);
		if (len && buf2[len - 1] == '\n') {
			buf2[len - 1] = 0;
		}
#ifdef DEBUGSTRING_TO_LOGFILE
		fwrite(buf, sprintf(buf, "debugstr\t50D510\t%p\t%s\n", from, buf2), 1, logfile);
#endif
#ifdef DEBUGSTRING_TO_DEBUGSTRING
		sprintf(buf, "debugstr\t50D510\t%p\t%s\n", from, buf2);
		OutputDebugString(buf);
#endif
	}
}

static
__declspec(naked) void hook50D510Print()
{
	_asm {
		mov eax, esp
		pushad
		push eax
		call do50D510Print
		add esp, 0x4
		popad
		xor eax, eax
		ret
	}
}

static
void initHook50D510Print()
{
#ifdef ENABLE_DEBUGSTRING_50D510
	mkjmp(0x10D510, &hook50D510Print);
#endif

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHook50D510Print
}

/***********************************************************************************************
RE-ENABLE CONSOLE

This doesn't do anything except for saving the string you're typing into the console_real_text
variable. Caret position can be changed with home, end, and arrow keys. Return and tab inserts
linefeeds. Escape inserts a zero byte at caret pos.
*/

char **console_text_string;
int *console_text_string_length;
int *console_text_string_caret_position;
int *console_enabled;
int *console_text_string_max_length;

char console_real_text[100];

static
void initConsolePOC()
{
	console_text_string = (char**) 0x8709B0;
	console_text_string_length = (int*) 0x8709B4;
	console_text_string_caret_position = (int*) 0x8709b8;
	console_enabled = (int*) 0x8709BC;
	console_text_string_max_length = (int*) 0x8709C0;

	*console_text_string = console_real_text;
	*console_text_string_max_length = sizeof(console_real_text);
	*console_enabled = 1;

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initConsolePOC
} 

/***********************************************************************************************
SOME HASH FUNCTIONS
*/

static
void SomeHashCS43DB50Print(char *arg, int *result)
{
	if (arg == NULL) {
		//OutputDebugString("43DB50: hashing nullptr\n");
	} else {
#ifdef HASH_HOOKS_TO_LOGFILE
		fwrite(buf, sprintf(buf, "hash\t%s\t%p\t43DB50\n", arg, *result), 1, logfile);
#endif
#ifdef HASH_HOOKS_TO_DEBUGSTRING
		sprintf(buf, "hash\t%s\t%p\t43DB50\n", arg, *result);
		OutputDebugString(buf);
#endif

		if (strcmp(arg, "FIRETRUCK") == 0) {
			//*result = 0x83FBE66E;
		}
		if (strcmp(arg, "TAXI") == 0) {
			//*result = 0x001D1775

			// use ambulance for taxi
			//*result = 0x43A98EA7;
		}
		if (strcmp(arg, "240SX") == 0) {
			//*result = 0x0150FB80;
			
			// use firetruck for 240sx
			//*result = 0x83FBE66E;

			// use taxi for 240sx
			//*result = 0x001D1775;

			// use mustang for 240sx
			//*result = 0x35165819;
		}
	}
}

static
__declspec(naked) void SomeHashCS43DB50HookPost()
{
	_asm {
		mov edx, [esp+0x4]
		//; result on the stack (so the prnt func can modify it)
		push eax
		pushad
		lea ecx, [esp+0x4*8]
		//; ptr to result
		push ecx
		//; the original arg
		push edx
		call SomeHashCS43DB50Print
		add esp, 0x8
		popad
		//; get result back (in case we modified it)
		pop eax
		ret
	}
}

static
__declspec(naked) void SomeHashCS43DB50HookPre()
{
	_asm {
		//; the overwritten instruction for our jump
		mov edx, [esp+0x4]
		mov cl, [edx]
		//; not pushing the arg because it's only used in the lines above
		//; ret to SomeHashHookPost
		push SomeHashCS43DB50HookPost
		//; call the actual function (using ret...)
		push 0x43DB56
		ret
	}
}

static
void SomeHashCI505450Print(char *arg, int *result)
{
	if (arg == NULL) {
		//OutputDebugString("50540: hashing nullptr\n");
	} else {
#ifdef HASH_HOOKS_TO_LOGFILE
		fwrite(buf, sprintf(buf, "hash\t%s\t%p\t505450\n", arg, *result), 1, logfile);
#endif
#ifdef HASH_HOOKS_TO_DEBUGSTRING
		sprintf(buf, "hash\t%s\t%p\t505450\n", arg, *result);
		OutputDebugString(buf);
#endif
	}
}

static
__declspec(naked) void SomeHashCI505450HookPost()
{
	_asm {
		mov edx, [esp+0x4]
		push eax // result on the stack
		pushad
		lea ecx, [esp+0x4*8]
		push ecx // ptr to result
		push edx // the arg
		call SomeHashCI505450Print
		add esp, 0x8
		popad
		pop eax // get result back (in case we modified it)
		ret
	}
}

static
__declspec(naked) void SomeHashCI505450HookPre()
{
	_asm {
		//; the overwritten instruction for our jump
		mov edx, [esp+0x4]
		or eax, 0xFFFFFFFF
		//; not pushing the arg because it's only used in the lines above
		//; ret to SomeHashHookPost
		push SomeHashCI505450HookPost
		//; call the actual function (using ret...)
		push 0x505457
		ret
	}
}

static
void PrintHashCI50B9C0Hook(
	/*pushad*/ int a, int b, int c, int d, int e, int f, int g, int h,
	int *result, int ignore, char *str)
{
	if (str != NULL) {
#ifdef HASH_HOOKS_TO_LOGFILE
		fwrite(buf, sprintf(buf, "hash\t%s\t%p\t50B9C0\n", str, ignore), 1, logfile);
#endif
#ifdef HASH_HOOKS_TO_DEBUGSTRING
		sprintf(buf, "hash\t%s\t%p\t50B9C0\n", str, ignore);
		OutputDebugString(buf);
#endif
	}
}

static
__declspec(naked) void SomeHashCI50B9C0Hook()
{
	_asm {
		push esp
		push eax
		push esp
		pushad
		call PrintHashCI50B9C0Hook
		popad
		add esp, 0x10C
		ret
	}
}

static
void initHashHooks()
{
#ifdef ENABLE_HASH_HOOK_505450
	mkjmp(0x105450, &SomeHashCI505450HookPre);
#endif
#ifdef ENABLE_HASH_HOOK_43DB50
	mkjmp(0x3DB50, &SomeHashCS43DB50HookPre);
#endif
#ifdef ENABLE_HASH_HOOK_50B9C0
	mkjmp(0x10BA0D, &SomeHashCI50B9C0Hook);
#endif

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHashHooks
}

/***********************************************************************************************
MAIN
*/

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason_for_call, LPVOID lpResrvd)
{
	DWORD oldvp;
	
	if (reason_for_call == DLL_PROCESS_ATTACH) {
		base = (int) GetModuleHandle(NULL);
		VirtualProtect((LPVOID) base, 280000, PAGE_EXECUTE_READWRITE, &oldvp);

		logfile = fopen("log.txt", "wb");
		INIT_FUNC();
	} else if (reason_for_call == DLL_PROCESS_DETACH) {
		if (logfile) {
			fclose(logfile);
		}
	}

	return 1;
}