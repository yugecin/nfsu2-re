static
int DidCursorPressInRectReplace(pos, size)
	struct { float x; float y; } *pos;
	struct { float w; float h; } *size;
{
	float cx, cy;

	if (_mouseData->areMouseButtonsSwapped) {
		if (!_mouseData->button1JustPressed) {
			return 0;
		}
	} else {
		if (!_mouseData->button0JustPressed) {
			return 0;
		}
	}
	cx = _mouseData->cursorX - *canvasWidth_2;
	cy = _mouseData->cursorY - *canvasHeight_2;
	return
		pos->x <= cx && cx < pos->x + size->w &&
		pos->y <= cy && cy < pos->y + size->h;
}

static
void initDidCursorPressInRectReplace()
{
	/*TODO: when does this actually get called?*/
	mkjmp(0x50CDB0, &DidCursorPressInRectReplace);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initDidCursorPressInRectReplace
}
