#pragma once

#include <Btk/comctl.hpp>
#include <functional>
#include <map>

BTK_NS_BEGIN

using UIBuilderProps = std::unordered_map<u8string, u8string>;

class UIBuilderItem : public Any {
    public:
        UIBuilderItem(u8string_view s) : _super(s) {}

        virtual bool add_child(Widget *w, const UIBuilderProps &props) {
            
        }
        virtual bool proc_pops() {
            
        }

        template <typename Callable>
        void def(u8string_view prop, Callable &&setter) {

        }
    private:
        std::map<u8string, std::function<void(Widget *, u8string_view value)>> setters;
        u8string _super;
};

class UIBuilder {
    public:
        UIBuilder();
        ~UIBuilder();

        auto build(u8string_view xmldoc) -> Widget *;
        void register_widget(UIBuilderItem *ctl, u8string_view name);
    private:
        std::map<u8string, std::unique_ptr<UIBuilderItem>> ctrls;
};

BTK_NS_END