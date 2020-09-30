static
void SomeHashCI505450Print(char *arg, int *result)
{
	hash_func_log(arg, *result, 0x505450);
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
void initHashHook505450()
{
	mkjmp(0x505450, &SomeHashCI505450HookPre);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHashHook505450
}
