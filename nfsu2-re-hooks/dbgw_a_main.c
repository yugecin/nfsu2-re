#include <winuser.h>
#include <commctrl.h>

#define ERRMSG(parent,text) MessageBoxA(parent,text,"oops",MB_ICONWARNING|MB_OK)

#define IDC_LABL 8
#define IDC_LABL_FLAGS 9
#define IDC_TAB 10
#define IDC_BTN_RENDERMODE_NORMAL 19
#define IDC_BTN_RENDERMODE_WIRE 20
#define IDC_BTN_RENDERMODE_PTS 21
#define IDC_BTN_RENDERMODE_FLATSHADE 22
#define IDC_UITREE 23
#define IDC_EDIT_UIPROP_X 24
#define IDC_EDIT_UIPROP_Y 25
#define IDC_EDIT_UIPROP_LBLLANGSTR 26
#define IDC_EDIT_UIPROP_ADDR 27
#define IDC_BTN_UIPROP_HIDEALL 28
#define IDC_BTN_UIPROP_SHOWALL 29
#define IDC_EDIT_UIPROP_LBLSTR 30
#define IDC_BTN_PCHELPBAR_HIDE 31
#define IDC_BTN_PCHELPBAR_SHOW 32

#define IDC_BTN_UIPROP_FLAG1 9024
#define IDC_BTN_UIPROP_FLAG32 9055
#define IDC_BTN_PCHELPBARMASK_FLAG1 9056
#define IDC_BTN_PCHELPBARMASK_FLAG32 9087

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

#define UIPROPS_WIDTH 350
#define UITREE_MINWIDTH 200

#define PX_PADDING 10
#define PX_INDENT 20
#define PX_SPACING_Y 18

const char *flagnames[32] = {
	"hidden", "use custom text",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"needs update(0)", "needs update(1)", "needs update(2)", "", "", "", "", "", "", ""
};

const char *pcHelpBarButtons[32] = {
	"PC_NAV_QUIT", "PC_NAV_LOGOFF", "PC_NAV_CONTINUE",
	"PC_NAV_INSTALL_PART", "PC_NAV_INSTALL_VINYL",
	"PC_NAV_INSTALL_DECAL", "PC_NAV_INSTALL_PACKAGE",
	"PC_NAV_INSTALL_PAINT", "PC_NAV_READ_MESSAGE", "CANCEL_CHANGES", "PC_NAV_BACK",
	"DYNO_RESET/PC_NAV_RESET_KEYS", "PC_NAV_CUSTOMIZE", "PC_NAV_DELETE",
	"PC_NAV_EA_MESSENGER",
	"LAYOUT_WORLDMAP_POSITION", "LAYOUT_ONE_LINE_NO_BG", "LAYOUT_PAUSEMENU_POSITION",
	"DELETE_TUNED_CAR",
	"ICE_TEST_NOS_PURGE", "ICE_OPEN_CLOSE_HOOD", "ICE_OPEN_CLOSE_DOORS",
	"DYNO_TIP", "DECAL_COLOR/FE_REPAINT", "MU_PAUSE_RESTART",
	"STRING_WORLD_MAP_SELECT", "FILTER_EVENT_OFF", "FILTER_EVENT_ON",
	"ACTIVATE_GPS", "DEACTIVATE_GPS", "OL_HOST_LAN_SERVER", "LAYOUT_RIGHT_POSITION"
};

#define DBGW_UI_LABEL_STRING_MAX_LEN 512

HGDIOBJ font;
HMODULE hModule;
HWND hMain, hTab;
HWND hBtnRenderModeNormal, hBtnRenderModeWire, hBtnRenderModePts, hBtnRenderFlatshade;
#define numtabpanes 3
HWND hTabpane[numtabpanes];
struct {
	HWND window;
	HWND addr;
	HWND chk_flags[32];
	HWND posX, posY;
	HWND lblLangStr;
	HWND lblStr;
	char wasLblLangStrReadOnly;
	char wasLblStrReadOnly;
	unsigned int lastFlags;
} hUiProp;
struct {
	HWND buttonBits[32];
	unsigned int lastButtonMask;
} hPcHelpBar;
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
struct UIElement *dbgw_get_selected_uielement()
{
	HTREEITEM itm;

	itm = (HTREEITEM) SendMessage(hUITree, TVM_GETNEXTITEM, TVGN_CARET, 0);
	return dbgw_ui_tree_get_uielement_for_tree_item(itm);
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
	char *str;
	short *src;
	struct UIElement *child;
	struct UILabel *label;
	int numchilds;

	if (element->type == 5) {
		numchilds = 0;
		child = ((struct UIContainer*) element)->children;
		while (child) {
			numchilds++;
			child = child->nextSibling;
		}
		len = sprintf(dst, "container %08x (%d)", element->hash, numchilds);
	} else if (element->type == 2) {
		len = sprintf(dst, "label %08x: ", element->hash);
		label = (void*) element;
		str = 0;
		if (!(element->someFlags & UIELEMENT_FLAG_USE_CUSTOM_TEXT)) {
			str = GetLanguageStringOrNull(label->textLanguageString);
		}
		if (str) {
			len += sprintf(dst + len, "%s", str);
		} else {
			src = label->string.ptrString;
			if (src) {
				dst += len;
				while ((*(dst++) = (0xFF & *(src++)))) {
					if (++len >= UI_LIST_ITM_MAX_LEN) {
						break;
					}
				}
			} else {
				len += sprintf(dst + len, "(null)");
			}
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
void dbgw_pchelpbar_update_before_present()
{
	unsigned int currentMask;
	int i;

	currentMask = pcHelpbar[0]->currentButtonMask;
	if (hPcHelpBar.lastButtonMask != currentMask) {
		hPcHelpBar.lastButtonMask = currentMask;
		for (i = 0; i < 32; i++) {
			if (currentMask & (1 << i)) {
				SendMessage(hPcHelpBar.buttonBits[i], BM_SETCHECK, BST_CHECKED, 0);
			} else {
				SendMessage(hPcHelpBar.buttonBits[i], BM_SETCHECK, BST_UNCHECKED, 0);
			}
		}
	}
}

static
void dbgw_ui_props_update_before_present()
{
	struct UIElement *uielement;
	struct UILabel *lbl;
	char buf1[32];
	char buf2[32];
	char buf3[DBGW_UI_LABEL_STRING_MAX_LEN];
	char *b;
	short *w;
	int i;

	if (
		!(GetWindowLongA(hTabpane[0], GWL_STYLE) & WS_VISIBLE) ||
		!(GetWindowLongA(hUiProp.window, GWL_STYLE) & WS_VISIBLE) ||
		!(uielement = dbgw_get_selected_uielement()))
	{
		return;
	}

	if (hUiProp.lastFlags != uielement->someFlags) {
		hUiProp.lastFlags = uielement->someFlags;
		for (i = 0; i < 32; i++) {
			if (uielement->someFlags & (1 << i)) {
				SendMessage(hUiProp.chk_flags[i], BM_SETCHECK, BST_CHECKED, 0);
			} else {
				SendMessage(hUiProp.chk_flags[i], BM_SETCHECK, BST_UNCHECKED, 0);
			}
		}
	}
	if (uielement->pos) {
		sprintf(buf2, "%.2f", uielement->pos->leftOffset);
	} else {
		strcpy(buf2, "(null)");
	}
	SendMessage(hUiProp.posX, WM_GETTEXT, sizeof(buf1), (LPARAM) buf1);
	if (strcmp(buf1, buf2)) {
		SendMessage(hUiProp.posX, WM_SETTEXT, 0, (LPARAM) buf2);
	}
	if (uielement->pos) {
		sprintf(buf2, "%.2f", uielement->pos->topOffset);
	} else {
		strcpy(buf2, "(null)");
	}
	SendMessage(hUiProp.posY, WM_GETTEXT, sizeof(buf1), (LPARAM) buf1);
	if (strcmp(buf1, buf2)) {
		SendMessage(hUiProp.posY, WM_SETTEXT, 0, (LPARAM) buf2);
	}

	if (uielement->type == 2) {
		if (hUiProp.wasLblLangStrReadOnly) {
			SendMessage(hUiProp.lblLangStr, EM_SETREADONLY, 0, 0);
			hUiProp.wasLblLangStrReadOnly = 0;
		}
		SendMessage(hUiProp.lblLangStr, WM_GETTEXT, sizeof(buf1), (LPARAM) buf1);
		lbl = (struct UILabel*) uielement;
		if (lbl->textLanguageString != hatoi(buf1)) {
			sprintf(buf1, "%08X", ((struct UILabel*) uielement)->textLanguageString);
			SendMessage(hUiProp.lblLangStr, WM_SETTEXT, 0, (LPARAM) buf1);
		}
		if (lbl->string.ptrString) {
			SendMessage(hUiProp.lblStr, WM_GETTEXT, sizeof(buf3), (LPARAM) buf3);
			b = buf3;
			w = lbl->string.ptrString;
			while (*(b++) == *(w++)) {
				if (!*b) {
					goto ok;
				}
			}
			b = buf3;
			w = lbl->string.ptrString;
			while (b < (buf3 + sizeof(buf3)) && (*(b++) = *(w++)));
			if (hUiProp.wasLblStrReadOnly) {
				hUiProp.wasLblStrReadOnly = 0;
				SendMessage(hUiProp.lblStr, EM_SETREADONLY, 0, 0);
			}
			SendMessage(hUiProp.lblStr, WM_SETTEXT, 0, (LPARAM) buf3);
ok:
			;
		} else {
			if (!hUiProp.wasLblStrReadOnly) {
				hUiProp.wasLblStrReadOnly = 1;
				buf3[0] = 0;
				SendMessage(hUiProp.lblStr, WM_SETTEXT, 0, (LPARAM) buf3);
				SendMessage(hUiProp.lblStr, EM_SETREADONLY, 1, 0);
			}
		}
	} else {
		if (!hUiProp.wasLblLangStrReadOnly) {
			hUiProp.wasLblLangStrReadOnly = 1;
			buf1[0] = 0;
			SendMessage(hUiProp.lblLangStr, WM_SETTEXT, 0, (LPARAM) buf1);
			SendMessage(hUiProp.lblLangStr, EM_SETREADONLY, 1, 0);
		}
		if (!hUiProp.wasLblStrReadOnly) {
			hUiProp.wasLblStrReadOnly = 1;
			buf1[0] = 0;
			SendMessage(hUiProp.lblStr, WM_SETTEXT, 0, (LPARAM) buf1);
			SendMessage(hUiProp.lblStr, EM_SETREADONLY, 1, 0);
		}
	}
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
}

static
void dbgw_on_resize(HWND hwnd)
{
	int selectedTabIndex;
	RECT rc;
	int w, w2, h;

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
		w = rc.right - rc.left;
		h = rc.bottom - rc.top;
		if (w < UITREE_MINWIDTH) {
			MoveWindow(hUITree,
				rc.left, rc.top,
				w, h,
				1);
			MoveWindow(hUiProp.window,
				rc.left + 200, rc.top,
				0, h,
				0);
		} else {
			if (w < UITREE_MINWIDTH + UIPROPS_WIDTH) {
				w2 = w - UITREE_MINWIDTH;
			} else {
				w2 = UIPROPS_WIDTH;
			}
			MoveWindow(hUITree,
				rc.left, rc.top,
				w - w2, h,
				1);
			MoveWindow(hUiProp.window,
				rc.left + (w - w2), rc.top,
				w2, h,
				1);
		}
	}
}

static
void dbgw_create_tab_pchelpbar_controls(HWND hWnd)
{
	HWND hTmp;
	RECT rc;
	int w, w2, h, h2;
	int x, y;
	int i;
	char buf[64];

	x = PX_PADDING; y = PX_PADDING;
	w = 500 - PX_PADDING * 2; h = PX_SPACING_Y; h2 = PX_SPACING_Y / 2;

	hTmp =
	CreateWindowExA(0, "Static",
		"Buttons/flags:",
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		x,	y,
		w,	h,
		hWnd, (HMENU) IDC_LABL, hModule, 0);
	SendMessage(hTmp, WM_SETFONT, (WPARAM) font, 0);
	x += PX_INDENT;
	y += h;
	w2 = (w - PX_INDENT) / 2;
	for (i = 0; i < 32;) {
		sprintf(buf, "%x %s", 1 << i, pcHelpBarButtons[i]);
		hTmp =
		CreateWindowExA(0, "Button",
			buf,
			WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			x,	y,
			w2,	h,
			hWnd, (HMENU) (IDC_BTN_PCHELPBARMASK_FLAG1 + i), hModule, 0);
		SendMessage(hTmp, WM_SETFONT, (WPARAM) font, 0);
		hPcHelpBar.buttonBits[i] = hTmp;
		i++;
		if (i % 2) {
			x += w2;
		} else {
			x -= w2;
			y += h;
		}
	}
	x -= PX_INDENT;
	y += h2;
	hTmp =
	CreateWindowExA(0, "Button",
		"Hide",
		WS_CHILD | WS_VISIBLE,
		x, y,
		97, h,
		hWnd, (HMENU) IDC_BTN_PCHELPBAR_HIDE, hModule, 0);
	SendMessage(hTmp, WM_SETFONT, (WPARAM) font, 0);
	hTmp =
	CreateWindowExA(0, "Button",
		"Show",
		WS_CHILD | WS_VISIBLE,
		x + 103, y,
		97, h,
		hWnd, (HMENU) IDC_BTN_PCHELPBAR_SHOW, hModule, 0);
	SendMessage(hTmp, WM_SETFONT, (WPARAM) font, 0);
}

static
void dbgw_create_tab_ui_controls(HWND hWnd)
{
	HWND hTmp;
	RECT rc;
	int w, h, h2;
	int x, y;
	int i;
	char buf[32];

	hUITree =
	CreateWindowEx(0, WC_TREEVIEW,
		0,
		TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_DISABLEDRAGDROP
		| WS_CHILD | WS_VISIBLE | TVS_SHOWSELALWAYS,
		0, 0,
		0, 0,
		hWnd, (HMENU) IDC_UITREE, hModule, 0);

	hUiProp.window =
	CreateWindowExA(0, "nfsu2-re-dbgw-child-class",
		0,
		WS_CHILD,
		0, 0,
		0, 0,
		hWnd, 0, hModule, 0);

	x = PX_PADDING; y = PX_PADDING;
	w = (UIPROPS_WIDTH - PX_INDENT) / 2; h = PX_SPACING_Y; h2 = PX_SPACING_Y / 2;
	hUiProp.addr =
	CreateWindowExA(WS_EX_CLIENTEDGE , "Edit",
		"",
		WS_CHILD | WS_VISIBLE | ES_LEFT | ES_READONLY,
		x,	y,
		200,	h,
		hUiProp.window, (HMENU) IDC_EDIT_UIPROP_ADDR, hModule, 0);
	SendMessage(hUiProp.addr, WM_SETFONT, (WPARAM) font, 0);
	y += h + h2;

	hTmp =
	CreateWindowExA(0, "Button",
		"Hide hierarchy",
		WS_CHILD | WS_VISIBLE,
		x, y,
		97, h,
		hUiProp.window, (HMENU) IDC_BTN_UIPROP_HIDEALL, hModule, 0);
	SendMessage(hTmp, WM_SETFONT, (WPARAM) font, 0);
	hTmp =
	CreateWindowExA(0, "Button",
		"Show hierarchy",
		WS_CHILD | WS_VISIBLE,
		x + 103, y,
		97, h,
		hUiProp.window, (HMENU) IDC_BTN_UIPROP_SHOWALL, hModule, 0);
	SendMessage(hTmp, WM_SETFONT, (WPARAM) font, 0);

	y += h + h2;
	hTmp =
	CreateWindowExA(0, "Static",
		"Element flags:",
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		x,	y,
		w,	h,
		hUiProp.window, (HMENU) IDC_LABL_FLAGS, hModule, 0);
	SendMessage(hTmp, WM_SETFONT, (WPARAM) font, 0);
	x += PX_INDENT;
	y += h;
	for (i = 0; i < 32; i++) {
		sprintf(buf, "%d %s", i + 1, flagnames[i]);
		hTmp =
		CreateWindowExA(0, "Button",
			buf,
			WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
			x,	y,
			w,	h,
			hUiProp.window, (HMENU) (IDC_BTN_UIPROP_FLAG1 + i), hModule, 0);
		SendMessage(hTmp, WM_SETFONT, (WPARAM) font, 0);
		hUiProp.chk_flags[i] = hTmp;
		if (i % 2) {
			x -= w;
			y += h;
		} else {
			x += w;
		}
	}
	x -= PX_INDENT;
	y += h2;

	hTmp =
	CreateWindowExA(0, "Static",
		"Position:",
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		x,	y,
		w,	h,
		hUiProp.window, (HMENU) IDC_LABL, hModule, 0);
	SendMessage(hTmp, WM_SETFONT, (WPARAM) font, 0);

	y += h;
	x += PX_INDENT;
	w = (UIPROPS_WIDTH - PX_INDENT) / 2;
	hTmp =
	CreateWindowExA(0, "Static",
		"x:",
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		x,	y,
		w,	h,
		hUiProp.window, (HMENU) IDC_LABL, hModule, 0);
	SendMessage(hTmp, WM_SETFONT, (WPARAM) font, 0);
	hTmp =
	CreateWindowExA(0, "Static",
		"y:",
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		x + w,	y,
		w,	h,
		hUiProp.window, (HMENU) IDC_LABL, hModule, 0);
	SendMessage(hTmp, WM_SETFONT, (WPARAM) font, 0);
	hUiProp.posX =
	CreateWindowExA(WS_EX_CLIENTEDGE, "Edit",
		"",
		WS_CHILD | WS_VISIBLE | ES_LEFT,
		x + 20,	y,
		w - 40,	h,
		hUiProp.window, (HMENU) IDC_EDIT_UIPROP_X, hModule, 0);
	SendMessage(hUiProp.posX, WM_SETFONT, (WPARAM) font, 0);
	x += w;
	hUiProp.posY =
	CreateWindowExA(WS_EX_CLIENTEDGE , "Edit",
		"",
		WS_CHILD | WS_VISIBLE | ES_LEFT,
		x + 20,	y,
		w - 40,	h,
		hUiProp.window, (HMENU) IDC_EDIT_UIPROP_Y, hModule, 0);
	SendMessage(hUiProp.posY, WM_SETFONT, (WPARAM) font, 0);
	x -= w;
	x -= PX_INDENT;

	y += h + h2;

	UIPROPS_WIDTH - PX_INDENT - 20;
	hTmp =
	CreateWindowExA(0, "Static",
		"Label language string:",
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		x,	y,
		w,	h,
		hUiProp.window, (HMENU) IDC_LABL, hModule, 0);
	SendMessage(hTmp, WM_SETFONT, (WPARAM) font, 0);
	x += PX_INDENT;
	y += h;
	hUiProp.lblLangStr =
	CreateWindowExA(WS_EX_CLIENTEDGE , "Edit",
		"",
		WS_CHILD | WS_VISIBLE | ES_LEFT,
		x,	y,
		w - 40,	h,
		hUiProp.window, (HMENU) IDC_EDIT_UIPROP_LBLLANGSTR, hModule, 0);
	SendMessage(hUiProp.lblLangStr, WM_SETFONT, (WPARAM) font, 0);
	y += h;
	x -= PX_INDENT;

	hTmp =
	CreateWindowExA(0, "Static",
		"Label string:",
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		x,	y,
		w,	h,
		hUiProp.window, (HMENU) IDC_LABL, hModule, 0);
	SendMessage(hTmp, WM_SETFONT, (WPARAM) font, 0);
	x += PX_INDENT;
	y += h;
	w = UIPROPS_WIDTH - PX_PADDING * 2 - PX_INDENT;
	hUiProp.lblStr =
	CreateWindowExA(WS_EX_CLIENTEDGE , "Edit",
		"",
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT,
		x,	y,
		w,	h,
		hUiProp.window, (HMENU) IDC_EDIT_UIPROP_LBLSTR, hModule, 0);
	SendMessage(hUiProp.lblStr, WM_SETFONT, (WPARAM) font, 0);
	y += h;
	x -= PX_INDENT;
}

static
void dbgw_create_tab_d3_controls(HWND hWnd)
{
	int x, y, w, h;
	HWND hLabel;

	x = 10; y = 10;
	w = 200; h = PX_SPACING_Y;

	hLabel =
	CreateWindowExA(0, "Static",
		"Change render mode:",
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		x,	y,
		w,	h,
		hWnd, (HMENU) IDC_LABL, hModule, 0);
	x += PX_INDENT;
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
	ti.pszText = "pchelpbar";
	ti.cchTextMax = strlen(ti.pszText);
	SendMessage(hTab, TCM_INSERTITEM, 3, (LPARAM) &ti);

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

	dbgw_create_tab_pchelpbar_controls(hTabpane[2]);
	dbgw_create_tab_d3_controls(hTabpane[1]);
	dbgw_create_tab_ui_controls(hTabpane[0]);

	dbgw_on_resize(hMain);
}

static
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int idc;

	switch (msg)
	{
	case WM_COMMAND:
		// https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3drenderstatetype
		// https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dfillmode
		// https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dshademode
		idc = LOWORD(wParam);
		switch (idc) {
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
		case IDC_EDIT_UIPROP_X:
		{
			struct UIElement *element;
			char buf[32];

			if (HIWORD(wParam) == EN_CHANGE) {
				element = dbgw_get_selected_uielement();
				if (element) {
					SendMessage(hUiProp.posX, WM_GETTEXT, sizeof(buf), (LPARAM) buf);
					element->pos->leftOffset = atof(buf);
					element->someFlags |= UIELEMENT_FLAG_NEED_UPDATE_0;
				}
			}
			break;
		}
		case IDC_EDIT_UIPROP_Y:
		{
			struct UIElement *element;
			char buf[32];

			if (HIWORD(wParam) == EN_CHANGE) {
				element = dbgw_get_selected_uielement();
				if (element) {
					SendMessage(hUiProp.posY, WM_GETTEXT, sizeof(buf), (LPARAM) buf);
					element->pos->topOffset = atof(buf);
					element->someFlags |= UIELEMENT_FLAG_NEED_UPDATE_0;
				}
			}
			break;
		}
		case IDC_EDIT_UIPROP_LBLLANGSTR:
		{
			struct UIElement *element;
			char buf[32];

			if (HIWORD(wParam) == EN_CHANGE) {
				element = dbgw_get_selected_uielement();
				if (element && element->type == 2) {
					SendMessage(hUiProp.lblLangStr, WM_GETTEXT, sizeof(buf), (LPARAM) buf);
					((struct UILabel*) element)->textLanguageString = hatoi(buf);
					element->someFlags |= UIELEMENT_FLAG_NEED_UPDATE_0;
				}
			}
			break;
		}
		case IDC_EDIT_UIPROP_LBLSTR:
		{
			struct UIElement *element;
			char buf[DBGW_UI_LABEL_STRING_MAX_LEN];

			if (HIWORD(wParam) == EN_CHANGE) {
				element = dbgw_get_selected_uielement();
				if (element && element->type == 2) {
					SendMessage(hUiProp.lblStr, WM_GETTEXT, sizeof(buf), (LPARAM) buf);
					UILabel__setString((void*) element, buf); // sets update flag
				}
			}
			break;
		}
		case IDC_BTN_UIPROP_HIDEALL:
		{
			struct UIElement *uielement;

			uielement = dbgw_get_selected_uielement();
			if (uielement) {
				((void (__cdecl*)(struct UIElement*))0x50CA00)(uielement);
			}
			break;
		}
		case IDC_BTN_UIPROP_SHOWALL:
		{
			struct UIElement *uielement;

			uielement = dbgw_get_selected_uielement();
			if (uielement) {
				((void (__cdecl*)(struct UIElement*))0x50CA50)(uielement);
			}
			break;
		}
		case IDC_BTN_PCHELPBAR_HIDE:
			PCHelpBarFNGObject__Hide();
			break;
		case IDC_BTN_PCHELPBAR_SHOW:
			PCHelpBarFNGObject__Show();
			break;
		default:
		{
			struct UIElement *uielement;
			HWND hFlag;
			int idx;
			unsigned int mask;
			char *oldOwner;

			if (IDC_BTN_UIPROP_FLAG1 <= idc && idc <= IDC_BTN_UIPROP_FLAG32) {
				idx = idc - IDC_BTN_UIPROP_FLAG1;
				hFlag = hUiProp.chk_flags[idx];
				uielement = dbgw_get_selected_uielement();
				if (uielement) {
					if (SendMessage(hFlag, BM_GETCHECK, 0, 0) == BST_CHECKED) {
						uielement->someFlags |= 1 << idx;
						hUiProp.lastFlags |= 1 << idx;
					} else {
						uielement->someFlags &= ~(1 << idx);
						hUiProp.lastFlags &= ~(1 << idx);
					}
					if (idx == 1) {
						// toggling 'use custom text', requires update
						uielement->someFlags |= UIELEMENT_FLAG_NEED_UPDATE_0;
					}
				}
			} else if (IDC_BTN_PCHELPBARMASK_FLAG1 <= idc && idc <= IDC_BTN_PCHELPBARMASK_FLAG32) {
				idx = idc - IDC_BTN_PCHELPBARMASK_FLAG1;
				hFlag = hPcHelpBar.buttonBits[idx];
				if (SendMessage(hFlag, BM_GETCHECK, 0, 0) == BST_CHECKED) {
					hPcHelpBar.lastButtonMask |= 1 << idx;
				} else {
					hPcHelpBar.lastButtonMask &= ~(1 << idx);
				}
				mask = 0;
				for (idx = 0; idx < 32; idx++) {
					if (SendMessage(hPcHelpBar.buttonBits[idx], BM_GETCHECK, 0, 0) == BST_CHECKED) {
						mask |= (1 << idx);
					}
				}
				oldOwner = pcHelpbar[0]->oldButtonOwnerFngName;
				PCHelpBarFNGObject__SyncByMask(mask, pcHelpbar[0]->buttonOwnerFngName);
				pcHelpbar[0]->oldButtonOwnerFngName = oldOwner;
			}
			break;
		}
		}
		break;
	case WM_NOTIFY:
	{
		struct UIElement *element;
		RECT rc;
		NMHDR *nmhdr = (void*) lParam;
		char buf[32];
		int sel;
		int i;

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
		} if (nmhdr->hwndFrom == hUITree && nmhdr->code == TVN_SELCHANGED) {
			element = dbgw_get_selected_uielement();
			if (element) {
				sprintf(buf, "Element@%08X", element);
				SendMessage(hUiProp.addr, WM_SETTEXT, 0, (LPARAM) buf);
				if (!(GetWindowLong(hUiProp.window, GWL_STYLE) & WS_VISIBLE)) {
					ShowWindow(hUiProp.window, SW_SHOW);
					UpdateWindow(hUiProp.window);
				}
			} else {
				buf[0] = 0;
				SendMessage(hUiProp.addr, WM_SETTEXT, 0, (LPARAM) buf);
				ShowWindow(hUiProp.window, SW_HIDE);
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
		dbgw_on_resize(hwnd);
		break;
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
		CW_USEDEFAULT, CW_USEDEFAULT, 500, 650,
		NULL, NULL, hModule, NULL
	);
	if (hMain == NULL) {
		ERRMSG(NULL, "Window creation failed.");
		return;
	}

	ShowWindow(hMain, SW_SHOW);
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
void dbgw_before_present()
{
	PRESENT_HOOK_FUNC();
#undef PRESENT_HOOK_FUNC
#define PRESENT_HOOK_FUNC dbgw_before_present

	if (hMain) {
		dbgw_ui_tree_update_before_present();
		dbgw_ui_props_update_before_present();
		dbgw_pchelpbar_update_before_present();
	}
}

static
void initDbgw()
{
	mkjmp(0x48CE08, CreateWindowHook);

	INIT_FUNC();
#undef INIT_FUNC
#define INIT_FUNC initDbgw
}
