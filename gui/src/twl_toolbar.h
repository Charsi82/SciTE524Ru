#pragma once
#include "twl.h"
#include "twl_menu.h"

class TToolbar : public TWin {
    MessageHandler* m_menu_handler;
    TEventWindow* m_form;
    TEventWindow* m_container;
    TCHAR m_path[MAX_PATH];
    int m_bwidth, m_bheight;

public:
    TToolbar(TEventWindow* form, int bwidth = 16, int bheight = 16, TEventWindow* m_container = NULL);
    ~TToolbar();
    void create();
    void set_path(pchar path);
    void add_item(pchar bmp, pchar tooltext, EventHandler eh, void* data = NULL);
    SIZE get_size();
    void release();
};

TToolbar& operator<< (TToolbar& tb, Item item);
TToolbar& operator<< (TToolbar& tb, Sep sep);
