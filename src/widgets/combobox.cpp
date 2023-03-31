#include "build.hpp"

#include <Btk/widgets/combobox.hpp>
#include <Btk/event.hpp>
#include <algorithm>

BTK_NS_BEGIN

ComboBox::ComboBox(Widget *parent) : Widget(parent) {
	// 目前不提供其他view类支持，只委托给listbox。
	_listbox_view = new ListBox();
	_listbox_view->signal_item_clicked().connect(&ComboBox::set_current_item, this);
	// _popup_widget.set_hide_on_focus_lost(true);
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
	return _items.size();
}

int ComboBox::find_text(u8string_view text) {
	int index = -1;
	for (int i = 0;i < _items.size(); ++i) {
		if (_items[i].text == text) {
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
	if (_current_index < 0 || _current_index >= _items.size()) {
		return "";
	}
	return _items[_current_index].text;
}

bool ComboBox::set_item_text(int index, u8string_view text) {
	if (index < 0 || _current_index >= _items.size()) {
		return false;
	} else {
		_items[index].text = text;
		auto item = _listbox_view->item_at(index);
		item->text = text;
		_text_changed.emit(index, text);
	}
	return true;
}

void ComboBox::add_item(u8string_view str) {
	ListItem list_item;
	list_item.text = str;
	_items.push_back(list_item);
	_listbox_view->add_item(list_item);
	if (_items.size() == 1) {
		set_current_index(0);
	}
}

void ComboBox::remove_item(u8string_view what) {
	int index = find_text(what);
	_listbox_view->remove_item(_listbox_view->item_at(index));
	_items.erase(_items.begin() + index);
}

void ComboBox::set_current_index(int index) {
	if (index >= 0 && index < _items.size()) {
		_current_index = index;
		if (_items[_current_index].font.empty()) {
			_text_layout.set_font(font());
		} else {
			_text_layout.set_font(_items[_current_index].font);
		}
		_text_layout.set_text(_items[_current_index].text);
		_current_index_changed(_current_index);
		_current_text_changed(_items[_current_index].text);
		repaint();
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

bool ComboBox::paint_event(PaintEvent &event)  {
	// comboBox border
	auto border = this->rect().cast<float>();
	// text view border
	FRect text_border{border.x, border.y, border.w - border.h / 2, border.h};
	// drop down symbol view border
	FRect drop_down_border{border.x + border.w - border.h / 2, border.y, border.h / 2, border.h};
    auto &painter = this->painter();

    auto size = _text_layout.size();
    FRect textbox(0, 0, text_border.w, text_border.h);
    textbox = textbox.align_at(text_border, _align);

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
	painter.set_brush(palette().light());
	painter.fill_rect(drop_down_border);
	painter.set_brush(palette().border());
	painter.draw_rect(drop_down_border);
	painter.set_brush(palette().hightlight());
	painter.draw_line({drop_down_border.x + drop_down_border.w / 5, drop_down_border.y + drop_down_border.h / 3}, {drop_down_border.x + drop_down_border.w / 2, drop_down_border.y + drop_down_border.h * 2 / 3});
	painter.draw_line({drop_down_border.x + drop_down_border.w * 4 / 5, drop_down_border.y + drop_down_border.h / 3}, {drop_down_border.x + drop_down_border.w / 2, drop_down_border.y + drop_down_border.h * 2 / 3});
	painter.pop_scissor();

	painter.set_brush(palette().border());
	painter.draw_rect(border);

    return true;
}

bool ComboBox::change_event(ChangeEvent &event) {
	if (event.type() == ChangeEvent::FontChanged) {
		if (_current_index >= 0 && _current_index <= _items.size()) {
			if (!_items[_current_index].font.empty()) {
				_text_layout.set_font(font());
			}
		}
	}
	return true;
}

bool ComboBox::mouse_press(MouseEvent &event) {
	if (event.button() == MouseButton::Left) {
		auto rect = this->rect();
		Point pos = map_to_screen(rect.position());
		Size  size = map_to_pixels(rect.size());

		_popup_widget = new PopupWidget;
		_popup_widget->popup();
		_popup_widget->set_window_position(pos.x, pos.y + size.h);
		_popup_widget->resize({rect.w, _max_visible_items * rect.h});
		_listbox_view->set_rect({0, 0, rect.w, _max_visible_items * rect.h});
		_popup_widget->set_window_borderless(true);
		_popup_widget->set_attribute(WidgetAttrs::DeleteOnClose, true);
		_popup_widget->set_attribute(WidgetAttrs::BackgroundTransparent, true);
		_popup_widget->signal_focus_lost().connect([this]() {
			// Detach it
			_listbox_view->set_parent(nullptr);
			_popup_widget = nullptr;
		});
		_listbox_view->set_parent(_popup_widget);
	}
	return true;
}

bool ComboBox::mouse_release(MouseEvent &event) {
	return true;
}

bool ComboBox::mouse_enter(MotionEvent &event) {
	return true;
}

bool ComboBox::mouse_leave(MotionEvent &event) {
	return true;
}

bool ComboBox::mouse_motion(MotionEvent &event) {
	return true;
}

bool ComboBox::mouse_wheel(WheelEvent &event) {
	if (_items.size() > 0 && !_popup_widget) {
		// When not display it
		int len = _items.size();
		set_current_index(((_current_index - event.y()) % len + len) % len);
	}
	return true;
}

Size ComboBox::size_hint() const {
	float height = style()->button_height / 2;
	TextLayout layout;
	layout.set_font(font());
	layout.set_text("    ");
	int blank = layout.size().w;
	float width = blank;
	for (const auto& item : _items) {
		if (item.font.empty()) {
			layout.set_font(font());
		} else {
			layout.set_font(item.font);
		}
		layout.set_text(item.text);
		width = max(width, layout.size().w + blank);
	}
	width += height / 2;
	BTK_LOG("win size (%f,%f)\n", width, height);
	return Size(width, height);
}

BTK_NS_END