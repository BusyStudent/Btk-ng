#include "build.hpp"

#include <Btk/context.hpp>
#include <Btk/comctl.hpp>
#include <Btk/event.hpp>

// Common useful widgets

BTK_NS_BEGIN

Frame::Frame(Widget *parent) : Widget(parent) {}
Frame::~Frame() {}

bool Frame::paint_event(PaintEvent &) {
    auto &painter = this->painter();
    painter.set_stroke_width(_line_width);
    switch (_frame_style) {
        case NoFrame : break;
        case Box : {
            if (under_mouse()) {
                painter.set_color(style()->border);
            }
            else {
                painter.set_color(style()->highlight);
            }
            painter.draw_rect(rect());
            break;
        }
    }
    painter.set_stroke_width(1.0f);
    return true;
}


// ProgressBar
ProgressBar::ProgressBar(Widget *parent) {
    if (parent) {
        parent->add_child(this);
    }
}
ProgressBar::~ProgressBar() {

}

void ProgressBar::set_range(int64_t min, int64_t max) {
    _min = min;
    _max = max;
    _range_changed.emit();
    repaint();
}
void ProgressBar::set_value(int64_t value) {
    value = clamp(value, _min, _max);
    _value = value;
    _value_changed.emit();
    repaint();
}
void ProgressBar::set_direction(Direction direction) {
    _direction = direction;
    repaint();
}
void ProgressBar::set_text_visible(bool visible) {
    _text_visible = visible;
    repaint();
}
void ProgressBar::reset() {
    set_value(_min);

    // TODO : reset animation
}
bool ProgressBar::paint_event(PaintEvent &) {
    auto rect = this->rect().cast<float>();
    auto style = this->style();
    auto &gc  = this->painter();

    auto border = rect.apply_margin(2.0f);

    gc.set_antialias(true);

    // Background
    gc.set_color(style->background);
    gc.fill_rect(border);

    // Progress
    gc.set_color(style->highlight);

    auto bar = border;

    switch(_direction) {
        case LeftToRight : {
            bar.w = (border.w * (_value - _min)) / (_max - _min);
            break;
        }
        case RightToLeft : {
            bar.w = (border.w * (_value - _min)) / (_max - _min);
            bar.x += border.w - bar.w;
            break;
        }
        case TopToBottom : {
            bar.h = (border.h * (_value - _min)) / (_max - _min);
            break;
        }
        case BottomToTop : {
            bar.h = (border.h * (_value - _min)) / (_max - _min);
            bar.y += border.h - bar.h;
            break;
        }
    }

    gc.fill_rect(bar);

    if (_text_visible) {
        int percent = (_value - _min) * 100 / (_max - _min);
        auto text = u8string::format("%d %%", percent);

        gc.set_text_align(Alignment::Center | Alignment::Middle);
        gc.set_font(font());

        gc.set_color(style->text);
        gc.draw_text(
            text,
            border.x + border.w / 2,
            border.y + border.h / 2
        );

        gc.push_scissor(bar);
        gc.set_color(style->highlight_text);
        gc.draw_text(
            text,
            border.x + border.w / 2,
            border.y + border.h / 2
        );
        gc.pop_scissor();
    }


    // Border
    gc.set_color(style->border);
    gc.draw_rect(border);

    // End
    gc.set_antialias(true);
    
    return true;
}

// Layout
Layout::Layout(Widget *p) {
    if (p) {
        attach(p);
    }
}
Layout::~Layout() {
    if (_widget) {
        attach(nullptr);
    }
}
void Layout::attach(Widget *w) {
    auto hook = [](Object *, Event &event, void *self) -> bool {
        static_cast<Layout *>(self)->run_hook(event);
        return false; //< nodiscard
    };
    if (_hooked) {
        // Detach the hook from the previous widget
        _widget->del_event_filter(hook, this);
    }
    _widget = w;

    if (_widget != nullptr) {
        _widget->add_event_filter(hook, this);
        _hooked = true;
    }
}
void Layout::set_margin(Margin ma) {
    _margin = ma;
    mark_dirty();
}
void Layout::set_spacing(int spacing) {
    _spacing = spacing;
    mark_dirty();
}
void Layout::set_parent(Layout *l) {
    _parent = l;
    mark_dirty();
}
Layout *Layout::layout() {
    return this;
}
Widget *Layout::widget() const {
    return _widget;
}
// BoxLayout
BoxLayout::BoxLayout(Direction d) {
    _direction = d;
}
BoxLayout::~BoxLayout() {
    for (auto v : items) {
        delete v.first;
    }
}
void BoxLayout::add_item(LayoutItem *item) {
    mark_dirty();
    items.push_back(std::make_pair(item, ItemExtra()));
}
void BoxLayout::add_widget(Widget *w, int stretch, Alignment align) {
    if (w) {
        auto p = parent();
        if (p) {
            // To top
            while(p->parent()) {
                p = p->parent();
            }
        }
        else {
            p = this;
        }
        auto item = new WidgetItem(w);
        item->set_alignment(align);

        mark_dirty();
        items.push_back(std::make_pair(item, ItemExtra{stretch}));

        p->widget()->add_child(w);
    }
}
void BoxLayout::add_layout(Layout *l, int stretch) {
    if (l) {
        mark_dirty();
        items.push_back(std::make_pair(static_cast<LayoutItem*>(l), ItemExtra{stretch}));
        l->set_parent(this);
    }
}
void BoxLayout::add_stretch(int stretch) {
    mark_dirty();
    items.push_back(std::make_pair(static_cast<LayoutItem*>(new SpacerItem(0, 0)), ItemExtra{stretch}));
}
void BoxLayout::add_spacing(int spacing) {
    mark_dirty();
    Size size = {0, 0};
    if (_direction == RightToLeft || _direction == LeftToRight) {
        size.w = spacing;
    }
    else {
        size.h = spacing;
    }
    add_item(new SpacerItem(size.w, size.h));
    n_spacing_item += 1;
}
void BoxLayout::set_direction(Direction d) {
    _direction = d;
    mark_dirty();

    if (n_spacing_item) {
        // We need for and excahnge items w, h
        for (auto [item, extra] : items) {
            auto i = item->spacer_item();
            if (!i) {
                continue;
            }
            auto size = item->size_hint();
            std::swap(size.w, size.h);
            i->set_size(size.w, size.h);

            --n_spacing_item;
            if (!n_spacing_item) {
                break;
            }
        }
    }
}
int  BoxLayout::item_index(LayoutItem *item) {
    auto iter = items.begin();
    while (iter != items.end()) {
        if (iter->first == item) {
            break;
        }
        ++iter;
    }
    if (iter == items.end()) {
        return -1;
    }
    return iter - items.begin();
}
int  BoxLayout::count_items() {
    return items.size();
}
LayoutItem *BoxLayout::item_at(int idx) {
    if (idx < 0 || idx > items.size()) {
        return nullptr;
    }
    return items[idx].first;
}
LayoutItem *BoxLayout::take_item(int idx) {
    if (idx < 0 || idx > items.size()) {
        return nullptr;
    }
    auto it = items[idx];
    items.erase(items.begin() + idx);
    mark_dirty();

    if (it.first->spacer_item()) {
        n_spacing_item -= 1;
    }

    return it.first;
}
void BoxLayout::mark_dirty() {
    size_dirty = true;
    _dirty = true;
}
void BoxLayout::run_hook(Event &event) {
    switch (event.type()) {
        case Event::Resized : {
            _rect.x = 0;
            _rect.y = 0;
            _rect.w = event.as<ResizeEvent>().width();
            _rect.h = event.as<ResizeEvent>().height();
            if (!_except_resize) {
                mark_dirty();
                run_layout(nullptr);
            }
            break;
        }
        case Event::LayoutRequest : {
            if (_layouting) {
                break;
            }
            [[fallthrough]];
        }
        case Event::Show : {
            run_layout(nullptr);
            break;
        }
    }
}
void BoxLayout::run_layout(const Rect *dst) {
    if (items.empty() || !_dirty) {
        return;
    }
    _layouting = true;
    // Calc need size
    Rect r;

    if (!dst) {
        // In tree top, measure it
        Size size;
        size = size_hint();
        r    = rect();
        if (r.w < size.w || r.h < size.h) {
            // We need bigger size
            BTK_LOG("BoxLayout resize to bigger size (%d, %d)\n", size.w, size.h);
            BTK_LOG("Current size (%d, %d)\n", r.w, r.h);

            assert(widget());
            
            _except_resize = true;
            widget()->set_minimum_size(size);
            widget()->resize(size);
            _except_resize = false;

            r.w = size.w;
            r.h = size.h;
        }
    }
    else {
        // Sub, just pack
        r = *dst;
    }
    r = r.apply_margin(margin());

    BTK_LOG("BoxLayout: run_layout\n");

    // Begin
    auto iter = items.begin();
    auto move_next = [&, this]() -> bool {
        if (_direction == RightToLeft || _direction == BottomToTop) {
            // Reverse
            if (iter == items.begin()) {
                // EOF
                return false;
            }
            --iter;
            return true;
        }
        ++iter;
        return iter != items.end();
    };
    auto move_to_begin = [&, this]() -> void {
        if (_direction == RightToLeft || _direction == BottomToTop) {
            iter = --items.end();
        }
        else {
            iter = items.begin();
        }
    };
    auto is_vertical = (_direction == RightToLeft || _direction == LeftToRight);

    move_to_begin();
    int stretch_sum = 0; //< All widget stretch
    int space = 0;

    if (is_vertical) {
        space = r.w - spacing() * (items.size() - 1);
    }
    else {
        space = r.h - spacing() * (items.size() - 1);
    }
    // Useable space
    do {
        auto &extra = iter->second;
        auto  item  = iter->first;
        auto  hint  = item->size_hint();

        // Save current alloc size
        extra.alloc_size = hint;

        if (is_vertical) {
            space -= hint.w;
        }
        else {
            space -= hint.h;
        }

        if (extra.stretch) {
            // Has stretch
            stretch_sum += extra.stretch;
        }
    }
    while(move_next());

    // Process extra space if
    if (space) {
        move_to_begin();
        BTK_LOG("BoxLayout: has extra space\n");
        if (stretch_sum) {
            // Alloc space for item has stretch
            int part = space / stretch_sum;
            do {
                auto &extra = iter->second;
                auto  item = iter->first;

                if (!extra.stretch) {
                    continue;
                }
                if (is_vertical) {
                    extra.alloc_size.w += part * extra.stretch;
                }
                else {
                    extra.alloc_size.h += part * extra.stretch;
                }
            }
            while(move_next());
        }
        else {
            // Alloc space for item has stretch 0 by size policy
            // TODO : temp use avg
            int part = space / items.size();
            do {
                auto &extra = iter->second;
                auto  item = iter->first;

                if (is_vertical) {
                    extra.alloc_size.w += part;
                }
                else {
                    extra.alloc_size.h += part;
                }
            }
            while(move_next());
        }
    }


    // Assign
    move_to_begin();
    int x = r.x;
    int y = r.y;
    do {
        auto &extra = iter->second;
        auto  item  = iter->first;
        auto  size  = extra.alloc_size;
        auto  rect  = Rect(x, y, size.w, size.h);
        // Make it to box
        if (is_vertical) {
            rect.h = r.h;
        }
        else {
            rect.w = r.w;
        }

        if (item->alignment() != Alignment{}) {
            // Align it
            auto perfered = item->size_hint();
            auto alig     = item->alignment();

            if (bool(alig & Alignment::Left)) {
                rect.w = perfered.w;
            }
            else if (bool(alig & Alignment::Right)) {
                rect.x = rect.x + rect.w - perfered.w;
                rect.w = perfered.w;
            }
            else if (bool(alig & Alignment::Center)) {
                rect.x = rect.x + rect.w / 2 - perfered.w / 2;
                rect.w = perfered.w;
            }

            if (bool(alig & Alignment::Top)) {
                rect.h = perfered.h;
            }
            else if (bool(alig & Alignment::Bottom)) {
                rect.y = rect.y + rect.h - perfered.h;
                rect.h = perfered.h;
            }
            else if (bool(alig & Alignment::Middle)) {
                rect.y = rect.y + rect.h / 2 - perfered.h / 2;
                rect.h = perfered.h;
            }
        }

        item->set_rect(rect);

        if (is_vertical) {
            x += size.w;
            x += spacing();
        }
        else {
            y += size.h;
            y += spacing();
        }
    }
    while(move_next());

    _dirty = false;
    _layouting = false;
}
void BoxLayout::set_rect(const Rect &r) {
    auto w = widget();
    if (w != nullptr) {
        _except_resize = true;
        w->set_rect(r.x, r.y, r.w, r.h);
        _except_resize = false;
    }
    _rect = r;
    mark_dirty();
    run_layout(&_rect);
}
Size BoxLayout::size_hint() const {
    if (size_dirty) {
        // Measure the width
        auto spacing = this->spacing();
        auto margin = this->margin();
        Rect result = {0, 0, 0, 0};

        switch (_direction) {
            case LeftToRight : {
                [[fallthrough]];
            }
            case RightToLeft : {
                for (auto [item, extra] : items) {
                    auto hint = item->size_hint();
                    
                    result.w += hint.w;
                    result.h = max(result.h, hint.h);
                }

                if (items.size() > 0) {
                    // Add spacing
                    result.w += spacing * (items.size() - 1);
                }
                break;
            }
            case BottomToTop : {
                [[fallthrough]];
            }
            case TopToBottom : {
                for (auto [item, extra] : items) {
                    auto hint = item->size_hint();
                    
                    result.h += hint.h;
                    result.w = max(result.w, hint.w);
                }

                if (items.size() > 0) {
                    // Add spacing
                    result.h += spacing * (items.size() - 1);
                }
                break;
            }
        }
        cached_size = result.unapply_margin(margin).size();
        size_dirty = false;
    }
    BTK_LOG("BoxLayout size_hint (%d, %d)\n", cached_size.w, cached_size.h);
    return cached_size;
}
Rect BoxLayout::rect() const {
    // auto w = widget();
    // if (w) {
    //     return w->rect();
    // }
    return _rect;
}


BTK_NS_END