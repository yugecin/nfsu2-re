/* vim: set filetype=c ts=8 noexpandtab: */

#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <windows.h>

static int base;
static FILE *logfile;

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
} 

/***********************************************************************************************
SOME HASH FUNCTIONS
*/

/*
When not defining HASH_HOOKS_TO_LOGFILE, it will be passed to OutputDebugString (use a debug
viewer like sysinternal's Dbgview.exe).
Outputing to logfile seems to be a thousand times faster, or I'm doing something wrong with
Dbgview.
*/
#define HASH_HOOKS_TO_LOGFILE
#define ENABLE_HASH_HOOKS

static char buf[1024];

static
void SomeHashCS43DB50Print(char *arg, int *result)
{
	if (arg == NULL) {
		//OutputDebugString("43DB50: hashing nullptr\n");
	} else {
#ifdef HASH_HOOKS_TO_LOGFILE
		fwrite(buf, sprintf(buf, "hash\t%s\t%p\t43DB50\n", arg, *result), 1, logfile);
#else
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
#else
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
void initHashHooks()
{
#ifdef ENABLE_HASH_HOOKS
	// called very often, but doesn't slow down too much
	mkjmp(0x105450, &SomeHashCI505450HookPre);
	// this one is called waaaay too much, will make startup time much longer
	mkjmp(0x3DB50, &SomeHashCS43DB50HookPre);
#endif
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
		initHashHooks();
		initConsolePOC();
	} else if (reason_for_call == DLL_PROCESS_DETACH) {
		if (logfile) {
			fclose(logfile);
		}
	}

	return 1;
}