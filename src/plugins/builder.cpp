#include "build.hpp"

#include <Btk/widgets/textedit.hpp>
#include <Btk/widgets/button.hpp>
#include <Btk/widgets/view.hpp>
#include <Btk/widgets/slider.hpp>
#include <Btk/plugins/builder.hpp>
#include <Btk/widget.hpp>
#include <Btk/layout.hpp>
#include <tinyxml2.h>
#include <algorithm>
#include <optional>
#include <sstream>
#include <stack>

#if defined(_MSC_VER)
#define UIXmlStrcasecmp _stricmp
#else
#define UIXmlStrcasecmp strcasecmp
#endif


BTK_PRIV_BEGIN

// It repensent the widget class exposed by it
class UIXmlClassHandler : public std::enable_shared_from_this<UIXmlClassHandler> {
    public:
        using AddChild = std::function<void(Widget *parent, Widget *self, tinyxml2::XMLElement*)>;
        using Handler = std::function<void(Widget *widget, const char *attrs, const char *value)>;
        using Create  = std::function<Widget*()>;

        std::shared_ptr<UIXmlClassHandler> parent;
        std::map<u8string, Handler>        handlers_map;
        Create                             create_fn;
        AddChild                           add_child_fn;

        bool handle(Widget *widget, const char *attrs, const char *value) {
            auto iter = handlers_map.find(attrs);
            if (iter != handlers_map.end()) {
                // Find it
                iter->second(widget, attrs, value);
                return true;
            }
            if (parent != nullptr) {
                return parent->handle(widget, attrs, value);
            }
            // Eof of chain
            return false;
        }
        void add_child(Widget *parent, Widget *self, tinyxml2::XMLElement *node) {
            if (add_child_fn) {
                return add_child_fn(parent, self, node);
            }
            return parent->add_child(self);
        }
        Widget *create() {
            if (create_fn) {
                return create_fn();
            }
            return nullptr;
        }
};

// Parser
template <typename T>
class UIXmlParser;
template <>
class UIXmlParser<Rect> {
    public:
        std::optional<Rect> parse(const char *str) {
            int x, y, w, h;
            if (sscanf(str, "%d,%d,%d,%d", &x, &y, &w, &h) == 4) {
                return Rect{x, y, w, h};
            }
            return std::nullopt;
        }
};
template <>
class UIXmlParser<FRect> {
    public:
        std::optional<FRect> parse(const char *str) {
            float x, y, w, h;
            if (sscanf(str, "%f,%f,%f,%f", &x, &y, &w, &h) == 4) {
                return FRect{x, y, w, h};
            }
            return std::nullopt;
        }
};
template <>
class UIXmlParser<Size> {
    public:
        std::optional<Size> parse(const char *str) {
            int w, h;
            if (sscanf(str, "%d,%d", &w, &h) == 2) {
                return Size{w, h};
            }
            return std::nullopt;
        }
};
template <>
class UIXmlParser<FSize> {
    public:
        std::optional<FSize> parse(const char *str) {
            float w, h;
            if (sscanf(str, "%f,%f", &w, &h) == 2) {
                return FSize{w, h};
            }
            return std::nullopt;
        }
};
template <>
class UIXmlParser<Point> {
    public:
        std::optional<Point> parse(const char *str) {
            int w, h;
            if (sscanf(str, "%d,%d", &w, &h) == 2) {
                return Point{w, h};
            }
            return std::nullopt;
        }
};
template <>
class UIXmlParser<FPoint> {
    public:
        std::optional<FPoint> parse(const char *str) {
            float w, h;
            if (sscanf(str, "%f,%f", &w, &h) == 2) {
                return FPoint{w, h};
            }
            return std::nullopt;
        }
};
template <>
class UIXmlParser<Margin> {
    public:
        std::optional<Margin> parse(const char *str) {
            int l, t, r, b;
            if (sscanf(str, "%d,%d,%d,%d", &l, &t, &r, &b) == 4) {
                return Margin{l, t, r, b};
            }
            else if (sscanf(str, "%d", &l) == 1) {
                return Margin{l, l, l, l};
            }
            return std::nullopt;
        }
};

template <>
class UIXmlParser<u8string> {
    public:
        std::optional<u8string> parse(const char *str) {
            return str;
        }
};
template <>
class UIXmlParser<u8string_view> {
    public:
        std::optional<u8string_view> parse(const char *str) {
            return str;
        }
};
template <>
class UIXmlParser<int> {
    public:
        std::optional<int> parse(const char *str) {
            int value;
            if (sscanf(str, "%d", &value) == 1) {
                return value;
            }
            return std::nullopt;
        }
};
template <>
class UIXmlParser<int64_t> {
    public:
        std::optional<int64_t> parse(const char *str) {
            int value;
            if (sscanf(str, "%ld", &value) == 1) {
                return value;
            }
            return std::nullopt;
        }
};
template <>
class UIXmlParser<bool> {
    public:
        std::optional<bool> parse(const char *str) {
            // Default in true
            if (!str) {
                return true;
            }
            if (strcmp(str, "true") == 0) {
                return true;
            }
            else if (strcmp(str, "false") == 0) {
                return false;
            }
            return std::nullopt;
        }
};
template <>
class UIXmlParser<float> {
    public:
        std::optional<float> parse(const char *str) {
            float value;
            if (sscanf(str, "%f", &value) == 1) {
                return value;
            }
            return std::nullopt;
        }
};
template <>
class UIXmlParser<double> {
    public:
        std::optional<double> parse(const char *str) {
            double value;
            if (sscanf(str, "%lf", &value) == 1) {
                return value;
            }
            return std::nullopt;
        }
};
template <>
class UIXmlParser<PixBuffer> {
    public:
        std::optional<PixBuffer> parse(const char *str) {
            return PixBuffer::FromFile(str);
        }
};
template <>
class UIXmlParser<Direction> {
    public:
        std::optional<Direction> parse(const char *str) {
            std::string s(str);

            char *left = s.data();
            char *right = strstr(left, ",");
            if (right == nullptr) {
                return std::nullopt;
            }
            *right = '\0';
            right++;
            while (*right == ' ') {
                right++;
            }
            if (UIXmlStrcasecmp(left, "Left") == 0 && UIXmlStrcasecmp(right, "Right") == 0) {
                return Direction::LeftToRight;
            }
            else if (UIXmlStrcasecmp(left, "Right") == 0 && UIXmlStrcasecmp(right, "Left") == 0) {
                return Direction::RightToLeft;
            }
            else if (UIXmlStrcasecmp(left, "Top") == 0 && UIXmlStrcasecmp(right, "Bottom") == 0) {
                return Direction::TopToBottom;
            }
            else if (UIXmlStrcasecmp(left, "Bottom") == 0 && UIXmlStrcasecmp(right, "Top") == 0) {
                return Direction::BottomToTop;
            }
            else {
                return std::nullopt;
            }
        }
};
template <>
class UIXmlParser<Alignment> {
    public:
        std::optional<Alignment> parse(const char *str) {
            Alignment value = Alignment{ };
            std::string s(str);
            std::transform(s.begin(), s.end(), s.begin(), std::tolower);

            std::istringstream iss(s);
            std::string token;
            while (std::getline(iss, token, ',')) {
                if (token == "left") {
                    value |= Alignment::Left;
                }
                else if (token == "right") {
                    value |= Alignment::Right;
                }
                else if (token == "center") {
                    value |= Alignment::Center;
                }
                else if (token == "top") {
                    value |= Alignment::Top;
                }
                else if (token == "bottom") {
                    value |= Alignment::Bottom;
                }
                else if (token == "middle") {
                    value |= Alignment::Middle;
                }
                else if (token == "baseline") {
                    value |= Alignment::Baseline;
                }
            }
            return value;
        }
};
template <>
class UIXmlParser<FocusPolicy> {
    public:
        std::optional<FocusPolicy> parse(const char *str) {
            std::string s(str);
            std::transform(s.begin(), s.end(), s.begin(), std::tolower);

            if (s == "mouse") {
                return FocusPolicy::Mouse;
            }
            return std::nullopt;
        }
};
template <>
class UIXmlParser<Font> {
    public:
        std::optional<Font> parse(const char *str) {
            std::string s(str);

            std::istringstream iss(s);
            std::string token;
            std::string font_name;
            float font_size = 12.0f;
            while (std::getline(iss, token, ' ')) {
                if (token == "") {
                    continue;
                }
                if (isdigit(token[0])) {
                    font_size = std::stof(token);
                }
                else {
                    font_name += token;
                }
            }
            return Font(font_name, font_size);
        }
};
template <>
class UIXmlParser<Color> {
    public:
        std::optional<Color> parse(const char *str) {
            return Color(str);
        }
};

template <typename T>
class UIXmlRegister {
    public:
        UIXmlRegister(UIXmlClassHandler *h) : handler(h) { };

        // Bind normal callable
        template <typename Callable>
        UIXmlRegister bind(const char *attr, Callable &&callable) {
            handler->handlers_map[attr] = [callable](Widget *wi, const char *attrs, const char *value) {
                callable(GetTarget(wi), attrs, value);
            };
            return *this;
        }
        // Bind method
        template <typename RetT, typename Input>
        UIXmlRegister bind(const char *attr, RetT (T::*fn)(Input i)) {
            handler->handlers_map[attr] = [fn](Widget *wi, const char *attrs, const char *value) {
                using DInput = std::decay_t<Input>;
                auto self = GetTarget(wi);
                auto arg = UIXmlParser<DInput>().parse(value).value();

                std::invoke(fn, self, arg);
            };
            return *this;
        }
        template <typename RetT>
        UIXmlRegister bind(const char *attr, RetT (T::*fn)()) {
            handler->handlers_map[attr] = [fn](Widget *wi, const char *attrs, const char *value) {
                auto self = GetTarget(wi);

                std::invoke(fn, self);
            };
            return *this;
        }

        UIXmlClassHandler *operator ->() {
            return handler;
        }
    private:
        template <typename Input>
        static T *GetTarget(Input *wi) {
            return  &dynamic_cast<T&>(*wi);
        }

        UIXmlClassHandler *handler;
};

class UIXmlBuilderInternal {
    public:
        tinyxml2::XMLDocument doc;
        std::map<u8string, std::shared_ptr<UIXmlClassHandler>> handlers; //< Handle class
        std::stack<std::pair<Widget *, UIXmlClassHandler *>> widgets; //< Current elems parents widget and handles

        void walk_xml_tree(tinyxml2::XMLElement *elem, int depth);

        // Handler
        UIXmlClassHandler *register_class(const char *baseclass, const char *selfclass, const UIXmlClassHandler::Create &);
        template <typename T>
        UIXmlRegister<T>   register_class(const char *baseclass, const char *selfclass);
        void register_all();
};


void UIXmlBuilderInternal::walk_xml_tree(tinyxml2::XMLElement *elem, int depth) {
    if (!elem) {
        return;
    }
    BTK_LOG("%*sElement Name: %s\n", depth * 4, "", elem->Name());
    // Find the widget
    auto witer = handlers.find(elem->Name());
    if (witer == handlers.end()) {
        // Bad
    }
    auto wclass = witer->second.get();
    // Try Create the widget
    Widget *widget = wclass->create();
    if (!widget) {
        // Bad
    }
    // Set is parent if
    if (!widgets.empty()) {
        auto [parent, parent_class] = widgets.top();
        parent_class->add_child(parent, widget, elem);
    }
    // Push it
    widgets.push(std::make_pair(widget, wclass));

    for (const tinyxml2::XMLAttribute *attr = elem->FirstAttribute(); attr != nullptr; attr = attr->Next()) {
        BTK_LOG("%*sAttribute Name: %s, Attribute Value: %s\n", depth * 4, "", attr->Name(), attr->Value());
        bool ret = wclass->handle(widget, attr->Name(), attr->Value());
        if (!ret) {
            BTK_LOG("%*sUnHandled Attribute\n", depth * 4, "");
        }
    }
    if (elem->GetText() != nullptr) {
        BTK_LOG("%*sElement Content: %s\n", depth * 4, "", elem->GetText());
        // Try apply
        bool ret = wclass->handle(widget, "<text>", elem->GetText());
        if (!ret) {
            BTK_LOG("%*sUnHandled Attribute\n", depth * 4, "");
        }
    }
    for (tinyxml2::XMLElement *child = elem->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
        walk_xml_tree(child, depth + 1);
    }

    // Leave
    if (widgets.size() != 1) {
        widgets.pop();
    }
}

UIXmlClassHandler *UIXmlBuilderInternal::register_class(const char *baseclass, const char *selfclass, const UIXmlClassHandler::Create &crt) {
    auto it = handlers.find(selfclass);
    if (it == handlers.end()) {
        // Not found
        auto ptr = std::make_shared<UIXmlClassHandler>();
        ptr->create_fn = crt;
        handlers[selfclass] = ptr;
        if (baseclass != nullptr) {
            // Check baseclass
            auto baseit = handlers.find(baseclass);
            if (baseit == handlers.end()) {
                BTK_LOG("Base class %s not found\n", baseclass);
                abort();
            }
            else {
                ptr->parent = baseit->second;
            }
        }
        return ptr.get();
    }
    else {
        // Found
        // Error
        BTK_LOG("Error Duplicate name\n");
        abort();
    }
    return nullptr;
}
template <typename T>
UIXmlRegister<T> UIXmlBuilderInternal::register_class(const char *baseclass, const char *selfclass) {
    if constexpr(std::is_abstract_v<T>) {
        return register_class(baseclass, selfclass, nullptr);
    }
    else {
        return register_class(baseclass, selfclass, []() -> Widget* { return new T(); });
    }
}
void UIXmlBuilderInternal::register_all() {
    register_class<Widget>(nullptr, "Widget")
        .bind("rect", &Widget::set_rect)
        .bind("size", &Widget::resize)
        .bind("position", &Widget::move)
        .bind("visible", &Widget::set_visible)
        .bind("hide", &Widget::hide)
        .bind("focus", &Widget::set_focus_policy)
        .bind("opacity", &Widget::set_opacity)
        .bind("font", &Widget::set_font)
        .bind("name", &Widget::set_name)

        // Attrs
        .bind("delete-on-close", [](Widget *wi, const char *attr, const char *value) {
            wi->set_attribute(WidgetAttrs::DeleteOnClose, UIXmlParser<bool>().parse(value).value());
        })
        .bind("background-transparent", [](Widget *wi, const char *attr, const char *value) {
            wi->set_attribute(WidgetAttrs::BackgroundTransparent, UIXmlParser<bool>().parse(value).value());
        })
        .bind("debug", [](Widget *wi, const char *attr, const char *value) {
            wi->set_attribute(WidgetAttrs::Debug, UIXmlParser<bool>().parse(value).value());
        })
    ;

    // Button
    register_class<AbstractButton>("Widget", "AbstractButton")
        .bind("text", &AbstractButton::set_text)
        .bind("flat", &AbstractButton::set_flat)
        .bind("icon", &AbstractButton::set_icon)
        .bind("<text>", &AbstractButton::set_text)
    ;
    register_class<Button>("AbstractButton", "Button");
    register_class<RadioButton>("AbstractButton", "RadioButton");

    // Views
    register_class<Label>("Widget", "Label")
        .bind("text", &Label::set_text)
        .bind("align", &Label::set_text_align)
        .bind("<text>", &Label::set_text)
    ;
    register_class<ProgressBar>("Widget", "ProgressBar")
        .bind("value", &ProgressBar::set_value)
        .bind("text-visible", &ProgressBar::set_text_visible)
        .bind("direction", &ProgressBar::set_direction)
        .bind("range", [](ProgressBar *self, const char *attr, const char *value) {
            auto [min, max] = UIXmlParser<Size>().parse(value).value();
            self->set_range(min, max);
        })
    ;
    register_class<ImageView>("Widget", "ImageView")
        .bind("image", [](ImageView *self, const char *attr, const char *value) {
            self->set_image(Image::FromFile(value));
        })
        .bind("keep-aspect-ratio", &ImageView::set_keep_aspect_ratio)
    ;
    register_class<ListBox>("Widget", "ListBox");

    // Slider
    register_class<AbstractSlider>("Widget", "AbstractSlider")
        .bind("value", &AbstractSlider::set_value)
        .bind("page-step", &AbstractSlider::set_page_step)
        .bind("single-step", &AbstractSlider::set_single_step)
        .bind("range", [](AbstractSlider *self, const char *attr, const char *value) {
            auto [min, max] = UIXmlParser<Size>().parse(value).value();
            self->set_range(min, max);
        })
    ;
    register_class<ScrollBar>("AbstractSlider", "ScrollBar");
    register_class<Slider>("AbstractSlider", "Slider");

    // Edit
    register_class<TextEdit>("Widget", "TextEdit")
        .bind("text", &TextEdit::set_text)
        .bind("placeholder", &TextEdit::set_placeholder)
        .bind("<text>", &TextEdit::set_text)
    ;

    // Layout
    class BxLayout : public Widget {
        public:
            BxLayout() {
                set_focus_policy(FocusPolicy::Mouse);
                layout.attach(this);
            }
            void set_direction(Direction d) {
                layout.set_direction(d);
            }
            void set_spacing(int s) {
                layout.set_spacing(s);
            }
            void set_margin(Margin m) {
                layout.set_margin(m);
            }
            Size size_hint() const {
                return layout.size_hint();
            }
            BoxLayout layout;
    };
    register_class<BxLayout>("Widget", "BoxLayout")
        .bind("direction", &BxLayout::set_direction)
        .bind("spacing", &BxLayout::set_spacing)
        .bind("margin", &BxLayout::set_margin)

    // Etc
        ->add_child_fn = [](Widget *parent, Widget *child, tinyxml2::XMLElement *childnode) {
        // Try get attribute
        int stretch = 0;
        Alignment align {};
        auto self = dynamic_cast<BxLayout*>(parent);
        
        auto value = childnode->Attribute("layout:stretch");
        if (value) {
            stretch = UIXmlParser<int>().parse(value).value();
            // Avoid child handle it
            childnode->DeleteAttribute("layout:stretch");
        }

        value = childnode->Attribute("layout:align");
        if (value) {
            align = UIXmlParser<Alignment>().parse(value).value();
            // Avoid child handle it
            childnode->DeleteAttribute("layout:align");
        }

        // Add it
        self->layout.add_widget(child, stretch, align);
    };
}

BTK_PRIV_END

BTK_NS_BEGIN

class UIXmlBuilderImpl final : public UIXmlBuilderInternal { };

UIXmlBuilder::UIXmlBuilder() {
    priv = new UIXmlBuilderImpl;
    priv->register_all();
}
UIXmlBuilder::~UIXmlBuilder() {
    delete priv;
}
bool    UIXmlBuilder::load(u8string_view view) {
    return (priv->doc.Parse(view.data(), view.size()) == 0);
}
Widget *UIXmlBuilder::build() const {
    // Root widget
    auto root = priv->doc.RootElement();
    if (!root) {
        return nullptr;
    }
    // Root widget
    try {
        priv->walk_xml_tree(root, 0);
    }
    catch (...) {
        BTK_LOG("Create failed");
        // Release the stack
        while (priv->widgets.size() != 1) {
            priv->widgets.pop();
        }
        if (priv->widgets.size() == 1) {
            delete priv->widgets.top().first;
            priv->widgets.pop();
        }
        
    }
    if (priv->widgets.empty()) {
        BTK_LOG("Create failed");
        return nullptr;
    }
    auto result = priv->widgets.top();
    priv->widgets.pop();
    return result.first;
}

BTK_NS_END