#pragma once

#include <Btk/defs.hpp>
#include <functional>
#include <map>

BTK_NS_BEGIN

class UIXmlBuilderImpl;
class UIXmlRuntimeImpl;

/**
 * @brief For dyn refelection & resource
 * 
 */
class BTKAPI UIXmlRuntime {

};
/**
 * @brief Build a widget tree from Xml file
 * 
 */
class BTKAPI UIXmlBuilder {
    public:
        UIXmlBuilder();
        ~UIXmlBuilder();

        bool    load(u8string_view xmlstring);
        Widget *build() const;
    private:
        UIXmlBuilderImpl *priv;
};


BTK_NS_END