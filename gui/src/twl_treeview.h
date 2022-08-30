#pragma once
#include <string>

class TTreeView : public TNotifyWin {
protected:
	SelectionHandler   m_on_select;
	SelectionHandler m_on_selection_changing;
	TWin*	   m_form;
	bool m_has_images;
public:

	void on_select(SelectionHandler handler)
	{
		m_on_select = handler;
	}

	void on_selection_changing(SelectionHandler handler)
	{
		m_on_selection_changing = handler;
	}

	TTreeView(TWin* form, bool has_lines = true, bool editable = false);
	Handle add_item(pchar caption, Handle parent = NULL, int idx1 = 0, int idx2 = -1, Handle data = 0);
	TEventWindow* get_parent_win() { return (TEventWindow*)m_form; };

	Handle get_data(Handle pn);
	void set_image_list(TImageList* il, bool normal = true);

	void select(Handle p);
	Handle get_selected();
	void clear();
	std::string get_itm_text(Handle);
	void set_itm_text(void* itm, pchar str);
	int handle_notify(void* p) override;
	virtual int handle_rclick(Handle) = 0;
	virtual void handle_select(Handle) = 0;
	
};
