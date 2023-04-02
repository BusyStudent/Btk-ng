#include <Btk/plugins/webview.hpp>
#include <Btk/widgets/textedit.hpp>
#include <Btk/widgets/view.hpp>
#include <Btk/context.hpp>
#include <Btk/layout.hpp>
#include <iostream>

using namespace BTK_NAMESPACE;

int main() {
    UIContext ctxt;
    Widget root;
    BoxLayout layout(TopToBottom);
    layout.attach(&root);
    
    WebView *wv = new WebView();
    TextEdit *edit = new TextEdit();
    layout.add_widget(edit);
    layout.add_widget(wv, 1);

    auto inf = wv->interface();

    ListBox lbox;

    lbox.resize(500, 600);
    root.resize(500, 600);

    lbox.show();

    inf->signal_ready().connect([&]() {
        inf->navigate(R"(https://www.github.com/)");
        inf->bind("set_window_title", [&](u8string_view args) {
            wv->root()->set_window_title(args);
        });
        inf->inject(R"(
            function my_inject() {
                set_window_title("HelloWorld");
            }
        )");
        inf->signal_url_changed().connect([&](u8string_view url) {
            edit->set_text(url);
        });
        inf->signal_title_changed().connect([&](u8string_view url) {
            wv->root()->set_window_title(url);
        });
        edit->signal_enter_pressed().connect([&]() {
            inf->navigate(edit->text());
        });
        inf->interop()->add_request_watcher([&](u8string_view url) {
            lbox.add_item(ListItem(url));
        });
        inf->signal_navigation_starting().connect([&]() {
            wv->root()->set_window_title("Starting");
            lbox.clear();
        });
        inf->signal_navigation_compeleted().connect([&]() {
            wv->root()->set_window_title("Compeleted");
        });
    });

    root.show();
    return ctxt.run();
}