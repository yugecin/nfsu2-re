static
void debug_custom_endoutrun(int wparam)
{
	if (wparam == 121) { // y
		// this ends active outrun, makes you lose
		_asm { mov eax, 0x890118 }
		_asm { mov ecx, [eax] }
		_asm { mov eax, 0x5F8490 }
		_asm { call eax }
	}

	DEBUG_WMCHAR_FUNC(wparam);
}
#undef DEBUG_WMCHAR_FUNC
#define DEBUG_WMCHAR_FUNC debug_custom_endoutrun
