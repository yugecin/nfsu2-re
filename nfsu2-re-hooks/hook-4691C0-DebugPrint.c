static
__declspec(naked) void DebugPrint4691C0_PrintWhenDone()
{
	_asm {
		pusha
		push 0x82C738 // char audioDebugString[512]
		call strlen
		push eax
		push 0x82C738 // char audioDebugString[512]
		call log
		add esp, 0xC
		popa
		add esp, 0x10 // overwrote this
		ret // it was the end of the proc so we can just return
	}
}

static
__declspec(naked) void DebugPrint4691C0_EnsureEnabled()
{
	// first arg must be 1 or the function won't actually write the debugstring to the destination
	// enforce that here
	_asm {
		sub esp, 0x10 // overwrote this
		// just jmp over the check
		mov eax, 0x4691CB
		jmp eax
	}
}

static
void initHookRecv()
{
	mkjmp(0x4691C0, &DebugPrint4691C0_EnsureEnabled);
	mkjmp(0x4691F8, &DebugPrint4691C0_PrintWhenDone);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initHookRecv
}
