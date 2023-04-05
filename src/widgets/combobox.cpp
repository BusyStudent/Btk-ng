#include "build.hpp"

#include <Btk/widgets/combobox.hpp>
#include <Btk/event.hpp>
#include <algorithm>

BTK_NS_BEGIN

ComboBox::ComboBox(Widget *parent) : Widget(parent) {
	// 目前不提供其他view类支持，只委托给listbox。
	_listbox_view = new ListBox();
	_listbox_view->signal_item_clicked().connect(&ComboBox::set_current_item, this);
	set_focus_policy(FocusPolicy::Mouse);
	_text_layout.set_font(font());
	_text_layout.set_text(" ");
	blank = _text_layout.size().w;
}

ComboBox::~ComboBox() {
	delete _listbox_view;
	delete _popup_widget;
}

int ComboBox::max_visible_items() const {
	return _max_visible_items;
}

void ComboBox::set_max_visible_items(int max_items) {
	_max_visible_items = max_items;
}

int ComboBox::count() const {
	return _count;
}

int ComboBox::find_text(u8string_view text) {
	int index = -1;
	for (int i = 0;i < _count; ++i) {
		if (_listbox_view->item_at(i)->text == text) {
			index = i;
			break;
		}
	}
	return index;
}

int ComboBox::current_index() const {
	return _current_index;
}

u8string ComboBox::current_text() const {
	if (_current_index < 0 || _current_index >= _count) {
		return "";
	}
	return _listbox_view->item_at(_current_index)->text;
}

bool ComboBox::set_item_text(int index, u8string_view text) {
	if (index < 0 || _current_index >= _count) {
		return false;
	} else {
		auto item = _listbox_view->item_at(index);
		item->text = text;
		_listbox_view->update_item(item);
		_text_changed.emit(index, text);
	}
	return true;
}

void ComboBox::add_item(u8string_view str) {
	ListItem list_item;
	list_item.text = str;
	_listbox_view->add_item(list_item);
	++ _count;
	if (_count == 1) {
		set_current_index(0);
	}
}

void ComboBox::add_item(const PixBuffer& icon) {
	ListItem list_item;
	list_item.image = icon;
	_listbox_view->add_item(list_item);
	++ _count;
	if (_count == 1) {
		set_current_index(0);
	}
}

void ComboBox::add_item(u8string_view str, const PixBuffer& icon) {
	ListItem list_item;
	list_item.image = icon;
	list_item.text = str;
	_listbox_view->add_item(list_item);
	++ _count;
	if (_count == 1) {
		set_current_index(0);
	}
}

void ComboBox::remove_item(u8string_view what) {
	int index = find_text(what);
	_listbox_view->remove_item(_listbox_view->item_at(index));
	-- _count;
	if (index == _current_index) {
		set_current_index(0);
	}
}

void ComboBox::set_current_index(int index) {
	if (index >= 0 && index < _count) {
		_current_index = index;
		if (_listbox_view->item_at(_current_index)->font.empty()) {
			_text_layout.set_font(font());
		} else {
			_text_layout.set_font(_listbox_view->item_at(_current_index)->font);
		}
		_text_layout.set_text(_listbox_view->item_at(_current_index)->text);
		_current_index_changed(_current_index);
		_current_text_changed(_listbox_view->item_at(_current_index)->text);
		repaint();
	} else {
		if (_count > 0) {
			set_current_index(0);
		} else {
			_text_layout.set_font(font());
			_text_layout.set_text("");
			repaint();
		}
	}
}

void ComboBox::set_current_text(u8string_view text) {
	set_current_index(find_text(text));
}

void ComboBox::set_current_item(ListItem* item) { // 一旦通过listbox选择item了就马上关闭弹窗
	_listbox_view->set_parent(nullptr);
	delete _popup_widget;
	_popup_widget = nullptr;

	set_current_text(item->text);
}

bool ComboBox::set_item_icon(int index, const PixBuffer& icon) {
	auto item = _listbox_view->item_at(index);
	item->image = icon;
	_listbox_view->update_item(item);
	return true;
}

bool ComboBox::paint_event(PaintEvent &event)  {
	// comboBox border
	auto border = rect().cast<float>().apply_margin(style()->margin);
	// text view border
	FRect text_border{border.x + blank, border.y, border.w - border.h / 2, border.h};
	// drop down symbol view border
	FRect drop_down_border{border.x + border.w - border.h / 2, border.y, border.h / 2, border.h};
    auto &painter = this->painter();

	painter.set_brush(palette().input());
	painter.fill_rect(border);
	
    auto size = _text_layout.size();
    auto textbox = text_border.align_object(_text_layout.size(), _align);

    painter.set_font(font());
    painter.set_text_align(AlignLeft + AlignTop);
    painter.set_brush(palette().text());

    painter.push_scissor(text_border);
    painter.draw_text(
        _text_layout,
        textbox.x,
        textbox.y
    );
    painter.pop_scissor();
	painter.push_scissor(drop_down_border);
	if (under_mouse() || _popup_widget != nullptr) {
		painter.set_brush(palette().light());
	} else {
		painter.set_brush(palette().window());
	}
	painter.fill_rect(drop_down_border);
	painter.set_brush(palette().border());
	painter.draw_rect(drop_down_border);
	if (under_mouse() || _popup_widget != nullptr) {
		painter.set_brush(palette().hightlight());
	} else {
		painter.set_brush(palette().light());
	}
	painter.draw_line({drop_down_border.x + drop_down_border.w / 5, drop_down_border.y + drop_down_border.h / 3}, {drop_down_border.x + drop_down_border.w / 2, drop_down_border.y + drop_down_border.h * 2 / 3});
	painter.draw_line({drop_down_border.x + drop_down_border.w * 4 / 5, drop_down_border.y + drop_down_border.h / 3}, {drop_down_border.x + drop_down_border.w / 2, drop_down_border.y + drop_down_border.h * 2 / 3});
	painter.pop_scissor();

	if (under_mouse() || _popup_widget != nullptr) {
		painter.set_brush(palette().hightlight());
	}
	else {
		painter.set_brush(palette().border());
	}
	painter.draw_rect(border);

    return true;
}

bool ComboBox::change_event(ChangeEvent &event) {
	if (event.type() == ChangeEvent::FontChanged) {
		if (_current_index >= 0 && _current_index <= _count) {
			if (!_listbox_view->item_at(_current_index)->font.empty()) {
				_text_layout.set_font(font());
			}
		}
	}
	return true;
}

bool ComboBox::mouse_press(MouseEvent &event) {
	if (event.button() == MouseButton::Left) {
		if (count() == 0) {
			return true;
		}
		auto rect = this->rect();
		Point pos = map_to_screen(rect.position());
		Size  size = map_to_pixels(rect.size());

		Size  lbox_size = _listbox_view->size_hint();
		// Clamp it to max visvible items
		lbox_size.h += 5;
		if (lbox_size.h > _max_visible_items * rect.h) {
			lbox_size.h = _max_visible_items * rect.h;
		}

		_popup_widget = new PopupWidget;
		_popup_widget->popup();
		_popup_widget->set_window_position(pos.x, pos.y + size.h);
		_popup_widget->resize({rect.w, lbox_size.h});
		_listbox_view->set_rect({0, 0, rect.w, lbox_size.h});
		_popup_widget->set_window_borderless(true);
		_popup_widget->set_attribute(WidgetAttrs::DeleteOnClose, true);
		_popup_widget->set_attribute(WidgetAttrs::BackgroundTransparent, true);
		_popup_widget->signal_focus_lost().connect([this]() {
			// Detach it
			_listbox_view->set_parent(nullptr);
			_popup_widget = nullptr;

			repaint();
		});
		_listbox_view->set_parent(_popup_widget);
	}
	return true;
}

bool ComboBox::focus_gained(FocusEvent &event) {
	repaint();
	return true;
}

bool ComboBox::focus_lost   (FocusEvent &event) {
	repaint();
	return true;
}

bool ComboBox::mouse_release(MouseEvent &event) {
	return true;
}

bool ComboBox::mouse_enter(MotionEvent &event) {
	repaint();
	return true;
}

bool ComboBox::mouse_leave(MotionEvent &event) {
	repaint();
	return true;
}

bool ComboBox::mouse_motion(MotionEvent &event) {
	return true;
}

bool ComboBox::mouse_wheel(WheelEvent &event) {
	if (_count > 0 && !_popup_widget) {
		// When not display it
		int len = _count;
		set_current_index(((_current_index - event.y()) % len + len) % len);
	}
	return true;
}

Size ComboBox::size_hint() const {
	float height = style()->button_height / 2;
	TextLayout layout;
	float width = blank;
	for (int i = 0;i < _count; ++ i) {
		const auto& item = _listbox_view->item_at(i);
		if (item->font.empty()) {
			layout.set_font(font());
		} else {
			layout.set_font(item->font);
		}
		layout.set_text(item->text);
		width = max(width, layout.size().w + blank);
	}
	width += height / 2;
	BTK_LOG("win size (%f,%f)\n", width, height);
	return Size(width, height);
}

BTK_NS_END