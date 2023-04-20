#pragma once

#include <Btk/widget.hpp>

BTK_NS_BEGIN

// Layout Tools
class SpacerItem ;
class LayoutItem : public Any {
    public:
        virtual void mark_dirty()               = 0;
        virtual void set_rect(const Rect &rect) = 0;

        virtual Size size_hint() const = 0;
        virtual Rect rect()      const = 0;

        // Safe cast to
        virtual SpacerItem *spacer_item() {return nullptr;}
        virtual Widget *widget() const { return nullptr; }
        virtual Layout *layout() { return nullptr; }

        void set_alignment(Alignment alig) {
            _alignment = alig;
            mark_dirty();
        }
        auto alignment() const -> Alignment {
            return _alignment;
        }
    private:
        Alignment _alignment = {};
};
class WidgetItem final : public LayoutItem {
    public:
        WidgetItem(Widget *w) : wi(w) {}

        void mark_dirty() override {
            
        }
        void set_rect(const Rect &r) override {
           wi->set_rect(r.x, r.y, r.w, r.h); 
        }
        Size size_hint() const override {
            if (wi->size_policy().horizontal_policy() == SizePolicy::Fixed && 
               wi->size_policy().vertical_policy() == SizePolicy::Fixed) 
            {
                // Currently only support fixed
                return wi->size();
            }
            return wi->size_hint();
        }
        Rect rect() const override {
            return wi->rect();
        }

        Widget *widget() const override {
            return wi;
        }
    private:
        Widget *wi;
};
class SpacerItem final : public LayoutItem {
    public:
        SpacerItem(int w, int h) : size(w, h) {}

        void mark_dirty() override {
            
        }
        void set_rect(const Rect &r) override {
        
        }
        Size size_hint() const override {
            return size;
        }
        SpacerItem *spacer_item() override {
            return this;
        }

        Rect rect() const override {
            return Rect(0, 0, size.w, size.h);
        }

        void set_size(int w, int h) {
            size.w = w;
            size.h = h;
        }
    private:
        Size size;
};

class BTKAPI Layout     : public LayoutItem, public Trackable {
    public:
        Layout(Widget *parent = nullptr);
        ~Layout();

        void attach(Widget *widget);
        void set_margin(Margin m);
        void set_spacing(int spacing);
        void set_parent(Layout *lay);

        Margin margin() const {
            return _margin;
        }
        int    spacing() const {
            return _spacing;
        }
        Layout *parent() const {
            return _parent;
        }

        // Override
        Widget *widget() const override;
        Layout *layout() override;

        // Abstract Interface
        virtual void        add_item(LayoutItem *item) = 0;
        virtual int         item_index(LayoutItem *item) = 0;
        virtual LayoutItem *item_at(int idx) = 0;
        virtual LayoutItem *take_item(int idx) = 0;
        virtual int         count_items() = 0;
        virtual void        mark_dirty() = 0;
        virtual void        run_hook(Event &) = 0;
    private:
        static 
        bool EventHook(Object *, Event &event, void *self);
        void on_attached_deleted(); //< Attached widget delete this


        Margin                 _margin = {0} ; //< Content margin
        Widget                *_widget = nullptr; //< Attached 
        Layout                *_parent = nullptr;
        int                    _spacing = 0;

        bool                   _hooked = false; //< Hooked to widget ? (default = false)
        Connection             _con;
};
class BTKAPI BoxLayout : public Layout {
    public:
        BoxLayout(Direction d = LeftToRight);
        BoxLayout(Widget *where, Direction d = LeftToRight) : BoxLayout(d) {
            attach(where);
        }
        ~BoxLayout();

        void add_layout(Layout *lay, int stretch = 0);
        void add_widget(Widget *wid, int stretch = 0, Alignment align = {});
        void add_spacing(int spacing);
        void add_stretch(int stretch);
        void set_direction(Direction d);

        void        add_item(LayoutItem *item)   override;
        int         item_index(LayoutItem *item) override;
        LayoutItem *item_at(int idx)             override;
        LayoutItem *take_item(int idx)           override;
        int         count_items()                override;
        void        mark_dirty()                 override;
        void        run_hook(Event &)            override;
        void        set_rect(const Rect &)       override;
        Rect        rect()                 const override;
        Size        size_hint()            const override; 
    private:
        void        run_layout(const Rect *dst);
        bool        should_skip(LayoutItem *item) const;
        size_t      visible_items() const;
        int         stretch_of(LayoutItem *item) const; //< Get stretch of this Item, it will apply Widget's sizepolicy stretch

        struct ItemExtra {
            int stretch = 0;

            // Cached field in run_layout
            Size alloc_size = {0, 0};
        };

        mutable Size cached_size = {0, 0}; //< Measured size
        mutable bool size_dirty = true;

        std::vector<std::pair<LayoutItem*, ItemExtra>> items;
        Direction _direction = LeftToRight;
        Rect      _rect = {0, 0, 0, 0};
        bool _except_resize = false;
        bool _layouting = false;
        bool _dirty = true;
        int  n_spacing_item = 0;
};

// Easy to use class
class HBoxLayout final : public BoxLayout {
    public:
        HBoxLayout() : BoxLayout(LeftToRight) { }
        HBoxLayout(Widget *parent) : BoxLayout(parent, LeftToRight) { }
};
class VBoxLayout final : public BoxLayout {
    public:
        VBoxLayout() : BoxLayout(TopToBottom) { }
        VBoxLayout(Widget *parent) : BoxLayout(parent, TopToBottom) { }
};

BTK_NS_END