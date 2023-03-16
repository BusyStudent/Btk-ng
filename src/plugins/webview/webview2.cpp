#include "build.hpp"

#include <Btk/detail/platform.hpp>
#include <Btk/plugins/webview.hpp>
#include "common/win32/com.hpp"
#include "WebView2.h"
#include <cstring>
#include <wrl.h>
#include <map>


#undef interface

BTK_PRIV_BEGIN

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

decltype(CreateCoreWebView2EnvironmentWithOptions) *webview2_CreateCoreWebView2EnvironmentWithOptions = nullptr;
decltype(DllCanUnloadNow)                          *webview2_DllCanUnloadNow = nullptr;
static ICoreWebView2Environment                    *webview2_enviroment = nullptr;
static ULONG                                        webview2_refcount = 0;

HMODULE webview2_dll = nullptr;


/**
 * @brief Class for Function binding
 * 
 */
class MSWebView2Binding final : public ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler {
    public:
        MSWebView2Binding(ICoreWebView2 *view, const WebFunction &func);
        ~MSWebView2Binding();

        ULONG   AddRef() override;
        ULONG   Release() override;
        HRESULT QueryInterface(REFIID id, void **pobj) override;

        HRESULT Invoke(HRESULT errorCode, LPCWSTR id) override;
        void    Invoke(std::wstring_view args);
    private:
        ComPtr<ICoreWebView2> webview;
        WebFunction func;
        ULONG refcount = 0;
        LPCWSTR id = nullptr;
};

/**
 * @brief Core class for WebView
 * 
 */
class MSWebView2 : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, //< For create env
                   public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, //< For create control
                   public ICoreWebView2PermissionRequestedEventHandler,
                   public ICoreWebView2WebMessageReceivedEventHandler,
                   public ICoreWebView2NewWindowRequestedEventHandler, //< For webview require new window
                   public ICoreWebView2SourceChangedEventHandler, //< For get url changed
                   public WebInterface,
                   public WebInterop 
{
    public:
        MSWebView2();
        ~MSWebView2();

        // IUnkown
        ULONG   AddRef() final override;
        ULONG   Release() final override;
        HRESULT QueryInterface(REFIID id, void **pobj) final override;

        // ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
        HRESULT Invoke(HRESULT errorCode, ICoreWebView2Environment *createdEnvironment) override;
        // ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
        HRESULT Invoke(HRESULT errorCode, ICoreWebView2Controller *createdController) override;
        // ICoreWebView2WebMessageReceivedEventHandler
        HRESULT Invoke(ICoreWebView2 *sender, ICoreWebView2WebMessageReceivedEventArgs *args) override;
        // ICoreWebView2PermissionRequestedEventHandler
        HRESULT Invoke(ICoreWebView2 *sender, ICoreWebView2PermissionRequestedEventArgs *args) override;
        // ICoreWebView2NewWindowRequestedEventHandler
        HRESULT Invoke(ICoreWebView2 *sender, ICoreWebView2NewWindowRequestedEventArgs *args) override;
        // ICoreWebView2SourceChangedEventHandler
        HRESULT Invoke(ICoreWebView2 *sender, ICoreWebView2SourceChangedEventArgs *args) override;

        // Init
        HRESULT Initialize(HWND h);
        HRESULT InitializeHeadless();

        HRESULT PutBounds(RECT rect);
        HRESULT SetVisible(BOOL visible);

        BOOL    IsReady() const;

        // WebInterface
        bool    navigate(u8string_view url) override;
        bool    navigate_html(u8string_view html) override;
        bool    inject(u8string_view javascript) override;
        bool    eval(u8string_view javascript) override;
        bool    bind(u8string_view name, const WebFunction &func) override;
        bool    unbind(u8string_view name) override;
        bool    reload() override;
        bool    stop() override;
        u8string url() override;
        WebInterop *interop() override;

        // WebInterop
        pointer_t add_request_watcher(const WebRequestWatcher &watcher) override;

        BTK_EXPOSE_SIGNAL(_got_focus);
        BTK_EXPOSE_SIGNAL(_lost_focus);
    private:
        HMODULE LoadLoader();

        Win32::ComInitializer cominit; //< Helper for init com
        ComPtr<ICoreWebView2Environment> enviroment;
        ComPtr<ICoreWebView2Controller> controller;
        ComPtr<ICoreWebView2> webview;

        // Map for binding function
        std::map<std::wstring, ComPtr<MSWebView2Binding>> bindings;

        HWND hwnd = nullptr; //< The parent window (nullptr on headless)
        ULONG refcount = 0; //< The refcount for COM
        BOOL ready = FALSE; //< Is fully inited
        BOOL filter_all = FALSE; //< Does filter all was added
        BOOL headless = TRUE; //< It is headless mode

        Signal<void()> _got_focus;
        Signal<void()> _lost_focus;
};
/**
 * @brief Wrapper for Manage WSTRING return by COM
 * 
 */
class ComWString {
    public:
        ComWString() = default;
        ComWString(const ComWString &) = delete;
        ~ComWString() { ::CoTaskMemFree(ws); }

        LPWSTR * operator &() noexcept {
            return &ws;
        }
        u8string to_utf8() const noexcept {
            return u8string::from(ws);
        }
        LPWSTR   get() const noexcept {
            return ws;
        }
    private:
        LPWSTR ws = nullptr;
};
class ComToWString {
    public:
        template <typename T>
        ComToWString(const T &input) : u16(input.to_utf16()) { }
        ~ComToWString() = default;

        operator LPCWSTR() const noexcept {
            static_assert(sizeof(WCHAR) == sizeof(char16_t));
            return reinterpret_cast<LPCWSTR>(u16.c_str());
        }
    private:
        std::u16string u16;
};

MSWebView2::MSWebView2() {

}
MSWebView2::~MSWebView2() {
    if (headless) {
        ::DestroyWindow(hwnd);
    }

    // Deref env
    webview2_refcount --;
    if (webview2_refcount == 0) {
        webview2_enviroment = nullptr;
    }
}

// Impl as it
ULONG MSWebView2::AddRef() {
    return ++refcount;
}
ULONG MSWebView2::Release() {
    if (refcount > 1) {
        return --refcount;
    }
    delete this;
    return 0;
}
HRESULT MSWebView2::QueryInterface(REFIID id, void **pobj) {
    // Check if
    if (id == __uuidof(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)) {
        *pobj = static_cast<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*>(this);
        AddRef();
        return S_OK;
    }
    if (id == __uuidof(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)) {
        *pobj = static_cast<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*>(this);
        AddRef();
        return S_OK;
    }
    if (id == __uuidof(ICoreWebView2WebMessageReceivedEventHandler)) {
        *pobj = static_cast<ICoreWebView2WebMessageReceivedEventHandler*>(this);
        AddRef();
        return S_OK;
    }
    if (id == __uuidof(ICoreWebView2PermissionRequestedEventHandler)) {
        *pobj = static_cast<ICoreWebView2PermissionRequestedEventHandler*>(this);
        AddRef();
    }
    return E_NOINTERFACE;
}
HRESULT MSWebView2::Initialize(HWND h) {
    // Has env, Just enter create controler
    if (webview2_enviroment) {
        return Invoke(S_OK, webview2_enviroment);
    }
    // No Global env, try load one
    if (!webview2_dll) {
        webview2_dll = LoadLibraryA("WebView2Loader.dll");
        webview2_DllCanUnloadNow = (decltype(::DllCanUnloadNow)*)
            GetProcAddress(webview2_dll, "DllCanUnloadNow");
    }
    if (!webview2_dll) {
        return E_FAIL;
    }
    if (!webview2_CreateCoreWebView2EnvironmentWithOptions) {
        webview2_CreateCoreWebView2EnvironmentWithOptions = (decltype(::CreateCoreWebView2EnvironmentWithOptions)*)
            (GetProcAddress(webview2_dll, "CreateCoreWebView2EnvironmentWithOptions"));
        webview2_DllCanUnloadNow = (decltype(::DllCanUnloadNow)*)
            (GetProcAddress(webview2_dll, "DllCanUnloadNow"));
    }
    if (!webview2_CreateCoreWebView2EnvironmentWithOptions) {
        return E_FAIL;
    }
    hwnd = h;
    return webview2_CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr, this);
}
HRESULT MSWebView2::InitializeHeadless() {
    // Create headless window here
    auto register_headless = []() {
        WNDCLASSEXW info = { };
        info.cbSize = sizeof(WNDCLASSEXW);
        info.lpszClassName = L"BtkHeadlessWebView";
        info.lpfnWndProc = ::DefWindowProcW;
        ::RegisterClassExW(&info);
    };
    BTK_ONCE(register_headless());

    // Make the window hidden
    hwnd = ::CreateWindowExW(
        0,
        L"BtkHeadlessWebView",
        nullptr,
        0,
        0,
        0,
        10,
        10,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );

    if (hwnd == nullptr) {
        return E_FAIL;
    }

    headless = TRUE;

    return Initialize(hwnd);
}
HRESULT MSWebView2::Invoke(HRESULT errorCode, ICoreWebView2Environment *createdEnvironment) {
    if (FAILED(errorCode)) {
        BTK_LOG(BTK_BLUE("[MSWebView2] ") "Invoke(ICoreWebView2Environment *) => %ld\n", errorCode);
        return errorCode;
    }

    // Setup global env & refcount
    webview2_refcount ++;
    webview2_enviroment = createdEnvironment;
    enviroment = createdEnvironment;

    // auto hr =  createdEnvironment->CreateCoreWebView2Controller(hwnd, this);
    auto hr = createdEnvironment->CreateCoreWebView2Controller(hwnd, this);
    if (FAILED(hr)) {
        BTK_LOG(BTK_BLUE("[MSWebView2] ") "CreateCoreWebView2Controller() => %ld\n", errorCode);
    }

    // Ok, Try unload if
    if (webview2_DllCanUnloadNow) {
        if (webview2_DllCanUnloadNow()) {
            FreeLibrary(webview2_dll);
            webview2_dll = nullptr;
            webview2_DllCanUnloadNow = nullptr;
            webview2_CreateCoreWebView2EnvironmentWithOptions = nullptr;
        }
    }
    return hr;
}
HRESULT MSWebView2::Invoke(HRESULT errorCode, ICoreWebView2Controller *createdController) {
    if (FAILED(errorCode)) {
        BTK_LOG(BTK_BLUE("[MSWebView2] ") "Invoke(ICoreWebView2Controller *) => %ld\n", errorCode);
        return errorCode;
    }
    if (createdController) {
        controller = createdController;
        controller->get_CoreWebView2(&webview);
    }

    ComPtr<ICoreWebView2Settings> settings;
    webview->get_Settings(&settings);

    // Enable somthing
    settings->put_IsScriptEnabled(true);
    settings->put_IsWebMessageEnabled(true);
    settings->put_AreDevToolsEnabled(true);

    // Listen
    EventRegistrationToken token;
    webview->add_PermissionRequested(this, &token);
    webview->add_WebMessageReceived(this, &token);
    webview->add_NewWindowRequested(this, &token);
    webview->add_SourceChanged(this, &token);

    // Listen focus
    controller->add_GotFocus(
        Callback<ICoreWebView2FocusChangedEventHandler>(
            [this](ICoreWebView2Controller *sender, IUnknown *) -> HRESULT {
                signal_got_focus().emit();
                return S_OK;
    }).Get(), &token);
    
    controller->add_LostFocus(
        Callback<ICoreWebView2FocusChangedEventHandler>(
            [this](ICoreWebView2Controller *sender, IUnknown *) -> HRESULT {
                signal_lost_focus().emit();
                return S_OK;
    }).Get(), &token);


    ready = TRUE;

    BTK_LOG(BTK_BLUE("[MSWebView2] ") "Env is fully ready\n");
    signal_ready().emit();

    return S_OK;
}
HRESULT MSWebView2::Invoke(ICoreWebView2 *sender, ICoreWebView2WebMessageReceivedEventArgs *args) {
    ComWString source;
    ComWString content;
    args->get_Source(&source);
    if (FAILED(args->TryGetWebMessageAsString(&content))) {
        // Not String
        return false;
    }

    BTK_LOG(BTK_BLUE("[MSWebView2] ") "WebMessage by %s : %s\n", source.to_utf8().c_str(), content.to_utf8().c_str());

    // Try to parse it
    std::wstring_view wcontent(content.get());
    
    if (wcontent.find(L"WebViewCall:") == 0) {
        auto space_pos = wcontent.find(L" ");
        auto name_pos = wcontent.find(L":");
        if (space_pos != std::wstring_view::npos && name_pos != std::wstring::npos && name_pos < space_pos) {
            auto name = wcontent.substr(name_pos + 1, space_pos - name_pos - 1);
            auto object = wcontent.substr(wcontent.find(L" ") + 1);

            auto iter = bindings.find(std::wstring(name));
            if (iter != bindings.end()) {
                // Got it
                iter->second->Invoke(object);
            }
            else {
                BTK_LOG(BTK_BLUE("[MSWebView2] ") "No such binding %s\n", u8string::from(name).c_str());
            }
        }
    }

    return S_OK;
}
HRESULT MSWebView2::Invoke(ICoreWebView2 *sender, ICoreWebView2PermissionRequestedEventArgs *args) {
    COREWEBVIEW2_PERMISSION_KIND kind;
    args->get_PermissionKind(&kind);
    if (kind == COREWEBVIEW2_PERMISSION_KIND_CLIPBOARD_READ) {
      args->put_State(COREWEBVIEW2_PERMISSION_STATE_ALLOW);
    }

#if !defined(NDEBUG)
    ComWString url;
    args->get_Uri(&url);
    BTK_LOG(BTK_BLUE("[MSWebView2] ") "Req Permission by %s\n", url.to_utf8().c_str());
#endif
    return S_OK;
}
HRESULT MSWebView2::Invoke(ICoreWebView2 *sender, ICoreWebView2NewWindowRequestedEventArgs *args) {
    // Default to drop new window
#if !defined(NDEBUG)
    ComWString url;
    args->get_Uri(&url);
    BTK_LOG(BTK_BLUE("[MSWebView2] ") "Reject new Window by %s\n", url.to_utf8().c_str());
#endif

    args->put_Handled(TRUE);
    return S_OK;
}
HRESULT MSWebView2::Invoke(ICoreWebView2 *sender, ICoreWebView2SourceChangedEventArgs *args) {
    BOOL value;
    args->get_IsNewDocument(&value);

    if (value) {
        ComWString url;
        ComWString title;
        webview->get_Source(&url);
        webview->get_DocumentTitle(&title);

        signal_url_changed().emit(url.to_utf8());
        signal_title_changed().emit(title.to_utf8());
    }

    return S_OK;
}
HRESULT MSWebView2::PutBounds(RECT rect) {
    if (!IsReady()) {
        return E_FAIL;
    }
    return controller->put_Bounds(rect);
}
HRESULT MSWebView2::SetVisible(BOOL v) {
    if (!IsReady()) {
        return E_FAIL;
    }
    return controller->put_IsVisible(v);
}
BOOL    MSWebView2::IsReady() const {
    return ready;
}

// WebInterface
bool MSWebView2::navigate(u8string_view url) {
    if (!IsReady()) {
        return false;
    }
    return SUCCEEDED(webview->Navigate(ComToWString(url)));
}
bool MSWebView2::navigate_html(u8string_view html) {
    if (!IsReady()) {
        return false;
    }
    return SUCCEEDED(webview->NavigateToString(ComToWString(html)));
}
bool MSWebView2::eval(u8string_view javascript) {
    if (!IsReady()) {
        return false;
    }
    return SUCCEEDED(webview->ExecuteScript(ComToWString(javascript), nullptr));
}
bool MSWebView2::inject(u8string_view javascript) {
    if (!IsReady()) {
        return false;
    }
    return SUCCEEDED(webview->AddScriptToExecuteOnDocumentCreated(ComToWString(javascript), nullptr));
}
bool MSWebView2::bind(u8string_view name, const WebFunction &func) {
    if (!IsReady()) {
        return false;
    }
    if (!func) {
        return false;
    }
    ComPtr<MSWebView2Binding> binding(
        new MSWebView2Binding(webview.Get(), func)
    );
    auto uname = u8string(name);
    auto script = u8string::format(R"(
        function %s() {
            var name = "%s";
            var json = JSON.stringify(arguments);
            chrome.webview.postMessage(`WebViewCall:${name} ${json}`);
        }
    )", uname.c_str(), uname.c_str()).to_utf16();

    // Inject to master
    HRESULT hr;
    hr = webview->AddScriptToExecuteOnDocumentCreated(reinterpret_cast<LPCWSTR>(script.c_str()), binding.Get());
    if (FAILED(hr)) {
        return false;
    }
    // Add to current
    hr = webview->ExecuteScript(reinterpret_cast<LPCWSTR>(script.c_str()), nullptr);
    if (FAILED(hr)) {
        return false;
    }

    // Register to table
    auto wname = name.to_utf16();
    bindings.insert(
        std::make_pair(std::wstring(reinterpret_cast<LPCWSTR>(wname.c_str())), binding)
    );
    return true;
}
bool MSWebView2::unbind(u8string_view name) {
    if (!IsReady()) {
        return false;
    }
    auto wname = name.to_utf16();
    auto iter = bindings.find(reinterpret_cast<LPCWSTR>(wname.c_str()));
    if (iter == bindings.end()) {
        // Not founded
        return false;
    }
    // Remove it
    bindings.erase(iter);
    
    // Execute script to remove it
    return eval(
        u8string::format(R"(
            %s = function() { }
        )", u8string(name).c_str())
    );
}
bool MSWebView2::reload() {
    if (!IsReady()) {
        return false;
    }
    return SUCCEEDED(webview->Reload());
}
bool MSWebView2::stop() {
    if (!IsReady()) {
        return false;
    }
    return SUCCEEDED(webview->Stop());
}
u8string MSWebView2::url() {
    if (!IsReady()) {
        return u8string();
    }
    ComWString source;
    if (FAILED(webview->get_Source(&source))) {
        return u8string();
    }
    return source.to_utf8();
}
WebInterop *MSWebView2::interop() {
    return this;
}

// WebInterop
pointer_t MSWebView2::add_request_watcher(const WebRequestWatcher &watcher) {
    if (!IsReady()) {
        return nullptr;
    }
    if (!watcher) {
        return nullptr;
    }
    // Register filter the doc sad
    if (!filter_all) {
        filter_all = TRUE;
        webview->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
    }
    EventRegistrationToken token;
    auto hr = webview->add_WebResourceRequested(
        Callback<ICoreWebView2WebResourceRequestedEventHandler>(
            [watcher](ICoreWebView2 *sender, ICoreWebView2WebResourceRequestedEventArgs *args) -> HRESULT {
                // Get URL here
                ComPtr<ICoreWebView2WebResourceRequest> request;
                LPWSTR wurl;
                args->get_Request(&request);
                request->get_Uri(&wurl);

                auto url = u8string::from(wurl);
                CoTaskMemFree(wurl);

                watcher(url);
                return S_OK;
            }
        ).Get(), &token
    );
    if (FAILED(hr)) {
        return nullptr;
    }
    static_assert(sizeof(EventRegistrationToken) <= sizeof(pointer_t));
    return reinterpret_cast<pointer_t&>(token);
}

// MSWebView2Binding
MSWebView2Binding::MSWebView2Binding(ICoreWebView2* webview, const WebFunction &fn) : webview(webview), func(fn) {

}
MSWebView2Binding::~MSWebView2Binding() {
    // ID should not be empty, if not called
    if (!id) {
        return;
    }
    auto hr = webview->RemoveScriptToExecuteOnDocumentCreated(id);
    BTK_ASSERT(SUCCEEDED(hr));
}
ULONG MSWebView2Binding::AddRef() {
    return ++refcount;
}
ULONG MSWebView2Binding::Release() {
    if (refcount > 1) {
        return --refcount;
    }
    delete this;
    return 0;
}
HRESULT MSWebView2Binding::QueryInterface(REFIID id, void **pobj) {
    if (id == __uuidof(ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler)) {
        AddRef();
        *pobj = static_cast<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler*>(this);
        return S_OK;
    }
    return E_NOINTERFACE;
}
HRESULT MSWebView2Binding::Invoke(HRESULT errorCode, LPCWSTR i) {
    if (FAILED(errorCode)) {
        return errorCode;
    }
    id = i;
    return S_OK;
}
void    MSWebView2Binding::Invoke(std::wstring_view object) {
    BTK_ASSERT(func != nullptr);
    func(u8string::from(object));
}

BTK_PRIV_END

BTK_NS_BEGIN

class WebViewImpl final : public MSWebView2 {
    public:
        void place_to(Widget *w);

        bool try_inited = false;
        bool init_failed = false;
        bool webview_has_focus = false;
        bool except_lost_focus = false; //< Lost focus from Window System
        // FIXME : Maybe we have a better choice
};

WebView::WebView(Widget *parent) : Widget(parent) {
    set_focus_policy(FocusPolicy::Mouse);

    priv = new WebViewImpl;
    priv->AddRef();

    priv->signal_ready().connect([this]() {
        priv->place_to(this);
    });
    priv->signal_got_focus().connect([this]() {
        BTK_LOG(BTK_BLUE("[MSWebView2] ") "Got focus\n");
        priv->webview_has_focus = true;
        // Because When Win32 child window take the focus
        // The Parent window will lost the focus, Maybe we will got focus lost
        priv->except_lost_focus = true;

        // Let other widget lost focus
        take_focus();
    });
    priv->signal_lost_focus().connect([this]() {
        BTK_LOG(BTK_BLUE("[MSWebView2] ") "Lost focus\n");
        priv->webview_has_focus = false;
    });
}
WebView::~WebView() {
    priv->Release();
}

bool WebView::paint_event(PaintEvent &) {
    if (priv->try_inited || priv->init_failed) {
        return true;
    }
    // Try Get hwnd
    auto win = root()->winhandle();
    HWND hwnd;
    if (!win->query_value(AbstractWindow::Hwnd, &hwnd)) {
        priv->init_failed = false;
        return true;
    }
    if (FAILED(priv->Initialize(hwnd))) {
        BTK_LOG(BTK_BLUE("[MSWebView2] ") "Initialize failed\n");

#if    !defined(NDEBUG)
        ::MessageBoxA(NULL, "WebView2 Initialize failed", "Error", MB_ICONERROR);
#endif

        priv->init_failed = false;
        return true;
    }
    priv->try_inited = true;
    return true;
}
bool WebView::resize_event(ResizeEvent &) {
    if (priv->IsReady()) {
        priv->place_to(this);
    }
    return true;
}
bool WebView::move_event(MoveEvent &) {
    if (priv->IsReady()) {
        priv->place_to(this);
    }
    return true;
}
bool WebView::change_event(ChangeEvent &) {
    return true;
}
bool WebView::focus_gained(FocusEvent &) {
    // BTK_LOG("[WebView2] focus_gained\n");
    return true;
}
bool WebView::focus_lost(FocusEvent &) {
    // BTK_LOG("[WebView2] focus_lost\n");
    if (!priv->webview_has_focus) {
        // WebView has the focus, so that we should release the focus
        return true;
    }
    if (priv->except_lost_focus) {
        // Lost focus from Parent window lost focus
        priv->except_lost_focus = false;
        // Retake focus
        defer_call(&Widget::take_focus, this);
        return true;
    }

    HWND hwnd;
    auto win = root()->winhandle();
    if (!win->query_value(AbstractWindow::Hwnd, &hwnd)) {
        BTK_LOG("[WebView2] Window unsupported hwnd\n");
        return true;
    }
    // Set focus to parent window
    ::SetFocus(hwnd);
    return true;
}

void WebViewImpl::place_to(Widget *w) {
    // Map to pixel
    auto win = w->root()->winhandle();
    auto r = w->rect();

    Point top_left = win->map_point(r.top_left(), AbstractWindow::ToPixel);
    Point bottom_right = win->map_point(r.bottom_right(), AbstractWindow::ToPixel);

    RECT rect;
    rect.top = top_left.y;
    rect.left = top_left.x;
    rect.right = bottom_right.x;
    rect.bottom = bottom_right.y;

    PutBounds(rect);
}
WebInterface *WebView::interface() const {
    return priv;
}
WebInterface *WebView::AllocHeadless() {
    auto f = std::make_unique<MSWebView2>();
    if (SUCCEEDED(f->InitializeHeadless())) {
        return f.release();
    }
    return nullptr;
}
BTK_NS_END