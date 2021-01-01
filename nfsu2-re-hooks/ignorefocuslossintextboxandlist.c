static
__declspec(naked)
void focuslossMsgInMCListMessageHandlerHook()
{
	// Clicking out of the game window when in MC_List.fng (for example, on boot when
	// selecting a profile to load) will continue with the selected entry. This patch
	// makes sure it only continues when the cursor is _inside_ the game window instead
	// of clicking outside.
	_asm {
		mov eax, [0x8763D4] // int cursorX
		mov eax, [eax]
		cmp eax, 0
		jl no
		mov eax, 0x8763D4 // int cursorX
		mov edx, 0x797D58 // float canvasWidth
		fild dword ptr [eax]
		fcomp dword ptr [edx]
		fnstsw ax
		test ah, 1
		jz no
		mov eax, [0x8763D8] // int cursorY
		mov eax, [eax]
		cmp eax, 0
		jl no
		mov eax, 0x8763D8 // int cursorX
		mov edx, 0x797D54 // float canvasWidth
		fild dword ptr [eax]
		fcomp dword ptr [edx]
		fnstsw ax
		test ah, 1
		jz no
		// overriden stuff
		mov eax, 0x836538
		mov [eax], ecx
		// and return
		mov eax, 0x4E9010
		jmp eax

no:
		pop edi
		retn 0x10
	}
}

static
void initIgnoreFocuslossInTextboxAndList()
{
	// When in textbox (like entering name when creating a new profile),
	// clicking out of the game window (when windowed) acts as pressing return.
	// Pressing on the dialog itself also seem to do this.
	*(unsigned char*) 0x54E67B = 0xEB; // unconditional jmp

	mkjmp(0x4E900A, focuslossMsgInMCListMessageHandlerHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initIgnoreFocuslossInTextboxAndList
}
