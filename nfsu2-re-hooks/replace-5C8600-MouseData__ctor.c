static
void MouseData__ctorReplace()
{
	_mouseData->dinputdevice = 0;
	_mouseData->button0State = _mouseData->button0JustPressed = _mouseData->button0JustReleased = 0;
	_mouseData->button1State = _mouseData->button1JustPressed = _mouseData->button1JustReleased = 0;
	_mouseData->button2State = _mouseData->button2JustPressed = _mouseData->button2JustReleased = 0;
	UpdateCursorPositionReplace(); /*need to include cursorpositionreplace as well*/
	_mouseData->areMouseButtonsSwapped = GetSystemMetrics(SM_SWAPBUTTON);
}

static
void initMouseData__ctorReplace()
{
	mkjmp(0x5C8600, &MouseData__ctorReplace);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initMouseData__ctorReplace
}
