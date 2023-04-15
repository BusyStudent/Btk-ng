#pragma once

#include <Btk/widget.hpp>
#include <vector>

BTK_NS_BEGIN

/**
 * @brief 
 * 
 */
class BTKAPI StackedWidget : public Widget {
    public:
        StackedWidget(Widget * parent = nullptr);
        ~StackedWidget();
        /**
         * @brief Add a widget at bottom of the stack, The StackedWidget will take the widget's ownship
         * 
         * @param widget the widget pointer, nullptr on no-op
         */
        void add_widget(Widget *widget);
        /**
         * @brief Insert a widget at pos, The StackedWidget will take the widget's ownship
         * 
         * @param idx The position of the widget
         * @param widget The widget pointer, nullptr on no-op
         */
        void insert_widget(int idx, Widget *widget);
        /**
         * @brief Remove the widget, it will return back the ownship of the widget
         * 
         * @param widget the widget pointer, nullptr on no-op
         */
        void remove_widget(Widget *widget);
        void delete_widget(Widget *widget);

        void set_current_widget(Widget *widget);
        void set_current_index(int idx);

        void set_margin(const Margin &m);

        Widget *current_widget() const;
        int     current_index()  const;
        Margin  margin()         const;

        int     index_of(Widget *w) const;
        Widget *widget_of(int idx) const;

        BTK_EXPOSE_SIGNAL(_current_changed);
    protected:
        bool resize_event(ResizeEvent &) override;
        bool move_event(MoveEvent &) override;
        bool change_event(ChangeEvent &) override;
    private:
        void apply_margin();

        std::vector<Widget*> _widgets;
        int                  _current = -1;
        bool                 _except_remove = false;
        Margin               _margin = {0};

        Signal<void(int)>    _current_changed;
};

/**
 * @brief TabBar
 * 
 */
class BTKAPI TabBar    : public Widget {
    public:
        TabBar(Widget *parent = nullptr);
        ~TabBar();

        void add_tab(const PixBuffer &icon, u8string_view name);
        void add_tab(u8string_view name) { add_tab({ }, name); }

        void insert_tab(int idx, const PixBuffer &icon, u8string_view name);
        void insert_tab(int idx, u8string_view name) { insert_tab(idx, { }, name); }

        void remove_tab(int idx);

        void set_tab_text(int idx, u8string_view text);
        void set_tab_icon(int idx, const PixBuffer &icon);

        void set_current_index(int idx);
        int  current_index() const {
            return _current_tab;
        }

        // Query Tab
        Rect tab_rect(int idx) const;
        int  tab_at(int x, int y) const;
        int  count_tabs() const;

        // Query size
        Size size_hint() const override;

        BTK_EXPOSE_SIGNAL(_current_changed);
        BTK_EXPOSE_SIGNAL(_tab_clicked);
        BTK_EXPOSE_SIGNAL(_tab_double_clicked);
    protected:
        bool resize_event(ResizeEvent &) override;
        bool paint_event(PaintEvent &) override;
        bool mouse_press(MouseEvent &) override;
        bool mouse_motion(MotionEvent &) override;
    private:
        class TabItem {
            public:
                TextLayout text;
                Brush      icon; // TODO : Icon support

                // Cache
                Rect       rect = {0, 0, 0, 0};
                bool       dirty = true;
        };
        mutable std::vector<TabItem> _tabs; 		//< The tabs in the bar.  Each tab has a name and a brush
        int _current_tab = -1;              //< The idx of the current tab, -1 means no tab selected
        int _hovered_tab = -1;              //< The idx of the hovered tab, -1 means no tab hovered

        Signal<void(int)> _current_changed;
        Signal<void(int)> _tab_clicked;
        Signal<void(int)> _tab_double_clicked;
};
/**
 * @brief Tabbed widget that can contain multiple widgets. The widget will display the title of the tab and the content of the tab.
 * 
 */
class BTKAPI TabWidget : public Widget {
    public:
        TabWidget(Widget  *parent = nullptr);
        ~TabWidget();

        void add_tab(Widget *page, const PixBuffer &icon, u8string_view text);
        void add_tab(Widget *page, u8string_view text) { add_tab(page, { }, text); }

        void insert_tab(int idx, Widget *page, const PixBuffer &icon, u8string_view text);
        void insert_tab(int idx, Widget *page, u8string_view text) { insert_tab(idx, page, { }, text); }

        void remove_tab(int idx);
        void delete_tab(int idx);

        void set_current_index(int idx);
        void set_paint_background(bool v);

        void set_content_margin(const Margin &m) {
            display.set_margin(m);
        }
        void set_tab_text(int idx, u8string_view text) {
            bar.set_tab_text(idx, text);
        }
        void set_tab_icon(int idx, const PixBuffer &icon) {
            bar.set_tab_icon(idx, icon);
        }
        void set_bar_visible(bool v) {
            bar.set_visible(v);
            put_internal();
        }

        // Query
        Margin  content_margin() const;
        Widget *current_widget() const;
        int     current_index()  const;

        auto signal_current_changed() -> Signal<void(int)> & {
            return bar.signal_current_changed();
        }
    protected:
        bool resize_event(ResizeEvent &) override;
        bool move_event(MoveEvent &) override;
        bool paint_event(PaintEvent &) override;
    private:
        void put_internal();

        TabBar        bar     {this};
        StackedWidget display {this};

        bool          paint_background = true;
};
class BTKAPI ListWidget : public Widget {
    public:
        ListWidget();
        ~ListWidget();
    private:
        std::vector<Widget*> widgets;
};

inline void TabWidget::set_paint_background(bool v) {
    paint_background = v;
    repaint();
}

BTK_NS_END