#include "build.hpp"

#include <Btk/plugins/builder.hpp>
#include <tinyxml2.h>

BTK_NS_BEGIN

auto UIBuilder::build(u8string_view xmldoc) -> Widget* {
    tinyxml2::XMLDocument doc;
    if (doc.Parse(xmldoc.data(), xmldoc.size()) != 0) {
        // Failed
        return nullptr;
    }

    // For tree
    auto node = doc.RootElement();

    
}

BTK_NS_END

