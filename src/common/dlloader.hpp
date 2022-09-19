#pragma once

#include <cstdlib>

#if defined(_WIN32)
    #define BTK_DLOPEN  ::LoadLibraryA
    #define BTK_DLCLOSE ::FreeLibrary
    #define BTK_DLSYM   ::GetProcAddress
    #define BTK_LIBRARY HMODULE
    #include <windows.h>
    #undef min
    #undef max
#else
    #define BTK_DLOPEN(file)  ::dlopen(file, 0)
    #define BTK_DLCLOSE(h)    ::dlclose(h)
    #define BTK_DLSYM         ::dlsym
    #define BTK_LIBRARY        void *
    #include <dlfcn.h>
#endif

// Decl library
#define LIB_BEGIN(type, dllfile) \
namespace { \
    class type { \
        public: \
            ~type() { \
                BTK_DLCLOSE(__library);\
            }  \
        private: \
            [[noreturn, maybe_unused]] \
            inline bool __abort() { ::abort(); return false; }\
            BTK_LIBRARY __library = BTK_DLOPEN(dllfile);\
        public: \

// Import function
#define LIB_PROC_RAW(pfn, fn, dlname) \
    using __Fn_##fn = pfn;\
    __Fn_##fn fn    = reinterpret_cast<__Fn_##fn>(BTK_DLSYM(__library, dlname));
#define LIB_PROC4(pfn, fn) \
    LIB_PROC_RAW(pfn, fn, #fn)
#define LIB_PROC(fn) \
    LIB_PROC4(decltype(::fn)*, fn)

// Import var
#define LIB_VAR_RAW(pvar, var, dlname) \
    pvar & var           = *reinterpret_cast<pvar*>(BTK_DLSYM(__library, dlname);
#define LIB_VAR4(pvar, var) \
    LIB_VAR_RAW(pvar, var, #var);
#define LIB_VAR(var) \
    LIB_VAR4(decltype(::var), var)

// Helper for zero ahead
#define LIB_COND_BEGIN(type) \
    type() { 
#define LIB_COND_END(type) \
    }

// Run expression
#define LIB_EXECUTE_IF(cond, exp) \
    uint8_t __libunused ## __LINE__ = (cond) ? exp : false;
#define LIB_THROW_IF(cond, what) \
    LIB_EXECUTE_IF(cond, throw what)
#define LIB_ABORT_IF(cond) \
    LIB_EXECUTE_IF(cond, __abort())

// End decl library
#define LIB_END(type) \
    };\
}