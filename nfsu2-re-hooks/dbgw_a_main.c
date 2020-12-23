#include <winuser.h>
#include <commctrl.h>

#define ERRMSG(parent,text) MessageBoxA(parent,text,"oops",MB_ICONWARNING|MB_OK)

#define IDC_TAB 10
#define IDC_LABL 18
#define IDC_BTN_RENDERMODE_NORMAL 19
#define IDC_BTN_RENDERMODE_WIRE 20
#define IDC_BTN_RENDERMODE_PTS 21
#define IDC_BTN_RENDERMODE_FLATSHADE 22
#define IDC_UITREE 23

#define UI_LIST_ITM_MAX_LEN 128
struct UiListItem {
	HTREEITEM itm;
	unsigned int hash;
	void *ptr;
};
EXPECT_SIZE(struct UiListItem, 0xC);
#define MAX_UI_LIST_ITEMS 1000
struct UiListItem uiListItems[MAX_UI_LIST_ITEMS];
char uiListItemStrings[MAX_UI_LIST_ITEMS][UI_LIST_ITM_MAX_LEN];
char uiListItemStillExists[MAX_UI_LIST_ITEMS];
int numUiListItems;

HGDIOBJ font;
HMODULE hModule;
HWND hMain, hTab;
HWND hBtnRenderModeNormal, hBtnRenderModeWire, hBtnRenderModePts, hBtnRenderFlatshade;
#define numtabpanes 2
HWND hTabpane[numtabpanes];
HWND hUITree;

static
int dbgw_ui_tree_is_fng(void *maybeFng)
{
	struct FNGInfo *currentFng;

	currentFng = pUIData[0]->field_8->topPackage;
	while (currentFng) {
		if (currentFng == maybeFng) {
			return 1;
		}
		currentFng = currentFng->child;
	}
	return 0;
}

static
struct UIElement *dbgw_ui_tree_get_uielement_for_tree_item(HTREEITEM itm)
{
	TVITEM itemdata;

	itemdata.hItem = itm;
	itemdata.mask = TVIF_PARAM;
	if (itm && SendMessage(hUITree, TVM_GETITEM, 0, (LPARAM) &itemdata)) {
		if (!dbgw_ui_tree_is_fng((void*) itemdata.lParam)) {
			return (void*) itemdata.lParam;
		}
	}
	return 0;
}

static
int dbgw_ui_tree_get_rect_for_tree_item(HTREEITEM itm, struct U2RECT *rect)
{
	struct UIElement *element;
	struct U2RECT rect2;

	element = dbgw_ui_tree_get_uielement_for_tree_item(itm);
	if (!element) {
		return 0;
	}
	((void (__cdecl *)(void*,struct U2RECT*))0x51D9F0)(element, rect);
	for (;;) {
		itm = (HTREEITEM) SendMessage(hUITree, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM) itm);
		element = dbgw_ui_tree_get_uielement_for_tree_item(itm);
		if (!element) {
			return 1;
		}
		((void (__cdecl *)(void*,struct U2RECT*))0x51D9F0)(element, &rect2);
		rect->top += (rect2.top + rect2.bottom) / 2;
		rect->bottom += (rect2.top + rect2.bottom) / 2;
		rect->left += (rect2.left + rect2.right) / 2;
		rect->right += (rect2.left + rect2.right) / 2;
	}
}

static
HTREEITEM dbgw_ui_tree_find_item(void *ptr, unsigned int hash, int *out_idx)
{
	int i;

	for (i = 0; i < numUiListItems; i++) {
		if (uiListItems[i].ptr == ptr && uiListItems[i].hash == hash) {
			*out_idx = i;
			uiListItemStillExists[i] = 1;
			return uiListItems[i].itm;
		}
	}
	return NULL;
}

static
HTREEITEM dbgw_ui_tree_ensure_item(
	void *ptr,
	unsigned int hash,
	char *text,
	int textlen,
	HTREEITEM parent,
	HTREEITEM after,
	int hasChildren)
{
	HTREEITEM itm;
	TVITEMA tvi;
	TVINSERTSTRUCTA tvis;
	int idx;

	itm = dbgw_ui_tree_find_item(ptr, hash, &idx);
	if (itm) {
		if (strcmp(text, uiListItemStrings[idx])) {
			strcpy(uiListItemStrings[idx], text);
			tvi.mask = TVIF_TEXT;
			tvi.cchTextMax = textlen;
			tvi.pszText = text;
			tvi.hItem = itm;
			SendMessage(hUITree, TVM_SETITEM, 0, (LPARAM) &tvi);
		}
	} else {
		tvis.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
		tvis.item.pszText = text;
		tvis.item.cchTextMax = textlen;
		tvis.item.cChildren = hasChildren;
		tvis.item.lParam = (LPARAM) ptr;
		tvis.hInsertAfter = after ? after : TVI_FIRST;
		tvis.hParent = parent;
		itm = (HTREEITEM) SendMessage(hUITree, TVM_INSERTITEM, 0, (LPARAM) &tvis);
		if (numUiListItems < MAX_UI_LIST_ITEMS) {
			uiListItems[numUiListItems].hash = hash;
			uiListItems[numUiListItems].ptr = ptr;
			uiListItems[numUiListItems].itm = itm;
			uiListItemStillExists[numUiListItems] = 1;
			strcpy(uiListItemStrings[numUiListItems], text);
			numUiListItems++;
		}
	}
	return itm;
}

static
void dbgw_ui_tree_element_format(struct UIElement *element, char *dst, int *out_len)
{
	int len;
	short *src;
	struct UIElement *child;
	int numchilds;

	if (element->type == 5) {
		numchilds = 0;
		child = ((struct UIContainer*) element)->children;
		while (child) {
			numchilds++;
			child = child->nextSibling;
		}
		len = sprintf(dst, "container %08x (%d)", element->hash, numchilds);
	} else if (element-> type == 2) {
		len = sprintf(dst, "label %08x: ", element->hash);
		src = ((struct UILabel*) element)->string.ptrString;
		if (src) {
			dst += len;
			while ((*(dst++) = (0xFF & *(src++)))) {
				if (++len >= UI_LIST_ITM_MAX_LEN) {
					break;
				}
			}
		} else {
			len += sprintf(buf + len, "(null)");
		}
	} else {
		len = sprintf(dst, "type %d %08x", element->type, element->hash);
	}
	*out_len = len;
}

static
int fnginfo_get_child_level(void *fng)
{
	struct FNGInfo *currentFng;
	int level;

	level = 0;
	currentFng = pUIData[0]->field_8->topPackage;
	while (currentFng) {
		if (currentFng == fng) {
			break;
		}
		level++;
		currentFng = currentFng->child;
	}
	return level;
}

static
int CALLBACK dbgw_ui_tree_compare_fng_tree_items(LPARAM a, LPARAM b, LPARAM lParam)
{
	return fnginfo_get_child_level((void*) a) - fnginfo_get_child_level((void*) b);
}

static
void dbgw_ui_tree_update_before_present()
{
#define MAX_CONTAINER_STACK 20
	struct UIElement *containerStack[MAX_CONTAINER_STACK];
	int containerStackSize;
	HTREEITEM lastItems[MAX_CONTAINER_STACK];
	HTREEITEM itm;
	TVSORTCB tvsortcb;
	struct U2RECT u2rect;
	struct Vert verts[5];
	int level;
	struct FNGInfo *fng;
	struct UIElement *element;
	char buf[UI_LIST_ITM_MAX_LEN];
	int len;
	int i;
	int anyElementsInserted;
	int lastElementCount;

	if (!hMain) {
		return;
	}

	if (!(GetWindowLongA(hTabpane[0], GWL_STYLE) & WS_VISIBLE)) {
		return;
	}

	memset(uiListItemStillExists, 0, sizeof(uiListItemStillExists));
	memset(lastItems, 0, sizeof(lastItems));
	lastElementCount = numUiListItems;
	level = 0;
	containerStackSize = 0;
	fng = pUIData[0]->field_8->topPackage;
	while (fng) {
		len = sprintf(buf, "fng %08x %s", fng->hash, fng->fngName);
		itm = dbgw_ui_tree_ensure_item(fng, fng->hash, buf, len, 0, lastItems[0], 1);
		lastItems[0] = itm;
		element = fng->rootUIElement;
		level = 1;
		for (;;) {
			if (!element) {
				if (containerStackSize) {
					lastItems[level] = 0;
					level--;
					containerStackSize--;
					element = containerStack[containerStackSize]->nextSibling;
					continue;
				}
				break;
			}

			dbgw_ui_tree_element_format(element, buf, &len);
			itm = dbgw_ui_tree_ensure_item(
				element, element->hash,
				buf, len,
				lastItems[level - 1], lastItems[level],
				element->type == 5 && ((struct UIContainer*) element)->children);
			lastItems[level] = itm;

			if (element->type == 5 && containerStackSize < MAX_CONTAINER_STACK) {
				containerStack[containerStackSize] = element;
				containerStackSize++;
				level++;
				element = ((struct UIContainer*) element)->children;
			} else {
				element = element->nextSibling;
			}
		}
		fng = fng->child;
	} while (fng);
	anyElementsInserted = lastElementCount != numUiListItems;

	for (i = 0; i < numUiListItems; i++) {
		if (!uiListItemStillExists[i]) {
			SendMessage(hUITree, TVM_DELETEITEM, 0, (LPARAM) uiListItems[i].itm);
			numUiListItems--;
			uiListItems[i] = uiListItems[numUiListItems];
			strcpy(uiListItemStrings[i], uiListItemStrings[numUiListItems]);
		}
	}

	if (anyElementsInserted) {
		tvsortcb.hParent = 0;
		tvsortcb.lpfnCompare = dbgw_ui_tree_compare_fng_tree_items;
		SendMessage(hUITree, TVM_SORTCHILDRENCB, 0, (LPARAM) &tvsortcb);
	}

	verts[0].spec = verts[1].spec = verts[2].spec
	= verts[3].spec = verts[4].spec = 0;

	itm = (HTREEITEM) SendMessage(hUITree, TVM_GETNEXTITEM, TVGN_CARET, 0);
	if (dbgw_ui_tree_get_rect_for_tree_item(itm, &u2rect)) {
		u2rect.top /= -*canvasHeight_2;
		u2rect.bottom /= -*canvasHeight_2;
		u2rect.left /= *canvasWidth_2;
		u2rect.right /= *canvasWidth_2;
		verts[0].x = u2rect.left;
		verts[0].y = u2rect.top;
		verts[1].x = u2rect.right;
		verts[1].y = u2rect.top;
		verts[2].x = u2rect.right;
		verts[2].y = u2rect.bottom;
		verts[3].x = u2rect.left;
		verts[3].y = u2rect.bottom;
		verts[4].x = u2rect.left;
		verts[4].y = u2rect.top;
		verts[0].z = verts[1].z = verts[2].z = verts[3].z = verts[4].z = 0.0f;
		verts[0].col = verts[1].col = verts[2].col
		= verts[3].col = verts[4].col = 0xFFFF00FF;
		d3d9_draw_line_strip(verts, 4);
	}

	u2rect.top = -(_mouseData->cursorY * 2 / *canvasHeight - 1.0f) - .05f;
	u2rect.left = _mouseData->cursorX * 2 / *canvasWidth - 1.0f - .05f;
	u2rect.right = u2rect.left + .1f;
	u2rect.bottom = u2rect.top + .1f;
	verts[0].x = u2rect.left;
	verts[0].y = u2rect.top;
	verts[1].x = u2rect.right;
	verts[1].y = u2rect.top;
	verts[2].x = u2rect.right;
	verts[2].y = u2rect.bottom;
	verts[3].x = u2rect.left;
	verts[3].y = u2rect.bottom;
	verts[4].x = u2rect.left;
	verts[4].y = u2rect.top;
	verts[0].z = verts[1].z = verts[2].z = verts[3].z = verts[4].z = 0.0f;
	verts[0].col = verts[1].col = verts[2].col
	= verts[3].col = verts[4].col = 0xFF00FFFF;
	d3d9_draw_line_strip(verts, 4);

	PRESENT_HOOK_FUNC();
#undef PRESENT_HOOK_FUNC
#define PRESENT_HOOK_FUNC dbgw_ui_tree_update_before_present
}

static
void dbgw_create_tab_ui_controls(HWND hWnd)
{
	RECT rc;

	GetClientRect(hWnd, &rc);
	hUITree =
	CreateWindowEx(0, WC_TREEVIEW,
		0,
		TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_DISABLEDRAGDROP
		| WS_CHILD | WS_VISIBLE | TVS_SHOWSELALWAYS,
		rc.left, rc.top,
		rc.right, rc.bottom,
		hWnd, (HMENU) IDC_UITREE, hModule, 0);
}

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
			i == 0 ? WS_CHILD | WS_VISIBLE : WS_CHILD,
			rc.left, rc.top,
			rc.right - rc.left, rc.bottom - rc.top,
			hTab, 0, hModule, 0);
	}

	dbgw_create_tab_d3_controls(hTabpane[1]);
	dbgw_create_tab_ui_controls(hTabpane[0]);
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
		} else if (hwnd == hTabpane[0]) {
			GetClientRect(hwnd, &rc);
			MoveWindow(hUITree,
				rc.left, rc.top,
				rc.right - rc.left, rc.bottom - rc.top,
				1);
		}
		break;
	}
	case WM_DESTROY:
		hMain = 0;
		break;
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
failinitcomm:
		ERRMSG(NULL, "Failed init common controls.");
		return;
	}
	iccx.dwICC = ICC_TREEVIEW_CLASSES;
	if (!InitCommonControlsEx(&iccx)) {
		goto failinitcomm;
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
		CW_USEDEFAULT, CW_USEDEFAULT, 333, 250,
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
