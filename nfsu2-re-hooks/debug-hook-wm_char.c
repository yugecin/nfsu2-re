static
void onCharMessage(int wparam)
{
	log(buf, sprintf(buf, "wmchar %d", wparam));
	DEBUG_WMCHAR_FUNC(wparam);
}

static
__declspec(naked) void charMessageHook()
{
	_asm {
		call onCharMessage
		push 0x5CCF5A // ret addr
		mov eax, 0x55DBD0
		jmp eax
	}
}

static
void initHookCharMessage()
{
	mkjmp(0x5CCF55, &charMessageHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHookCharMessage
}
