static
void UpdateCursorPositionReplace()
{
	int previousX, previousY;
	int relpos;
	RECT windowRect, clientRect;
	POINT cursorPoint;

	_mouseData->previousCursorX = _mouseData->cursorX;
	_mouseData->previousCursorY = _mouseData->cursorY;

	GetWindowRect(*hwnd, &windowRect);
	GetClientRect(*hwnd, &clientRect);
	GetCursorPos(&cursorPoint);

#ifdef NFSU2_RUN_WINDOWED
	windowRect.left += xleftoffset;
	windowRect.top += ytopoffset;
	windowRect.right += xrightoffset;
	windowRect.bottom += ybotoffset;
#endif

	relpos = cursorPoint.y - windowRect.top;
	if (clientRect.bottom) {
		_mouseData->cursorY = (int) (relpos * 480.0f / clientRect.bottom);
	} else {
		_mouseData->cursorY = 0;
	}

	relpos = cursorPoint.x - windowRect.left;
	if (clientRect.right) {
#ifdef WIDESCREEN_MOD
		_mouseData->cursorX = (int) (relpos * 853.333313f / (clientRect.right - clientRect.left));
#else
		_mouseData->cursorX = (int) (relpos * 640.0f / (clientRect.right - clientRect.left));
#endif
	} else {
		_mouseData->cursorX = 0;
	}

	_mouseData->deltaCursorX = _mouseData->cursorX - _mouseData->previousCursorX;
	_mouseData->deltaCursorY = _mouseData->cursorY - _mouseData->previousCursorY;
}

static
void initUpdateCursorPositionReplace()
{
	mkjmp(0x5BF750, &UpdateCursorPositionReplace);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initUpdateCursorPositionReplace
}
