# Btk

![Logo](./logo/logo.svg)

Fast, lightweight and easy to use UIToolkit for building user interfaces. 

Written in C++17

Its goal is less dependency and mini

## CI

|Platform| CI Status|
| ---    | ---      |
|Linux   | CI TODO  |
|Windows |[![C/C++ CI on Windows](https://github.com/BusyStudent/Btk-ng/actions/workflows/window_build_test.yml/badge.svg)](https://github.com/BusyStudent/Btk-ng/actions/workflows/window_build_test.yml)|



### ScreenShoot

![ScreenShoot](https://btk-project.github.io/screenshoot.png)


## Example

Create a button on window

```cpp
#include <Btk/context.hpp> //< For UIContext
#include <Btk/comctl.hpp> //< Base widget & comctl

using namespace BTK_NAMESPACE; //< The namespace is configurable, default in Btk

class App : public Widget {
    public:
        App() {
            set_window_title("My Window");

            btn.set_text("Hello,click to close the window");
            btn.signal_clicked().connect(&App::on_button_clicked, this);
            // Also it can connect to lambda function or a callable
        }
        void on_button_clicked() {
            close();
        }
    private:
        // Add some widget on the root
        Button btn{this};
        // Or new like this
        // new Button(this);
};

int main() {

#if 0
    auto your_graphics_driver = new YourDriver();
    UIContext ctxt(your_graphics_driver);
#else
    UIContext ctxt;
#endif

    App app;
    app.show();

    return ctxt.run();
}


```

## Features

- Utf8 support
- Cross platform
- Template based signal system
- HI-DPI Support
- No external dependencies on windows

## Add this packages

In XMake  

``` lua
add_repositories("btk-project https://github.com/Btk-Project/xmake-repo")
add_requires("btk")
add_packages("btk")
```

## Painter backend

| Name     | Platform        | Description                             |
| ---      | ---             | ---                                     |
| Direct2D |  Windows        | Native painter on windows               |
| Cairo    |  X11 (Xlib)     | Unix painter                            |
| Nanovg   |  OpenGL         | Cross platform painter (imcompeleted)   |

## Font backend

| Name     | Platform        | Description                             |
| ---      | ---             | ---                                     |
| DWrite   |  Windows        | Native font backend on windows          |
| Freetype |  Any            | In Progress                             |


## Window system backend

| Name     | Platform        | Description              |
| ---      | ---             | ---                      |
| Win32    | Windows         |                          |
| SDL2     | Any             |                          |

## Widgets

| Name          | Done            | TODO / Description       |
| ---           | ---             | ---                      |
| Button        |                 |                          |
| RadioButton   |                 |                          |
| ComboBox      |                 |                          |
| TextEdit      | Half            | Single line, need multi support |
| Slider        |                 |                          |
| ScrollBar     |                 |                          |
| ProgressBar   |                 |                          |
| Label         |                 |                          |
| ImageView     |                 | Can play gif             |
| PopupWidget   |                 |                          |
| PopupMenu 	|                 |                          |
| FileDialog    |                 |                          |
| Menu          |                 |                          |
| MenuBar       |                 |                          |
| MainWindow    |                 |                          |
| TabWidget     |                 |                          |
| TabBar        |                 |                          |
| StackedWidget |                 |                          |
| GLWidget      |                 | For use OpenGL           |
| ListBox       |                 | Display string items     |
| VideoWidget   |                 | Video output for MediaPlayer (multimedia plugin) |
| WebView       |                 | CoreWebView2 on Win32 (webview plugin)           |

## Existing bugs

- [ ] IME disappearence when has Win32 child window (SDL backend)
- [ ] Focus does gained for once mouse click when has Win32 child window (SDL backend)
- [ ] Render Wrong on clip, maybe matrix or paint backend has bug?

## Todo List

- [ ] Compelete PainterEffect, refactorying it
- [ ] Compelete Nanovg Painter
- [ ] Improve Style
- [ ] Focus Conflict for System Window System & Logical Widget System (We used a dirty way to handle it)

## Install

- Linux

```sh
$ apt install libcairo2-dev libpango1.0-dev libsdl2-dev
```

## LICENSE  

MIT License
