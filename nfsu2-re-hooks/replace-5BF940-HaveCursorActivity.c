static
int HaveCursorActivityReplace()
{
	return
		_mouseData->deltaCursorX || _mouseData->deltaCursorY ||
		_mouseData->button0JustPressed || _mouseData->button0JustReleased ||
		_mouseData->button1JustPressed || _mouseData->button1JustReleased ||
		_mouseData->button2JustPressed || _mouseData->button2JustReleased ||
		_mouseData->mousestate_lZ;
}

static
void initReplaceHaveCursorActivity()
{
	mkjmp(0x5BF940, &HaveCursorActivityReplace);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initReplaceHaveCursorActivity
}
