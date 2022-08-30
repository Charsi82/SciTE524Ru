// twl_toolbar.cpp
// implements the following common controls
//   TToolBar
//   TImageList
//   TTabControl
//   TListView
//   TTreeView
#include <windows.h>
#include <commctrl.h>
#define IS_IMPLEMENTATION
#include "twl.h"
#include "twl_menu.h"
#include "twl_imagelist.h"
#include "twl_listview.h"
#include "twl_toolbar.h"
#include "twl_tab.h"
#include "twl_treeview.h"
#include <stdio.h>
#include <io.h>
#include "utf.h"

void* ApplicationInstance();

static size_t gID = 445560;

static HWND create_common_control(TWin* form, pchar winclass, int style, int height = -1)
{
	int w = CW_USEDEFAULT, h = CW_USEDEFAULT;
	if (height != -1) { w = 100; h = height; }
	return CreateWindowEx(0L,   // No extended styles.
		winclass, L"", WS_CHILD | style,
		0, 0, w, h,
		(HWND)form->handle(),                  // Parent window of the control.
		(HMENU)(void*)gID++,
		(HINSTANCE)ApplicationInstance(),             // Current instance.
		NULL);
}

static HICON load_icon(pchar file)
{
	return (HICON)LoadImage((HINSTANCE)ApplicationInstance(), file, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
}

static HBITMAP load_bitmap(pchar file)
{
	HBITMAP res = (HBITMAP)LoadImage(0/*ApplicationInstance()*/, file, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
	if (!res) {
		int _res = _waccess(file, 0);
		int err = GetLastError();
		err = err - 1;
	}
	return res;
}

///// TImageList class

static int icon_size(bool s)
{
	return s ? 16 : 32;
}

TImageList::TImageList(int cx, int cy)
{
	create(cx, cy);
	m_small_icons = cx == icon_size(true);
}

TImageList::TImageList(bool s /*= true*/)
	: m_small_icons(s)
{
	int cx = icon_size(s);
	int cy = cx;
	create(cx, cy);
}

void TImageList::create(int cx, int cy)
{
	m_handle = ImageList_Create(cx, cy, ILC_COLOR32 | ILC_MASK, 0, 32);
}

int TImageList::add_icon(pchar iconfile)
{
	HICON hIcon = load_icon(iconfile);
	if (!hIcon) return -1;  // can't find icon
	return ImageList_AddIcon(m_handle, hIcon);
}

int TImageList::add(pchar bitmapfile, long mask_clr)
{
	HBITMAP hBitmap = load_bitmap(bitmapfile);
	if (!hBitmap) return -1;  // can't find bitmap
	if (mask_clr != 1)
		return ImageList_AddMasked(m_handle, hBitmap, mask_clr);
	else
		return ImageList_Add(m_handle, hBitmap, NULL);
}

int TImageList::load_icons_from_module(pchar mod)
{
	HINSTANCE hInst = GetModuleHandle(mod);
	HICON hIcon;
	int cx = icon_size(m_small_icons);
	int cy = cx;
	int i = 1;
	while (hIcon = (HICON)LoadImage(hInst, (pchar)(i++), IMAGE_ICON, cx, cy, LR_LOADMAP3DCOLORS))
		ImageList_AddIcon(m_handle, hIcon);
	return i;
}

void TImageList::set_back_colour(long clrRef)
{
	ImageList_SetBkColor(m_handle, clrRef);
}

void TImageList::load_shell_icons()
{
	load_icons_from_module(L"shell32.dll");
	set_back_colour(CLR_NONE);
}

////// TListView

TListViewB::TListViewB(TWin* form, bool large_icons, bool multiple_columns, bool single_select)
{
	int style = WS_CHILD;
	if (large_icons) {
		style |= (LVS_ICON | LVS_AUTOARRANGE);
	}
	else {
		style |= LVS_REPORT;
		if (single_select) {
			style |= LVS_SINGLESEL;
		}
		if (!multiple_columns) {
			style |= LVS_NOCOLUMNHEADER;
			//add_column("*",1000);
		}
	}

	style |= LVS_SHOWSELALWAYS;

	// Create the list view control.
	set(create_common_control(form, WC_LISTVIEW, style));
	parent_win = form;
	m_custom_paint = false;
	m_has_images = false;
	m_last_col = 0;
	m_last_row = -1;
	m_bg = 0;
	m_fg = 0;
	send_msg(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT); // Set style
}

void TListViewB::set_image_list(TImageList* il_small, TImageList* il_large)
{
	if (il_small) send_msg(LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)il_small->handle());
	if (il_large) send_msg(LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)il_large->handle());
	m_has_images = true;
}

void TListViewB::add_column(pchar label, int width)
{
	LVCOLUMN lvc{};
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;   // left-align, by default
	lvc.cx = width;
	lvc.pszText = (wchar_t*)label;
	lvc.iSubItem = m_last_col;

	ListView_InsertColumn((HWND)m_hwnd, m_last_col, &lvc);
	m_last_col++;
}

void TListViewB::set_foreground(unsigned int colour)
{
	send_msg(LVM_SETTEXTCOLOR, 0, (LPARAM)colour);
	m_fg = colour;
}

void TListViewB::set_background(unsigned int colour)
{
	send_msg(LVM_SETBKCOLOR, 0, (LPARAM)colour);
	m_bg = colour;
	m_custom_paint = true;
}


int TListViewB::columns()
{
	return m_last_col;
}

void TListViewB::autosize_column(int col, bool by_contents)
{
	ListView_SetColumnWidth((HWND)m_hwnd, col, by_contents ? LVSCW_AUTOSIZE : LVSCW_AUTOSIZE_USEHEADER);
}

void TListViewB::start_items()
{
	m_last_row = -1;
}

int TListViewB::add_item_at(int i, pchar text, int idx, void* data)
{
	LVITEM lvi{};
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
	if (m_has_images) lvi.mask |= LVIF_IMAGE;
	lvi.state = 0;
	lvi.stateMask = 0;
	lvi.pszText = (wchar_t*)text;
	lvi.lParam = (LPARAM)data;
	lvi.iItem = i;
	lvi.iImage = idx;                // image list index
	lvi.iSubItem = 0;

	ListView_InsertItem((HWND)m_hwnd, &lvi);
	return i;
}

int TListViewB::add_item(pchar text, int idx, void* data)
{
	m_last_row++;
	return add_item_at(m_last_row, text, idx, data);
}

void TListViewB::add_subitem(int i, pchar text, int idx)
{
	ListView_SetItemText((HWND)m_hwnd, i, idx, (wchar_t*)text);
}

void TListViewB::delete_item(int i)
{
	ListView_DeleteItem((HWND)m_hwnd, i);
}

void TListViewB::select_item(int i)
{
	ListView_SetItemState((HWND)m_hwnd, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	ListView_EnsureVisible((HWND)m_hwnd, i, true);
}

void TListViewB::get_item_text(int i, wchar_t* buff, int buffsize)
{
	ListView_GetItemText((HWND)m_hwnd, i, 0, buff, buffsize);
}

void* TListViewB::get_item_data(int i)
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iItem = i;
	lvi.iSubItem = 0;
	ListView_GetItem((HWND)m_hwnd, &lvi);
	return (void*)lvi.lParam;
}

int TListViewB::selected_id()
{
	return (int)send_msg(LVM_GETNEXTITEM, (WPARAM)(-1), LVNI_FOCUSED);
}

int TListViewB::next_selected_id(int i)
{
	return (int)send_msg(LVM_GETNEXTITEM, i, LVNI_SELECTED);
}

int TListViewB::count()
{
	return (int)send_msg(LVM_GETITEMCOUNT);
}

int TListViewB::selected_count()
{
	return (int)send_msg(LVM_GETSELECTEDCOUNT);
}

void TListViewB::clear()
{
	send_msg(LVM_DELETEALLITEMS);
	m_last_row = -1;
}

static int list_custom_draw(void* lParam, COLORREF fg, COLORREF bg)
{
	LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)lParam;

	if (lplvcd->nmcd.dwDrawStage == CDDS_PREPAINT)
		// Request prepaint notifications for each item.
		return CDRF_NOTIFYITEMDRAW;

	if (lplvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
		lplvcd->clrText = fg;
		lplvcd->clrTextBk = bg;
		return CDRF_NEWFONT;
	}
	return 0;
}

int TListViewB::handle_notify(void* lparam)
{
	LPNMHDR np = (LPNMHDR)lparam;
	int id = selected_id();
	switch (np->code) {
	case LVN_ITEMCHANGED:
		handle_select(id);
		return 1;
	case NM_DBLCLK:
	{
		LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lparam;
		LVHITTESTINFO pInfo;
		pInfo.pt = lpnmitem->ptAction;
		ListView_SubItemHitTest((HWND)handle(), &pInfo);

		int i = pInfo.iItem;
		int j = pInfo.iSubItem;
		wchar_t buffer[10]{};
		LVITEM item;
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.iItem = i;
		item.iSubItem = j;
		item.cchTextMax = sizeof(buffer) / sizeof(buffer[0]);
		item.pszText = buffer;
		ListView_GetItem((HWND)handle(), &item);
		if (i > -1)
			handle_double_click(i, j, UTF8FromString(std::wstring(buffer)).c_str());
		return 1;
	}
	case LVN_KEYDOWN:
		handle_onkey(((LPNMLVKEYDOWN)lparam)->wVKey);
		return 0;  // ignored, anyway
	case NM_RCLICK:
		//send_msg(WM_CHAR,VK_ESCAPE,0);
		return handle_rclick(id);
	case NM_SETFOCUS:
		handle_onfocus(true);
		return 1;
	case NM_KILLFOCUS:
		handle_onfocus(false);
		return 1;
	case NM_CUSTOMDRAW:
		if (m_custom_paint) {
			return list_custom_draw(lparam, m_fg, m_bg);
		}
		return 0;
	}
	return 0;
}

TListView::TListView(TEventWindow* form, bool large_icons, bool multiple_columns, bool single_select)
	: TListViewB(form, large_icons, multiple_columns, single_select),
	m_form(form), m_on_select(NULL), m_on_double_click(NULL), m_on_key(NULL)
{
}

void TListView::handle_select(intptr_t i)
{
	if (m_on_select) {
		(m_form->*m_on_select)(i);
	}
}

void TListView::handle_double_click(int row, int col, const char* s)
{
	if (m_on_double_click) {
		(m_form->*m_on_double_click)(row, col, s);
	}
}

void TListView::handle_onkey(int i)
{
	if (m_on_key) {
		(m_form->*m_on_key)(i);
	}
}

TTabControlB::TTabControlB(TWin* form, bool multiline)
{
	// Create the tab control.
	int style = WS_CHILD; // | TCS_TOOLTIPS;
	if (multiline) style |= TCS_MULTILINE;
	set(create_common_control(form, WC_TABCONTROL, style | TCS_TOOLTIPS | TCS_TABS, 25));
	send_msg(WM_SETFONT, (WPARAM)::GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);

	m_index = 0;
}

void TTabControlB::add(pchar caption, void* data, int image_idx /*= -1*/)
{
	TCITEM item{};
	item.mask = TCIF_TEXT | TCIF_PARAM;
	item.pszText = (wchar_t*)caption;
	item.lParam = (LPARAM)data;
	send_msg(TCM_INSERTITEM, m_index++, (LPARAM)&item);
}

void* TTabControlB::get_data(int idx)
{
	if (idx == -1) idx = selected();
	TCITEM item{};
	item.mask = TCIF_PARAM;
	send_msg(TCM_GETITEM, idx, (LPARAM)&item);
	return (void*)item.lParam;
}

void TTabControlB::remove(int idx /*= -1*/)
{
	send_msg(idx > -1 ? TCM_DELETEITEM : TCM_DELETEALLITEMS, idx);
}

void TTabControlB::selected(int idx)
{
	send_msg(TCM_SETCURSEL, idx);
	NMHDR nmh;
	nmh.code = TCN_SELCHANGE;
	nmh.idFrom = GetDlgCtrlID((HWND)handle());
	nmh.hwndFrom = (HWND)handle();
	SendMessage(GetParent((HWND)handle()), WM_NOTIFY, nmh.idFrom, (LPARAM)&nmh);
}

int TTabControlB::selected()
{
	return (int)send_msg(TCM_GETCURSEL);
}

int TTabControlB::handle_notify(void* p)
{
	LPNMHDR np = (LPNMHDR)p;
	int id = selected();
	switch (np->code) {
	case TCN_SELCHANGE:
		handle_select(id);
		return 1;
	case NM_RCLICK:
		return handle_rclick(id);
	case TTN_NEEDTEXT:
		LPNMTTDISPINFO ttn = (LPNMTTDISPINFO)p;
		static TCHAR buf[256]{};
		TCITEM item{};
		item.mask = TCIF_TEXT;
		item.pszText = buf;
		item.cchTextMax = 256;
		send_msg(TCM_GETITEM, np->idFrom, (LPARAM)&item);
		lstrcpy(buf, item.pszText);
		ttn->lpszText = buf;
		return 1;
	}
	return 0;
}

TTabControl::TTabControl(TEventWindow* parent, bool multiline /*= false*/)
	: TTabControlB(parent, multiline), form(parent)
{}

TTreeView::TTreeView(TWin* form, bool has_lines, bool editable)
	: m_form(form), m_on_select(NULL), m_on_selection_changing(NULL), m_has_images(false)
{
	int style = TVS_HASBUTTONS;
	if (has_lines) style |= (TVS_HASLINES | TVS_LINESATROOT);
	if (editable) style |= TVS_EDITLABELS;
	set(create_common_control(form, WC_TREEVIEW, style));
}

void TTreeView::set_image_list(TImageList* il, bool normal)
{
	send_msg(TVM_SETIMAGELIST, normal ? TVSIL_NORMAL : TVSIL_STATE, (LPARAM)il->handle());
	m_has_images = true;
}

void TTreeView::clear()
{
	TreeView_DeleteAllItems((HWND)handle());
	//send_msg(TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);
}

Handle TTreeView::add_item(pchar caption, Handle parent, int idx1, int idx2, Handle data)
{
	TVITEM item{};
	item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
	if (m_has_images) {
		item.mask |= (TVIF_IMAGE | TVIF_SELECTEDIMAGE);
		if (idx2 == -1) idx2 = idx1;
	}

	static HTREEITEM hPrev = (HTREEITEM)TVI_LAST;
	static HTREEITEM hPrevRootItem = NULL;
	static HTREEITEM hPrevLev2Item = NULL;

	item.pszText = (wchar_t*)caption;
	item.cchTextMax = lstrlen(caption);
	item.iImage = idx1;
	item.iSelectedImage = idx2;
	item.lParam = (LPARAM)TVI_LAST; //data;
	item.cChildren = 0;// has_children ? 1 : 0;

	TVINSERTSTRUCT tvsi{};
	tvsi.item = item;
	tvsi.hInsertAfter = (HTREEITEM)parent; // hPrev/*TVI_LAST*/;
	//tvsi.hParent = (HTREEITEM)parent;

	//if (nLevel == 1)
	//	tvsi.hParent = TVI_ROOT;
	//else if (nLevel == 2)
	//	tvsi.hParent = hPrevRootItem;
	//else
	//	tvsi.hParent = hPrevLev2Item;

	if (parent)
	{
		tvsi.hParent = (HTREEITEM)parent;

		// set mask parent 
		TVITEM item_p{};
		item_p.mask = TVIF_CHILDREN;
		item_p.hItem = (HTREEITEM)parent;
		item_p.cChildren = 1;
		TreeView_SetItem((HWND)handle(), &item_p);
	}
	else
	{
		tvsi.hParent = TVI_ROOT;
	}

	hPrev = (HTREEITEM)send_msg(TVM_INSERTITEM, 0, (LPARAM)&tvsi);
	if (hPrev == NULL)
		return NULL;

	// Save the handle to the item. 
	//if (nLevel == 1)
	//	hPrevRootItem = hPrev;
	//else if (nLevel == 2)
	//	hPrevLev2Item = hPrev;

	return (Handle)hPrev;
}

void* TTreeView::get_data(Handle pn)
{
	//if (pn == NULL) pn = selected();
	TVITEM item{};
	item.mask = TVIF_PARAM | TVIF_HANDLE;
	item.hItem = (HTREEITEM)pn;
	send_msg(TVM_GETITEM, 0, (LPARAM)&item);
	return (void*)item.lParam;
}

void TTreeView::select(Handle p)
{
	send_msg(TVM_SELECTITEM, TVGN_CARET, (LPARAM)p);
}

Handle TTreeView::get_selected()
{
	return TreeView_GetSelection((HWND)handle());
}

void TTreeView::set_itm_text(void* itm, pchar str) {
	TVITEM tvi{};
	tvi.pszText = (LPWSTR)str;
	tvi.cchTextMax = 256;
	tvi.mask = TVIF_TEXT;
	tvi.hItem = (HTREEITEM)itm;
	send_msg(TVM_SETITEM, 0, (LPARAM)&tvi);
}

std::string TTreeView::get_itm_text(Handle itm) {
	TCHAR buffer[256]{};
	TVITEM tvi{};
	tvi.pszText = buffer;
	tvi.cchTextMax = 256;
	tvi.mask = TVIF_TEXT;
	tvi.hItem = (HTREEITEM)itm;
	send_msg(TVM_GETITEM, 0, (LPARAM)&tvi);
	return UTF8FromString(std::wstring(buffer));
}

int TTreeView::handle_notify(void* p)
{
	NMTREEVIEW* np = (NMTREEVIEW*)p;
	Handle item = np->itemNew.hItem;
	switch (np->hdr.code) {
	case NM_RCLICK:
	{
		return handle_rclick(get_selected());
		break;
	}
	case TVN_SELCHANGED:
	{
		//if (GetActiveWindow() == (HWND)handle()) return 0;
		handle_select(get_selected());
		break;
	}
	//case TVN_SELCHANGING:
		//if (GetActiveWindow() == (HWND)handle()) return 0;
		//handle_selection_changing((intptr_t)item);
	}
	return 0;
}

////////////////// toolbar /////////////

TToolbar::TToolbar(TEventWindow* form, int bwidth, int bheight, TEventWindow* c)
	: m_form(form), m_container(c), m_bwidth(bwidth), m_bheight(bheight)
{
	m_menu_handler = new MessageHandler(form);
	if (m_container == NULL)
		m_container = form;
	create();
}

TToolbar::~TToolbar()
{
	delete m_menu_handler;
	if (m_container != NULL)
		release();
}

void TToolbar::create()
{
	int style = WS_BORDER | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT;
	set(create_common_control(m_container, TOOLBARCLASSNAME, style, 150));

	send_msg(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON));

	// add the standard bitmaps
	TBADDBITMAP std_bitmaps{};
	std_bitmaps.hInst = HINST_COMMCTRL;
	std_bitmaps.nID = IDB_STD_SMALL_COLOR;
	int idx = send_msg(TB_ADDBITMAP, 1, (intptr_t)&std_bitmaps);

	send_msg(TB_SETBITMAPSIZE, 0, MAKELONG(m_bwidth, m_bheight));

}

void TToolbar::set_path(pchar path)
{
	lstrcpy(m_path, path);
}

SIZE TToolbar::get_size()
{
	SIZE sz;
	send_msg(TB_GETMAXSIZE, 0, (LPARAM)&sz);
	return sz;
}

class TToolBarToolTipNotifyHandler : public TNotifyWin {
	TWin* m_tb;
public:
	TToolBarToolTipNotifyHandler(TWin* tb, Handle tth) : m_tb(tb)
	{
		set(tth);
	}

	int handle_notify(void* p)
	{
		LPTOOLTIPTEXT lpToolTipText = (LPTOOLTIPTEXT)p;
		if (lpToolTipText->hdr.code == TTN_GETDISPINFO) {
			TBBUTTON btn;
			int id = lpToolTipText->hdr.idFrom;                   // command id
			int index = m_tb->send_msg(TB_COMMANDTOINDEX, id);    // index in the buttons
			m_tb->send_msg(TB_GETBUTTON, index, (LPARAM)&btn);    // get the button
			// and pick up the text we left there
			lpToolTipText->hinst = NULL; // ??
			lpToolTipText->lpszText = lpToolTipText->szText; // ??
			if (!btn.dwData) return 0;
			lstrcpy(lpToolTipText->szText, (wchar_t*)btn.dwData);
			return 1;
		}
		return 0;
	}
};

void TToolbar::release()
{
	if (!m_container) return;
	send_msg(TB_AUTOSIZE);
	HWND hToolTip = (HWND)send_msg(TB_GETTOOLTIPS);
	TWin* tb = new TWin(handle());
	m_container->set_toolbar(tb, new TToolBarToolTipNotifyHandler(tb, hToolTip));
	m_container->add_handler(m_menu_handler);
	m_container = NULL;
}


TToolbar& operator<< (TToolbar& tb, Item item)
{
	//wchar_t* caption = _wcsdup((wchar_t*)item.caption); // hack
	//wchar_t* bmp_file = wcstok(caption, L":");
	//wchar_t* tooltip_text = wcstok(NULL, L"");

	gui_string capt(item.caption);
	size_t pos = capt.find_first_of(L":");
	gui_string bmp_file = capt.substr(0, pos);
	gui_string tooltip_text = capt.substr(pos + 1); // capt.c_str() + pos + 1;
	tb.add_item(bmp_file.c_str(), tooltip_text.c_str(), item.handler, item.data);
	return tb;
}

TToolbar& operator<< (TToolbar& tb, Sep sep)
{
	tb.add_item(NULL, NULL, NULL);
	return tb;
}

struct StdItem {
	const wchar_t* name;
	int idx;
};

static StdItem std_items[] = {
	{L"COPY",STD_COPY},
	{L"CUT",STD_CUT},
	{L"DELETE",STD_DELETE},
	{L"FILENEW",STD_FILENEW},
	{L"FILEOPEN",STD_FILEOPEN},
	{L"FILESAVE",STD_FILESAVE},
	{L"FIND",STD_FIND},
	{L"HELP",STD_HELP},
	{L"PASTE",STD_PASTE},
	{L"PRINT",STD_PRINT},
	{L"PRINTPRE",STD_PRINTPRE},
	{L"PROPERTIES",STD_PROPERTIES},
	{L"REDOW",STD_REDOW},
	{L"REPLACE",STD_REPLACE},
	{L"UNDO",STD_UNDO},
	{L"_DETAILS",VIEW_DETAILS},
	{L"_LARGEICONS",VIEW_LARGEICONS},
	{L"_LIST",VIEW_LIST},
	{L"_SMALLICONS",VIEW_SMALLICONS},
	{L"_SORTDATE",VIEW_SORTDATE},
	{L"_SORTNAME",VIEW_SORTNAME},
	{L"_SORTSIZE",VIEW_SORTSIZE},
	{L"_SORTTYPE",VIEW_SORTTYPE},
	{NULL,-1}
};

void TToolbar::add_item(pchar bmp, pchar tooltext, EventHandler eh, void* data)
{
	int idx = 0;
	if (bmp) {
		int std_id = -1;
		StdItem* pi = std_items;
		while (pi->name != NULL) {
			if (wcscmp(pi->name, bmp) == 0) break;
			pi++;
		}
		idx = pi->idx;
		if (idx == -1) { // wasn't a standard toolbar button
			wchar_t bmpfile[256];
			if (m_path != NULL) {
				swprintf_s(bmpfile, L"%s\\%s", m_path, bmp);
			}
			else {
				lstrcpy(bmpfile, bmp);
			}
			int sz = lstrlen(bmpfile);
			if (wcsncmp(bmpfile + sz - 4, L".bmp", 4) != 0) {
				lstrcat(bmpfile, L".bmp");
			}
			HANDLE hBitmap = load_bitmap(bmpfile);
			TBADDBITMAP bitmap;
			bitmap.hInst = NULL; // i.e we're passing a bitmap handle
			bitmap.nID = (UINT_PTR)hBitmap;
			idx = send_msg(TB_ADDBITMAP, 1, (LPARAM)&bitmap);
		}
	}
	Item item(0, eh, data, -1);
	if (bmp)
		m_menu_handler->add(item);

	TBBUTTON btn;
	btn.iBitmap = idx;
	btn.idCommand = bmp ? item.id : 0;
	btn.fsStyle = bmp ? TBSTYLE_BUTTON : TBSTYLE_SEP;
	btn.fsState = TBSTATE_ENABLED;
	btn.dwData = (DWORD_PTR)tooltext;
	btn.iString = 0;
	int ret = send_msg(TB_ADDBUTTONS, 1, (LPARAM)&btn);
}