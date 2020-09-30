static
void PrintHashCI50B9C0Hook(
	/*pushad*/ int a, int b, int c, int d, int e, int f, int g, int h,
	int result, char *str)
{
	hash_func_log(str, result, 0x50B9C0);
}

static
__declspec(naked) void SomeHashCI50B9C0Hook()
{
	_asm {
		push esp
		push eax
		pushad
		call PrintHashCI50B9C0Hook
		popad
		add esp, 0x108
		ret
	}
}

static
void initHashHook50B9C0()
{
	mkjmp(0x50BA0D, &SomeHashCI50B9C0Hook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHashHook50B9C0
}
