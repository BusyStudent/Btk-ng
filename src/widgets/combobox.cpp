#include <Btk/event.hpp>
#include <Btk/widgets/combobox.hpp>
#include <Btk/detail/platform.hpp>
#include <algorithm>

BTK_NS_BEGIN

ComboBox::ComboBox(Widget *parent) : Widget(parent) {
	_listbox_view = new ListBox(&_popup_widget);
	_listbox_view->signal_item_clicked().connect(&ComboBox::set_current_item, this);
	_text_layout.set_font(font());
}

ComboBox::~ComboBox() {
    if (_listbox_view) {
        delete _listbox_view;
    }
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
	return std::find(_items.begin(), _items.end(), text) - _items.begin();
}

int ComboBox::current_index() const {
	return _current_index;
}
u8string ComboBox::current_text() const {
	if (_current_index < 0 || _current_index >= _items.size()) {
		return "";
	}
	return _items[_current_index];
}

bool ComboBox::set_item_text(int index, u8string_view text) {
	if (index < 0 || _current_index >= _items.size()) {
		return false;
	} else {
		_items[index] = text;
		auto item = _listbox_view->item_at(index);
		item->text = text;
		_text_changed.emit(index, text);
	}
	return true;
}

void ComboBox::add_item(u8string_view str) {
	_items.push_back(str.copy());
	ListItem list_item;
	list_item.text = str;
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
		_text_layout.set_text(_items[_current_index]);
		_current_index_changed(_current_index);
		_current_text_changed(_items[_current_index]);
		repaint();
	}
}

void ComboBox::set_current_text(u8string_view text) {
	set_current_index(find_text(text));
}

void ComboBox::set_current_item(ListItem* item) {
	_popup_widget.close();
	set_current_text(item->text);
}

bool ComboBox::paint_event(PaintEvent &event)  {
	auto border = this->rect().cast<float>();
	FRect text_border{border.x, border.y, border.w - border.h / 2, border.h};
	FRect drop_down_border{border.x + border.w - border.h / 2, border.y, border.h / 2, border.h};
    auto &painter = this->painter();

    // Get textbox
    auto size = _text_layout.size();
    FRect textbox(0, 0, text_border.w, text_border.h);
    textbox = textbox.align_at(text_border, _align);

    painter.set_font(font());
    painter.set_text_align(AlignLeft + AlignTop);
    painter.set_brush(palette().text());

    painter.push_scissor(text_border);
	BTK_LOG("text layout : %s\n", _text_layout.text().data());
    painter.draw_text(
        _text_layout,
        textbox.x,
        textbox.y
    );
    painter.pop_scissor();

	painter.push_scissor(drop_down_border);
	painter.set_brush(palette().button());
	painter.fill_rect(drop_down_border);
	painter.set_brush(palette().border());
	painter.draw_rect(drop_down_border);
	painter.set_brush(palette().light());
	painter.draw_line({drop_down_border.x + drop_down_border.w / 5, drop_down_border.y + drop_down_border.h / 3}, {drop_down_border.x + drop_down_border.w / 2, drop_down_border.y + drop_down_border.h * 2 / 3});
	painter.draw_line({drop_down_border.x + drop_down_border.w * 4 / 5, drop_down_border.y + drop_down_border.h / 3}, {drop_down_border.x + drop_down_border.w / 2, drop_down_border.y + drop_down_border.h * 2 / 3});
	painter.pop_scissor();

	painter.set_brush(palette().border());
	painter.draw_rect(border);

    return true;
}

bool ComboBox::change_event(ChangeEvent &event) {
	if (event.type() == ChangeEvent::FontChanged) {
		_text_layout.set_font(font());
	}
	return true;
}

bool ComboBox::mouse_press(MouseEvent &event) {
	if (event.button() == MouseButton::Left) {
		auto rect = this->rect();
		Point pos = root()->winhandle()->map_point({rect.x, rect.y}, AbstractWindow::ToScreen);
		Point size = root()->winhandle()->map_point({rect.w, rect.h}, AbstractWindow::ToPixel);
		_popup_widget.show();
		_popup_widget.set_window_position(pos.x,pos.y + size.y);
		_popup_widget.resize({rect.w, _max_visible_items * style()->button_height});
		_listbox_view->set_rect({0, 0, rect.w, _max_visible_items * style()->button_height});
		_popup_widget.set_window_borderless(true);
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
	if (_items.size() > 0) {
		int len = _items.size();
		set_current_index(((_current_index + event.y()) % len + len) % len);
	}
	return true;
}

Size ComboBox::size_hint() const {
	float height = style()->button_height;
	float width = 0;
	TextLayout layout;
	layout.set_font(font());
	for (const auto& text : _items) {
		layout.set_text(text);
		width = max(width, layout.size().w);
	}
	width += height / 2;
	BTK_LOG("win size (%f,%f)\n", width, height);
	return Size(width, height);
}

BTK_NS_END