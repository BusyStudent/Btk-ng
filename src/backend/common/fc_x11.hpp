#pragma once

#include "build.hpp"

#include <fontconfig/fontconfig.h>

BTK_NS_BEGIN2()

using namespace BTK_NAMESPACE;

class FcFont {
    public:
        u8string name;
        u8string file;
        uint32_t index;
};

class FcContext {
    public:
        FcContext();
        ~FcContext();

        FcFont     match(const char_t *pattern);
        StringList list(); 
    private:
        FcConfig *config;
};

FcContext::FcContext() {
    FcInit();
    config = FcInitLoadConfigAndFonts();
}
FcContext::~FcContext() {
    FcConfigDestroy(config);
    FcFini();
}

FcFont FcContext::match(const char_t *pattern) {
    if (pattern == nullptr) {
        pattern = reinterpret_cast<const char_t *>("");
    }
    FcFont font;
    FcPattern *pat = FcNameParse(reinterpret_cast<const FcChar8 *>(pattern));
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    FcResult result;
    FcPattern *match = FcFontMatch(config, pat, &result);
    FcChar8 *file = nullptr;
    FcPatternGetString(match, FC_FILE, 0, &file);
    int index = 0;
    FcPatternGetInteger(match, FC_INDEX, 0, &index);
    FcChar8 *name;
    FcPatternGetString(match, FC_FULLNAME, 0, &name);
    font.name = reinterpret_cast<const char_t *>(name);
    font.file = reinterpret_cast<const char_t *>(file);
    font.index = index;
    FcPatternDestroy(match);
    FcPatternDestroy(pat);
    return font;
}


BTK_NS_END2()