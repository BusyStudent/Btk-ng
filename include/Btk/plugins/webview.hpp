#pragma once

#include <Btk/widget.hpp>
#include <functional>

#if defined(interface)
#undef interface
#endif


BTK_NS_BEGIN

class WebViewImpl;
class WebInterop;

/**
 * @brief Function to watch request
 * @param url the request url
 * 
 */
using WebRequestWatcher = std::function<void (u8string_view url)>;
/**
 * @brief Function to bind with javascript
 * @param args
 * 
 * @return json to return (empty on undefined)
 * 
 */
using WebFunction       = std::function<void (u8string_view args_json)>;

/**
 * @brief Interface to control the webview
 * 
 */
class WebInterface {
    public:
        virtual ~WebInterface() = default;
        /**
         * @brief Send a request to navigate url
         * 
         * @param url 
         * @return true 
         * @return false Not ready or 
         */
        virtual bool navigate(u8string_view url) = 0;
        /**
         * @brief Send a request to navigate html doc
         * 
         * @param html The String of the htmml doc
         * @return true 
         * @return false 
         */
        virtual bool navigate_html(u8string_view html) = 0;
        /**
         * @brief Inject javascript code at docoument created
         * 
         * @param javascipt The string to inject
         * @return true 
         * @return false 
         */
        virtual bool inject(u8string_view javascipt) = 0;
        /**
         * @brief Execute javascript code at current page
         * 
         * @param javascript The string of javascript code
         * @return true Ok
         * @return false Failed to eval
         */
        virtual bool eval(u8string_view javascript) = 0;
        /**
         * @brief Bind a native function to javascript
         * 
         * @param name The name of it
         * @param func The function to bind see WebFunction
         * @return true Ok
         * @return false failed to bind
         */
        virtual bool bind(u8string_view name, const WebFunction &func) = 0;
        /**
         * @brief Unbind a native function to javascript
         * 
         * @param name 
         * @return true 
         * @return false 
         */
        virtual bool unbind(u8string_view name) = 0;
        /**
         * @brief Reload current page
         * 
         * @return true 
         * @return false 
         */
        virtual bool reload() = 0;
        /**
         * @brief Stop loading cureent page
         * 
         * @return true 
         * @return false 
         */
        virtual bool stop() = 0;
        /**
         * @brief Get current url 
         * 
         * @return u8string 
         */
        virtual u8string url() = 0;
        /**
         * @brief Get low level interop interface
         * 
         * @return WebInterop* 
         */
        virtual WebInterop *interop() = 0;

        /**
         * @brief Signal of environment is fully ready
         * 
         * @return Signal<void()>& 
         */
        auto signal_ready()         -> Signal<void()> & {
            return _ready;
        }
        /**
         * @brief Signal of the docoument title changed
         * 
         * @return Signal<void(u8string_view)>& 
         */
        auto signal_title_changed() -> Signal<void(u8string_view)> & {
            return _title_changed;
        }
        /**
         * @brief Signal of the url changed
         * 
         * @return Signal<void(u8string_view)>& 
         */
        auto signal_url_changed() -> Signal<void(u8string_view)> & {
            return _url_changed;
        }
        auto signal_navigation_starting() -> Signal<void()> & {
            return _navigation_starting;
        }
        auto signal_navigation_compeleted() -> Signal<void()> & {
            return _navigation_completed;
        }
    private:
        Signal<void()> _ready; //< Emit on env ready
        Signal<void(u8string_view)> _title_changed; //< Emit on view title changed
        Signal<void(u8string_view)> _url_changed; //< Emit on view url changed
        Signal<void()> _navigation_starting; //< Emit on view navigation_starting
        Signal<void()> _navigation_completed; //< Emit on view navigation_completed
};
/**
 * @brief Low level interop for WebView
 * 
 */
class WebInterop {
    public:
        /**
         * @brief Register a watcher to watch request
         * 
         * @param watcher The watcher to register
         * @return pointer_t nullptr on failure
         */
        virtual pointer_t add_request_watcher(const WebRequestWatcher &watcher) = 0;
        virtual bool      del_request_watcher(pointer_t where) = 0;
    protected:
        ~WebInterop() = default;
};

class WebSettings {
    public:

};

/**
 * @brief Widget for User to user web
 * 
 */
class BTKAPI WebView final : public Widget {
    public:
        WebView(Widget *parent = nullptr);
        ~WebView();

        /**
         * @brief Query interface
         * 
         * @return WebInterface* 
         */
        WebInterface *interface() const;
        WebInterface *operator ->() const {
            return interface();
        }

        /**
         * @brief Alloc a headless broswer (you take the interface's ownship )
         * 
         * @return WebInterface* nullptr on error
         */
        static WebInterface *AllocHeadless();
    protected:
        bool move_event(MoveEvent &) override;
        bool paint_event(PaintEvent &) override;
        bool resize_event(ResizeEvent &) override;
        bool change_event(ChangeEvent &) override;
        bool focus_gained(FocusEvent &) override;
        bool focus_lost(FocusEvent &) override;;
    private:
        WebViewImpl *priv;
};

BTK_NS_END