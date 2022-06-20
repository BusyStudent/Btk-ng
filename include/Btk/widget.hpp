#pragma once

#include <Btk/object.hpp>
#include <Btk/string.hpp>
#include <Btk/style.hpp>
#include <Btk/defs.hpp>
#include <Btk/rect.hpp>

#define BTK_EXPOSE_SIGNAL(name) \
    auto &signal##name() { \
        return name;\
    }

BTK_NS_BEGIN

class KeyEvent;
class MouseEvent;
class PaintEvent;
class MotionEvent;

// Enum for window flags
enum class WindowFlags : uint32_t {
    None = 0,
    Resizable  = 1 << 0,
    Fullscreen = 1 << 1,
    Borderless = 1 << 2,
    Hidden     = 1 << 3,
    Maximized  = 1 << 4,
    Minimized  = 1 << 5,
    OpenGL     = 1 << 6,
    Vulkan     = 1 << 7,
};
// Enum for widget.

BTK_FLAGS_OPERATOR(WindowFlags, uint32_t);

class WidgetImpl;

// Event filter, return true to discard event
using EventFilter = bool (*)(Widget *, Event &, void *);

/**
 * @brief Widget base class
 * 
 */
class BTKAPI Widget : public Object {
    public:
        Widget();
        ~Widget();

        bool handle(Event &event) override;

        void show();
        void hide();
        void repaint();
        void set_visible(bool visible);
        void resize(int width, int height);
        void resize(const Point &p) {
            resize(p.x, p.y);
        }

        // Querys
        Rect    rect() const;
        bool    visible() const;
        Widget *parent() const;
        Widget *root() const;
        Style  *style() const;
        bool    is_window() const;

        int     width() const {
            return rect().w;
        }
        int     height() const {
            return rect().h;
        }

        // Size Hints
        virtual Size size_hint() const;

        // Event handlers
        virtual bool key_press  (KeyEvent &) { return false; }
        virtual bool key_release(KeyEvent &) { return false; }

        virtual bool mouse_press  (MouseEvent &) { return false; }
        virtual bool mouse_release(MouseEvent &) { return false; }
        virtual bool mouse_enter  (MotionEvent &) { return false; }
        virtual bool mouse_leave  (MotionEvent &) { return false; }
        virtual bool mouse_motion (MotionEvent &) { return false; }

        virtual bool paint_event(PaintEvent &) { return false; }

        // Get window associated with widget
        window_t   winhandle() const;
        gcontext_t gc() const;

        // Child widgets
        Widget    *child_at(int x, int y) const;

        // EventFilter
        void add_event_filter(EventFilter filter, pointer_t data);
        void del_event_filter(EventFilter filter, pointer_t data);
    private:

        WidgetImpl *priv = nullptr;
};


// Common useful widgets

// class BTKAPI AbstractButton : public Widget {
//     public:
//         AbstractButton();
//         ~AbstractButton();

//         void set_text(const char *text);
// };

// class BTKAPI CheckButton : public AbstractButton {
//     public:
//         CheckButton();
//         ~CheckButton();

//         void set_checked(bool checked);
//         bool is_checked() const;
// };

// class BTKAPI RadioButton : public AbstractButton {
//     public:
//         RadioButton();
//         ~RadioButton();

//         void set_checked(bool checked);
//         bool is_checked() const;
// };
class BTKAPI Frame : public Widget {

};

class BTKAPI Button : public Widget {
    public:
        Button();
        ~Button();

        void set_text(const char *text);
        void set_flat(bool flat) {
            _flat = flat;
            repaint();
        }


        // Event handlers
        bool paint_event(PaintEvent &event) override;
        bool mouse_press(MouseEvent &event) override;
        bool mouse_release(MouseEvent &event) override;
        bool mouse_enter(MotionEvent &event) override;
        bool mouse_leave(MotionEvent &event) override;

        Size size_hint() const override;

        // Signals
        BTK_EXPOSE_SIGNAL(_howered);
        BTK_EXPOSE_SIGNAL(_clicked);
    private:
        u8string _text;
        bool     _pressed = false;
        bool     _entered = false;
        bool     _flat    = false;

        Signal<void()> _howered;
        Signal<void()> _clicked;
};

BTK_NS_END