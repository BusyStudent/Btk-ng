# Btk

Fast, lightweight and easy to use UIToolkit for building user interfaces.  

It still at early developing.  

## Example

Create a button on window

```cpp
#include <Btk/context.hpp> //< For UIContext
#include <Btk/comctl.hpp> //< Base widget & comctl

using namespace BTK_NAMESPACE; //< The namespace is configurable, default in Btk

class App : public Widget {
    public:
        App() {
            btn.signal_clicked().connect([this, n = 0]() mutable {
                n += 1;
                if (n == 5) {
                    this->close();
                }
            });

            // Or add widget like this
            auto label = new Label("HelloWorld");
            add_child(label);
        }
    private:
        // Add some widget on the root
        Button btn{this};
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
    ctxt.run();
}


```

## Features

- Utf8 support
- Cross platform
- Template based signal system

## Add this packages

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
| TextEdit      | Half            | Single line, need multi support |
| Slider        |                 |                          |
| ScrollBar     |                 |                          |
| ProgressBar   |                 |                          |
| Label         |                 |                          |
| ImageView     |                 | Can play gif             |
| PopupWidget   |                 |                          |
| FileDialog    |                 |                          |
| MenuBar       | Not yet         |                          |
| GLWidget      |                 | For use OpenGL           |
| VideoWidget   |                 | Video output for MediaPlayer (multimedia plugin) |

## Existing bugs



## Todo List

- [ ] Compelete PainterEffect, refactorying it
- [ ] Compelete Nanovg Painter
- [ ] Improve Style

## Install

- Linux 

```sh
$ apt install libcairo2-dev libpango1.0-dev libsdl2-dev
```