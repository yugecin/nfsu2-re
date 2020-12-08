static int ytopoffset, ybotoffset;
static int xleftoffset, xrightoffset;

static
void loadMetrics(HWND hWnd)
{
	xleftoffset = GetSystemMetrics(SM_CXEDGE);
	xrightoffset = -xleftoffset;
	ytopoffset = GetSystemMetrics(SM_CYEDGE);
	ybotoffset = -ytopoffset;
	ytopoffset += GetSystemMetrics(SM_CYCAPTION);
}

static
__declspec(naked) void GetActiveResolutionHook()
{
	_asm {
		mov eax, 0x87098C
		mov dword ptr [eax], 1 // _runWindowed
		// __stdcall
		pop eax
		pop ecx
#if defined WIDESCREEN_MOD
		mov dword ptr [ecx], 1280 // width
#else
		mov dword ptr [ecx], 960 // width
#endif
		pop ecx
		mov dword ptr [ecx], 720 // height
		jmp eax
	}
}

static
__declspec(naked) int GetWindowRectPatched(HWND hWnd, LPRECT lpRect)
{
	/*Since a border was added, the rect values from GetWindowRect should be adjusted because the
	game expects a rect that is at the position of GetClientRect (or the cursor position will be wrong).*/
	_asm {
		push [esp+0x8]
		push [esp+0x8]
		mov eax, GetWindowRect
		call eax
		push eax
		push esi

		mov eax, ytopoffset
		test eax, eax
		jnz already_have_sizes
		pushad
		call loadMetrics
		popad
already_have_sizes:

		mov esi, [esp+0x10]
		mov eax, [esi] // left
		add eax, xleftoffset
		mov [esi], eax
		mov eax, [esi+0x4] // top
		add eax, ytopoffset
		mov [esi+0x4], eax
		mov eax, [esi+0x8] // right
		add eax, xrightoffset
		mov [esi+0x8], eax
		mov eax, [esi+0xC] // bottom
		add eax, ybotoffset
		mov [esi+0xC], eax

		mov eax, [esp+0x8]
		mov [esp+0x10], eax
		pop esi
		pop eax
		add esp, 0x8
		ret
	}
}

static
void initRunwindowed()
{
	// make windowed
	mkjmp(0x5BF610, &GetActiveResolutionHook);

	// and add title bar, and adjust offsets because game doesn't expect title bar
	// style passed to CreateWindowExA (was WS_POPUP)
	*(int*) 0x5D2665 = WS_OVERLAPPEDWINDOW;
	// style passed to SetWindowLongA (was WS_VISIBLE)
	*(int*) 0x5D279B = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
	*(int*) 0x783388 = (int) &GetWindowRectPatched;

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initRunwindowed
}
