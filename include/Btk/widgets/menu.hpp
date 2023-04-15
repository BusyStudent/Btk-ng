#pragma once

#include <Btk/pixels.hpp>
#include <Btk/widget.hpp>

BTK_NS_BEGIN

class PopupMenu;
/**
 * @brief Menu's Item
 * 
 */
class BTKAPI MenuItem {
    public:
        MenuItem();
        MenuItem(u8string_view text) : _text(text) { }
        MenuItem(Widget *wi) : _widget(wi) { }
        ~MenuItem();

        PixBuffer     image() const {
            return _image;
        }
        u8string_view text() const {
            return _text;
        }
        const Font &  font() const {
            return _font;
        }
        Widget       *widget() const {
            return _widget;
        }
        bool          is_seperator() const {
            return _seperator;
        }
        void          set_image(const PixBuffer &img);
        void          set_text(u8string_view text) {
            _text = text;
        }
        void          set_font(const Font &f) {
            _font = f;
        }
        void          prepare_at(Widget *master);

        BTK_EXPOSE_SIGNAL(_hovered);
        BTK_EXPOSE_SIGNAL(_triggered);
    private:
        PixBuffer _image;
        u8string  _text;
        Font      _font;
        bool      _seperator = false;
        Widget   *_widget = nullptr; // For submenus.        

        // < Private for draw
        TextLayout _textlay;
        Brush      _imgbrush;

        // < Signals
        Signal<void()> _hovered;
        Signal<void()> _triggered;
    friend class PopupMenu; //< Using PopupMenu to paint it
    friend class MenuBar;
    friend class Menu;
};
/**
 * @brief Widget for displaying & hold items
 * 
 */
class BTKAPI Menu    : public Widget {
    public:
        Menu(Widget *parent = nullptr, u8string_view text = { });
        Menu(u8string_view text) : Menu(nullptr, text) { }
        ~Menu();

        // Note The Menu Object take it's ownship
        MenuItem *add_menu(Menu *submenu);
        MenuItem *add_separator();
        MenuItem *add_item(MenuItem *item);

        Menu      *add_menu(u8string_view name) {
            auto menu = new Menu(name);
            add_menu(menu);
            return menu;
        }
        MenuItem *add_item(u8string_view name) {
            return add_item(new MenuItem(name)); 
        }
        /**
         * @brief Get the item assosiated with
         * 
         * @return MenuItem* nullptr on none
         */
        MenuItem *binded_item() const;

        Size size_hint() const override;
    protected:
        bool paint_event(PaintEvent &event) override;
        bool mouse_motion(MotionEvent &event) override;
        bool mouse_press(MouseEvent &event) override;
        bool mouse_release(MouseEvent &event) override;
    private:
        std::vector<MenuItem *> _items;
        TextLayout              _textlay;

        bool                    _displaying_popup = false;
        PopupMenu              *_current_popup = nullptr;
    friend class PopupMenu; //< Using PopupMenu to paint it the popuped
};
/**
 * @brief Bar for hold Menu or MenuItem
 * 
 */
class BTKAPI MenuBar : public Widget {
    public:
        MenuBar(Widget *parent = nullptr);
        ~MenuBar();

        MenuItem *add_menu(Menu *submenu);
        MenuItem *add_item(MenuItem *item);
        MenuItem *add_separator();

        Menu      *add_menu(u8string_view name) {
            auto menu = new Menu(name);
            add_menu(menu);
            return menu;
        }
        MenuItem *add_item(u8string_view name) {
            return add_item(new MenuItem(name)); 
        }

        Rect      item_rect(MenuItem *item) const;
        MenuItem *item_at(int x, int y) const;

        Size size_hint() const override;
    protected:
        bool mouse_press(MouseEvent &) override;
        bool mouse_release(MouseEvent &) override;
        bool mouse_motion(MotionEvent &) override;
        bool mouse_enter(MotionEvent &) override;
        bool mouse_leave(MotionEvent &) override;
        bool paint_event(PaintEvent &) override;
        bool resize_event(ResizeEvent &) override;
        bool timer_event(TimerEvent &) override;
        bool move_event(MoveEvent &) override;
    private:
        void relayout();

        std::vector<MenuItem *> _items;

        MenuItem               *_current_item = nullptr;
        size_t                  _num_widget_item = 0; //< Number of widget item

        MenuItem               *_hovered_item  = nullptr; //< Current hovered item
        timerid_t               _hovered_timer = 0; //< For detected mouse hovered
        timestamp_t             _last_motion   = 0; //< For last mouse motion
        int                     _hovered_time  = 300; //< In 300
};

/**
 * @brief Popuped widget, it is better to placee it at heap and set DeleteOnClose attribute
 * 
 */
class BTKAPI PopupWidget : public Widget {
    public:
        PopupWidget();
        ~PopupWidget();

        /**
         * @brief Popup widget at where
         * 
         * @param where The widget for locking up the root widget, nullptr on PopupWindow
         */
        void popup(Widget *where = nullptr);
        /**
         * @brief Set the hide on focus lost, It will replace the behaivor of the widget when focus is lost
         * 
         * @param v 
         */
        void set_hide_on_focus_lost(bool v);
    public:
        // Signals
        BTK_EXPOSE_SIGNAL(_focus_lost);
    protected:
        bool mouse_press(MouseEvent &) override;
        bool mouse_release(MouseEvent &) override;
        bool paint_event(PaintEvent &) override;
        bool focus_lost(FocusEvent &) override;
    private:
        bool hide_on_focus_lost = false;

        Signal<void()> _focus_lost;
};

/**
 * @brief Popuped Menu
 * 
 */
class BTKAPI PopupMenu : public Widget {
    public:
        PopupMenu(Widget *parent = nullptr);
        ~PopupMenu();

        void popup();

        // Note The PopupMenu Object take it's ownship
        MenuItem *add_menu(Menu *submenu);
        MenuItem *add_item(MenuItem *item);
        MenuItem *add_separator();

        // Borrow from an existing menu item, it will not be freed when the menu is closed.
        void borrow_items_from(const std::vector<MenuItem *> &items);
    protected:
        bool mouse_motion(MotionEvent &) override;
        bool mouse_press(MouseEvent &) override;  // For the menu items to be clickable.
        bool mouse_release(MouseEvent &) override;  // For the menu items to be clickable.
        bool mouse_enter(MotionEvent &) override;
        bool mouse_leave(MotionEvent &) override;
        bool paint_event(PaintEvent &) override;
        bool timer_event(TimerEvent &) override;
        bool close_event(CloseEvent &) override;
        bool focus_lost(FocusEvent &) override;
    private:
        Rect      item_rect(MenuItem *item);
        MenuItem *item_at(const Point &where);

        std::vector<MenuItem *> _items;
        bool                    _items_owned = true;

        MenuItem               *_current_item = nullptr;
        PopupMenu              *_current_poping = nullptr;

        MenuItem               *_hovered_item  = nullptr; //< Current hovered item
        timerid_t               _hovered_timer = 0; //< For detected mouse hovered
        timestamp_t             _last_motion   = 0; //< For last mouse motion
        int                     _hovered_time  = 300; //< In 300
};

inline void PopupWidget::set_hide_on_focus_lost(bool v) {
    hide_on_focus_lost = v;
}

BTK_NS_END