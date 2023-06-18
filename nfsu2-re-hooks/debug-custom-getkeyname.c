static
__declspec(naked)
char* __stdcall ControllerStruct874C40__GetKeyName(int keyIndex, int isSecondary)
{
	_asm {
		mov ecx, 0x874C40
		//add ecx, 0x370 // second controller... crashes if my gamepad is not connected ofc
		mov eax, 0x5C20E0
		jmp eax
	}
}

static
__declspec(naked)
char* __stdcall ControllerStruct874C40__GetKeyboardKeyName(int keyIndex, int isSecondary)
{
	_asm {
		mov ecx, 0x874C40
		//add ecx, 0x370 // second controller... crashes if my gamepad is not connected ofc
		mov eax, 0x5B8D90
		jmp eax
	}
}

static
__declspec(naked)
char* __stdcall ControllerStruct874C40__GetControllerKeyName(int keyIndex, int isSecondary)
{
	_asm {
		mov ecx, 0x874C40
		//add ecx, 0x370 // second controller... crashes if my gamepad is not connected ofc
		mov eax, 0x5B8A90
		jmp eax
	}
}

static
void debug_custom_uielementvisitor(int wparam)
{
	int i;
	char *prim, *secn;

	if (wparam == 121) { // y
		for (i = 0; i < 30; i++) {
			prim = ControllerStruct874C40__GetKeyName(i, 0);
			secn = ControllerStruct874C40__GetKeyName(i, 1);
			log(buf, sprintf(buf, "key %d     : prim: %s sec: %s", i, prim, secn));
			prim = ControllerStruct874C40__GetKeyboardKeyName(i, 0);
			secn = ControllerStruct874C40__GetKeyboardKeyName(i, 1);
			log(buf, sprintf(buf, "key %d keyb: prim: %s sec: %s", i, prim, secn));
			prim = ControllerStruct874C40__GetControllerKeyName(i, 0);
			secn = ControllerStruct874C40__GetControllerKeyName(i, 1);
			log(buf, sprintf(buf, "key %d cont: prim: %s sec: %s", i, prim, secn));
		}
	}

	DEBUG_WMCHAR_FUNC(wparam);
}
#undef DEBUG_WMCHAR_FUNC
#define DEBUG_WMCHAR_FUNC debug_custom_uielementvisitor
