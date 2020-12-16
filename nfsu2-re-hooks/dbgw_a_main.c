#include <winuser.h>

#define ERRMSG(parent,text) MessageBoxA(parent,text,"oops",MB_ICONWARNING|MB_OK)

#define IDC_LABL 18
#define IDC_BTN_RENDERMODE_NORMAL 19
#define IDC_BTN_RENDERMODE_WIRE 20
#define IDC_BTN_RENDERMODE_PTS 21
#define IDC_BTN_RENDERMODE_FLATSHADE 22

HFONT hfDefault;
HMODULE hModule;
HWND hMain, hBtnRenderModeNormal, hBtnRenderModeWire, hBtnRenderModePts, hBtnRenderFlatshade;

static
void dbgw_create_main_window(HWND hWnd)
{
	int x, y, w, h;
	HWND hLabel;

	x = 10; y = 10;
	w = 200; h = 20;

	hLabel =
	CreateWindowExA(0, "Static",
		"Change render mode:",
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		x,		y,
		w,		h,
		hWnd, (HMENU) IDC_LABL, hModule, 0);
	x += 10;
	y += h;
	hBtnRenderModeNormal =
	CreateWindowExA(0, "Button",
		"Normal",
		WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
		x,	y,
		w,	h,
		hWnd, (HMENU) IDC_BTN_RENDERMODE_NORMAL, hModule, 0);
	y += h;
	hBtnRenderModeWire =
	CreateWindowExA(0, "Button",
		"Wireframe",
		WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
		x,	y,
		w,	h,
		hWnd, (HMENU) IDC_BTN_RENDERMODE_WIRE, hModule, 0);
	y += h;
	hBtnRenderModePts =
	CreateWindowExA(0, "Button",
		"Points",
		WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
		x,	y,
		w,	h,
		hWnd, (HMENU) IDC_BTN_RENDERMODE_PTS, hModule, 0);
	y += h;
	hBtnRenderFlatshade =
	CreateWindowExA(0, "Button",
		"Flatshading",
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		x,	y,
		w,	h,
		hWnd, (HMENU) IDC_BTN_RENDERMODE_FLATSHADE, hModule, 0);

	SendMessage(hLabel, WM_SETFONT, (WPARAM) hfDefault, MAKELPARAM(FALSE, 0));
	SendMessage(hBtnRenderModeNormal, WM_SETFONT, (WPARAM) hfDefault, MAKELPARAM(FALSE, 0));
	SendMessage(hBtnRenderModeWire, WM_SETFONT, (WPARAM) hfDefault, MAKELPARAM(FALSE, 0));
	SendMessage(hBtnRenderModePts, WM_SETFONT, (WPARAM) hfDefault, MAKELPARAM(FALSE, 0));
	SendMessage(hBtnRenderFlatshade, WM_SETFONT, (WPARAM) hfDefault, MAKELPARAM(FALSE, 0));

	SendMessage(hBtnRenderModeNormal, BM_SETCHECK, BST_CHECKED, 0);
}

static
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		// https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3drenderstatetype
		// https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dfillmode
		// https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dshademode
		switch (LOWORD(wParam)) {
		case IDC_BTN_RENDERMODE_NORMAL:
			d3device9_SetRenderState(8/*D3DRS_FILLMODE*/, 3/*D3DFILL_SOLID*/);
			break;
		case IDC_BTN_RENDERMODE_WIRE:
			d3device9_SetRenderState(8/*D3DRS_FILLMODE*/, 2/*D3DFILL_WIREFRAME*/);
			break;
		case IDC_BTN_RENDERMODE_PTS:
			d3device9_SetRenderState(8/*D3DRS_FILLMODE*/, 1/*D3DFILL_POINT*/);
			break;
		case IDC_BTN_RENDERMODE_FLATSHADE:
		{
			int mode;
			if (SendMessage(hBtnRenderFlatshade, BM_GETCHECK, 0, 0) == BST_CHECKED) {
				mode = 1/*D3DSHADE_FLAT*/;
			} else {
				mode = 2/*D3DSHADE_GOURAUD*/;
			}
			d3device9_SetRenderState(9/*D3DRS_SHADEMODE*/, mode);
			break;
		}
		}
		break;
	case WM_KEYDOWN:
		break;
	case WM_CREATE:
		dbgw_create_main_window(hwnd);
		break;
	case WM_SIZE:
	{
		RECT rcClient;

		if (hwnd == hMain) {
		}
		break;
	}
	default: return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

static
void dbgw_init()
{
	MSG msg;
	WNDCLASSEX wc;
	NONCLIENTMETRICS ncm;

	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
	hfDefault = CreateFontIndirect(&ncm.lfMessageFont);
	hModule = GetModuleHandleA(NULL);

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hModule;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); /*large icon (alt tab)*/
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "nfsu2-re-dbgw-class";
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION); /*small icon (taskbar)*/

	if (!RegisterClassEx(&wc)) {
		ERRMSG(NULL, "Window reg failed.");
		return;
	}

	hMain = CreateWindowEx(
		0,
		wc.lpszClassName,
		"nfsu2-re-dbgw",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 300, 224,
		NULL, NULL, hModule, NULL
	);
	if (hMain == NULL) {
		ERRMSG(NULL, "Window creation failed.");
		return;
	}

	ShowWindow(hMain, 1);
	UpdateWindow(hMain);
}

static
__declspec(naked) void CreateWindowHook()
{
	_asm { mov eax, 0x5D24F0 }
	_asm { call eax }
	_asm { jmp dbgw_init }
}

static
void initDbgw()
{
	mkjmp(0x48CE08, CreateWindowHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initDbgw
}
