static
__declspec(naked) void doHookBeforePresent()
{
	_asm {
		call PRESENT_HOOK_FUNC
		mov eax, 0x870974
		mov eax, [eax]
		mov ecx, 0x5D2BBF
		jmp ecx
	}
}

static
void initHookBeforePresent()
{
	mkjmp(0x5D2BBA, &doHookBeforePresent);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHookBeforePresent
}
