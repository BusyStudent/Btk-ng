# Btk

Fast and easy to use UIToolkit for building user interfaces.  

It still at early developing.  

## Example

```cpp
#include <Btk/context.hpp> //< For UIContext
#include <Btk/widget.hpp> //< Base widget & comctl

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

## Existing bugs

- [ ] Crash when using Direct2d with SDL2 backend  
- [ ] Win32 backend window frozen at first time user call fullscreen

## Todo List

- [ ] Compelete PainterEffect, refactorying it
- [ ] Compelete Nanovg Painter
- [ ] Compelete CMake build file
- [ ] Improve Style
