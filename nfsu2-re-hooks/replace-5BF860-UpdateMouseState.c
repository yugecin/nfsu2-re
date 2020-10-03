static
void UpdateMouseStateReplace()
{
	struct DIMOUSESTATE2 {
		int lX;
		int lY;
		int lZ;
		char rbgButton0;
		char rbgButton1;
		char rbgButton2;
		char rbgButton3;
		char rbgButton4;
		char rbgButton5;
		char rbgButton6;
		char rbgButton7;
	} ms;
	EXPECT_SIZE(ms, 0x14);
	HRESULT result;
	char was0pressed, was1pressed, was2pressed;
	char state0, state1, state2;
	void *dinputdevice;
	int dinputdevice_vtable;
	int GetDeviceState;
	int Acquire;

	dinputdevice = _mouseData->dinputdevice;
	dinputdevice_vtable = *(int*) dinputdevice;
	GetDeviceState = *((int*) dinputdevice_vtable + 9);

	result = ((HRESULT (__stdcall *)(void*,int,void*))GetDeviceState)(dinputdevice,sizeof(ms),&ms);
	if (result/* != DI_OK*/) {
		Acquire = *((int*) dinputdevice_vtable + 7);
		/*If this is not done, mouse won't update anymore when coming back from unfocused state.*/
		do {
			result = ((HRESULT (__stdcall *)(void*))Acquire)(dinputdevice);
		} while (result == 0x8007001E /*DIERR_INPUTLOST*/);
		return;
	}

	UpdateCursorPositionReplace(); /*need to include cursorpositionreplace as well*/

	state0 = ms.rbgButton0 >> 7;
	state1 = ms.rbgButton1 >> 7;
	state2 = ms.rbgButton2 >> 7;
	_mouseData->mousestate_lZ = ms.lZ;
	was0pressed = _mouseData->button0State;
	was1pressed = _mouseData->button1State;
	was2pressed = _mouseData->button2State;
	_mouseData->button0State = state0;
	_mouseData->button1State = state1;
	_mouseData->button2State = state2;
	_mouseData->button0JustPressed = !was0pressed & state0;
	_mouseData->button1JustPressed = !was1pressed & state1;
	_mouseData->button2JustPressed = !was2pressed & state2;
	_mouseData->button0JustReleased = was0pressed & !state0;
	_mouseData->button1JustReleased = was1pressed & !state1;
	_mouseData->button2JustReleased = was2pressed & !state2;
}

static
void initUpdateMouseStateReplace()
{
	mkjmp(0x5BF860, &UpdateMouseStateReplace);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initUpdateMouseStateReplace
}
