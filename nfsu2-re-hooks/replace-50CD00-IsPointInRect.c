static
int IsPointInRectReplace(
	float x, float y,
	struct { float x; float y; } *pos,
	struct { float x; float y; } *size)
{
	/*If this stops working, check if x & y are not getting treated as qwords.
	It can happen when using original C-style parameter declarations
	but I have no idea why that is.*/
	return
		pos->x <= x && x < pos->x + size->x &&
		pos->y <= y && y < pos->y + size->y;
}

static
__declspec(naked) void IsPointInRectReplaceHook()
{
	_asm {
		mov eax, esp
		pushad
		push eax
		push [eax+0x10]
		push [eax+0xC]
		push [eax+0x8]
		push [eax+0x4]
		call IsPointInRectReplace
		add esp, 0x10
		// fucking juggle
		push eax // after: >(res) (frameesp)
		add esp, 0x4 // after: (res) >(frameesp)
		pop eax // after(eax=frameesp): (res) (frameesp) >
		sub esp, 0x8 // after: >(res) (frameesp)
		pop dword ptr [eax+4] // after: (res) >(frameesp)
		pop eax
		popad
		mov eax, [esp+0x4]
		ret
	}
}

static
void initIsPointInRectReplace()
{
	mkjmp(0x50CD00, &IsPointInRectReplaceHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initIsPointInRectReplace
}
