#pragma once
typedef int (TEventWindow::* NotifyEventHandler)(intptr_t id);
typedef NotifyEventHandler NEH;

class TTabControlB : public TNotifyWin {
	int m_index;
public:
	TTabControlB(TWin* form, bool multiline = false);
	void add(pchar caption, void* data, int image_idx = -1);
	void remove(int idx = -1);
	void* get_data(int idx = -1);
	void selected(int idx);
	int selected();

	virtual void handle_select(intptr_t id) = 0;
	virtual int handle_rclick(int id) = 0;
	// override
	int handle_notify(void* p);
};

class TTabControl : public TTabControlB {
protected:
	TEventWindow* form;
public:
	TTabControl(TEventWindow* parent, bool multiline = false);
};
