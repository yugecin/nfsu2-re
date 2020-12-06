static
void debug_custom_remove_all_received_engage_markers(int wparam)
{
	if (wparam == 121) { // y
		_asm { mov ecx, 0x85AD40 }
		((void(__stdcall *)(void))0x528980)();
	}

	DEBUG_WMCHAR_FUNC(wparam);
}
#undef DEBUG_WMCHAR_FUNC
#define DEBUG_WMCHAR_FUNC debug_custom_remove_all_received_engage_markers
