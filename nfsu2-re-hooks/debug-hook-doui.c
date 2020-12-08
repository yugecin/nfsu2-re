static
__declspec(naked) void doUIHook()
{
	_asm {
		mov eax, DEBUG_DOUI_FUNC
		jmp eax
		ret
	}
}

static
void initHookDoUI()
{
	mkjmp(0x5663F9, &doUIHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHookDoUI
}
