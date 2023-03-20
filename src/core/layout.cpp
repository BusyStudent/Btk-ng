#include "build.hpp"

#include <Btk/layout.hpp>
#include <Btk/event.hpp>

BTK_NS_BEGIN

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
        // case Event::Moved : {
        //     _rect.x = event.as<MoveEvent>().x();
        //     _rect.y = event.as<MoveEvent>().y();
        //     if (!_except_resize) {
        //         mark_dirty();
        //         run_layout(nullptr);
        //     }
        //     break;
        // }
        case Event::Moved : {
            [[fallthrough]];
        }
        case Event::Resized : {
            _rect = widget()->rect();
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
            mark_dirty();
            break;
        }
        case Event::Show : {
            run_layout(nullptr);
            break;
        }
        case Event::Paint : {
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
            while (iter != items.begin()) {
                --iter;
                if (iter != items.begin()) {
                    if (!should_skip(iter->first)) {
                        // Got 
                        return true;
                    }
                }
            }
            if (iter == items.begin()) {
                // EOF
                return false;
            }
            return true;
        }
        while (iter != items.end()) {
            ++iter;
            if (iter != items.end()) {
                if (!should_skip(iter->first)) {
                    // Got 
                    return true;
                }
            }
        }
        // EOF
        return false;
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
        space = r.w - spacing() * (visible_items() - 1);
    }
    else {
        space = r.h - spacing() * (visible_items() - 1);
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
            int part = space / visible_items();
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
                    if (should_skip(item)) {
                        continue;
                    }

                    auto hint = item->size_hint();
                    
                    result.w += hint.w;
                    result.h = max(result.h, hint.h);
                }

                if (visible_items() > 0) {
                    // Add spacing
                    result.w += spacing * (visible_items() - 1);
                }
                break;
            }
            case BottomToTop : {
                [[fallthrough]];
            }
            case TopToBottom : {
                for (auto [item, extra] : items) {
                    if (should_skip(item)) {
                        continue;
                    }

                    auto hint = item->size_hint();
                    
                    result.h += hint.h;
                    result.w = max(result.w, hint.w);
                }

                if (visible_items() > 0) {
                    // Add spacing
                    result.h += spacing * (visible_items() - 1);
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
bool BoxLayout::should_skip(LayoutItem *item) const {
    auto w = item->widget();
    if (w) {
        if (!w->visible()) {
            return true;
        }
    }
    return false;
}
size_t BoxLayout::visible_items() const {
    size_t n = 0;
    for (auto item : items) {
        if (should_skip(item.first)) {
            continue;
        }
        n += 1;
    }
    return n;
}

BTK_NS_END