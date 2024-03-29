#pragma once

#include <Btk/defs.hpp>
#include <type_traits>
#include <typeinfo>
#include <cstring>

#if !defined(BTK_NO_RTTI) && defined(__GNUC__)

// GCC typename
#include <cxxabi.h>
#include <string>

BTK_NS_BEGIN

namespace {
    template <typename T>
    std::string _Btk_typename(T &&v) {
        const char *raw;
        if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) {
            if (v) {
                raw = typeid(*v).name();
            }
            else {
                return "<Null, Unknown>";
            }
        }
        else {
            raw = typeid(v).name();
        }
        
        int status;
        char *name = ::abi::__cxa_demangle(raw, nullptr, nullptr, &status);
        if (name == nullptr) {
            return raw;
        }

        std::string ret;
        ret = name;
        ::free(name);
        return ret;
    }
}

#define Btk_typename(x) _Btk_typename(x).c_str()

BTK_NS_END

#elif !defined(BTK_NO_RTTI)

// MSVC typename

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
    inline
    const char *_Btk_typeinfo_name(const std::type_info &info) noexcept {
        const char struct_prefix[] = "struct ";
        const char class_prefix[]  = "class ";
        auto name = typeid(info).name();

        if (strncmp(name, struct_prefix, sizeof(struct_prefix) - 1) == 0) {
            name += sizeof(struct_prefix) - 1;
        }
        else if (strncmp(name, class_prefix, sizeof(class_prefix) - 1) == 0) {
            name += sizeof(class_prefix) - 1;
        }

        return name;
    }
}

#define Btk_typeinfo_name _Btk_typeinfo_name
#define Btk_typename      _Btk_typename

#else
#define Btk_typeinfo_name(x) "<No RTTI, Unknown>"
#define Btk_typename(x)      "<No RTTI, Unknown>"
#endif
