#include <winuser.h>
#include <commctrl.h>

#define ERRMSG(parent,text) MessageBoxA(parent,text,"oops",MB_ICONWARNING|MB_OK)

#define IDC_TAB 10
#define IDC_LABL 18
#define IDC_BTN_RENDERMODE_NORMAL 19
#define IDC_BTN_RENDERMODE_WIRE 20
#define IDC_BTN_RENDERMODE_PTS 21
#define IDC_BTN_RENDERMODE_FLATSHADE 22

HGDIOBJ font;
HMODULE hModule;
HWND hMain, hTab;
HWND hBtnRenderModeNormal, hBtnRenderModeWire, hBtnRenderModePts, hBtnRenderFlatshade;
#define numtabpanes 2
HWND hTabpane[numtabpanes];

static
void dbgw_create_tab_d3_controls(HWND hWnd)
{
	int x, y, w, h;
	HWND hLabel;

	x = 10; y = 10;
	w = 200; h = 20;

	hLabel =
	CreateWindowExA(0, "Static",
		"Change render mode:",
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		x,	y,
		w,	h,
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

	SendMessage(hLabel, WM_SETFONT, (WPARAM) font, 0);
	SendMessage(hBtnRenderModeNormal, WM_SETFONT, (WPARAM) font, 0);
	SendMessage(hBtnRenderModeWire, WM_SETFONT, (WPARAM) font, 0);
	SendMessage(hBtnRenderModePts, WM_SETFONT, (WPARAM) font, 0);
	SendMessage(hBtnRenderFlatshade, WM_SETFONT, (WPARAM) font, 0);

	SendMessage(hBtnRenderModeNormal, BM_SETCHECK, BST_CHECKED, 0);
}

static
void dbgw_create_main_window_controls(HWND hWnd)
{
	TCITEM ti = {0};
	RECT rc;
	int i;

	GetClientRect(hWnd, &rc);
	hTab =
	CreateWindowExA(0, "SysTabControl32",
		0,
		WS_CHILD | WS_VISIBLE,
		rc.left, rc.top,
		rc.right - rc.left, rc.bottom - rc.top,
		hWnd, (HMENU) IDC_TAB, hModule, 0);

	SendMessage(hTab, WM_SETFONT, (WPARAM) font, 0);

	ti.mask = TCIF_TEXT;
	ti.pszText = "UI";
	ti.cchTextMax = strlen(ti.pszText);
	SendMessage(hTab, TCM_INSERTITEM, 0, (LPARAM) &ti);
	ti.pszText = "d3d9";
	ti.cchTextMax = strlen(ti.pszText);
	SendMessage(hTab, TCM_INSERTITEM, 1, (LPARAM) &ti);

	SendMessage(hTab, TCM_ADJUSTRECT, 0, (LPARAM) &rc);
	for (i = 0; i < numtabpanes; i++) {
		hTabpane[i] =
		CreateWindowExA(0, "nfsu2-re-dbgw-child-class",
			0,
			WS_CHILD,
			rc.left, rc.top,
			rc.right - rc.left, rc.bottom - rc.top,
			hTab, 0, hModule, 0);
	}

	dbgw_create_tab_d3_controls(hTabpane[1]);
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
			d3device9_SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
			break;
		case IDC_BTN_RENDERMODE_WIRE:
			d3device9_SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
			break;
		case IDC_BTN_RENDERMODE_PTS:
			d3device9_SetRenderState(D3DRS_FILLMODE, D3DFILL_POINT);
			break;
		case IDC_BTN_RENDERMODE_FLATSHADE:
		{
			int mode;
			if (SendMessage(hBtnRenderFlatshade, BM_GETCHECK, 0, 0) == BST_CHECKED) {
				mode = D3DSHADE_FLAT;
			} else {
				mode = D3DSHADE_GOURAUD;
			}
			d3device9_SetRenderState(D3DRS_SHADEMODE, mode);
			break;
		}
		}
		break;
	case WM_NOTIFY:
	{
		RECT rc;
		NMHDR* nmhdr = (void*) lParam;
		int sel;

		if (nmhdr->hwndFrom == hTab) {
			if (nmhdr->code == TCN_SELCHANGING) {
				sel = SendMessage(hTab, TCM_GETCURSEL, 0, 0);
				ShowWindow(hTabpane[sel], SW_HIDE);
			} else if (nmhdr->code == TCN_SELCHANGE) {
				sel = SendMessage(hTab, TCM_GETCURSEL, 0, 0);
				GetClientRect(hMain, &rc);
				SendMessage(hTab, TCM_ADJUSTRECT, 0, (LPARAM) &rc);
				MoveWindow(hTabpane[sel],
					rc.left, rc.top,
					rc.right - rc.left, rc.bottom - rc.top,
					0);
				ShowWindow(hTabpane[sel], SW_SHOW);
				UpdateWindow(hTabpane[sel]);
			}
		}
		break;
	}
	case WM_KEYDOWN:
		break;
	case WM_CREATE:
	{
		CREATESTRUCTA* createstruct = (void*) lParam;

		if (!createstruct->hMenu && !createstruct->hwndParent) {
			dbgw_create_main_window_controls(hwnd);
		}
		break;
	}
	case WM_SIZE:
	{
		int selectedTabIndex;
		RECT rc;

		if (hwnd == hMain) {
			selectedTabIndex = SendMessage(hTab, TCM_GETCURSEL, 0, 0);
			GetClientRect(hwnd, &rc);
			SendMessage(hTab, TCM_ADJUSTRECT, 0, (LPARAM) &rc);
			MoveWindow(hTabpane[selectedTabIndex],
				rc.left, rc.top,
				rc.right - rc.left, rc.bottom - rc.top,
				0);
			GetClientRect(hwnd, &rc);
			MoveWindow(hTab,
				rc.left, rc.top,
				rc.right - rc.left, rc.bottom - rc.top,
				1);
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
	WNDCLASS childwc;
	WNDCLASSEX wc;
	INITCOMMONCONTROLSEX iccx;

	iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
	iccx.dwICC = ICC_TAB_CLASSES;
	if (!InitCommonControlsEx(&iccx)) {
		ERRMSG(NULL, "Failed init common controls.");
		return;
	}

	font = GetStockObject(DEFAULT_GUI_FONT);
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

	childwc.style = 0;
	childwc.lpfnWndProc = WndProc;
	childwc.cbClsExtra = 0;
	childwc.cbWndExtra = 0;
	childwc.hInstance = hModule;
	childwc.hIcon = LoadIcon(NULL, IDI_APPLICATION); /*large icon (alt tab)*/
	childwc.hCursor = LoadCursor(NULL, IDC_ARROW);
	childwc.hbrBackground = (HBRUSH) COLOR_WINDOW;
	childwc.lpszMenuName = NULL;
	childwc.lpszClassName = "nfsu2-re-dbgw-child-class";

	if (!RegisterClassEx(&wc) || !RegisterClass(&childwc)) {
		ERRMSG(NULL, "class reg failed.");
		return;
	}

	hMain = CreateWindowExA(
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
