#pragma once

#include <Btk/defs.hpp>
#include <typeinfo>

#if 0
// #if !defined(BTK_NO_RTTI) && defined(__GNUC__)

BTK_NS_BEGIN

BTK_NS_END

#elif !defined(BTK_NO_RTTI)

namespace {
    template <typename T>
    const char *_Btk_typename(T &&v) {
        if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) {
            if (v) {
                return typeid(*v).name();
            }
            return "<Null, Unknown>";
        }
        else {
            return typeid(v).name();
        }
    }
}

#define Btk_typename _Btk_typename

#else
#define Btk_typename(x) "<No RTTI, Unknown>"
#endif
