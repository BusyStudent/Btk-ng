#pragma once

#include <Btk/widget.hpp>
#include <vector>

BTK_NS_BEGIN

/**
 * @brief Button used by TabWidget
 * 
 */
class BTKAPI TabBar     : public Widget {

};

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

        Widget *current_widget() const;
        int     current_index()  const;

        int     index_of(Widget *w) const;

        BTK_EXPOSE_SIGNAL(_current_changed);
    protected:
        bool resize_event(ResizeEvent &) override;
        bool move_event(MoveEvent &) override;
    private:
        std::vector<Widget*> _widgets;
        int                  _current = -1;

        Signal<void(int)>    _current_changed;
};

class BTKAPI TabWidget : public Widget {
    public:

    protected:
        bool resize_event(ResizeEvent &) override;
        bool move_event(MoveEvent &) override;
        bool paint_event();
    private:
        StackedWidget display {this};
};
class BTKAPI ListWidget : public Widget {
    public:
        ListWidget();
        ~ListWidget();
    private:
        std::vector<Widget*> widgets;
};

BTK_NS_END