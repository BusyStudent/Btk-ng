#pragma once

// Basic macro definitions
#ifndef BTK_NAMESPACE
#define BTK_NAMESPACE Btk
#endif

#define BTK_NS_BEGIN namespace BTK_NAMESPACE {
#define BTK_NS_END   }

#define BTK_NS_BEGIN2(ns) namespace ns {
#define BTK_NS_END2(ns)   }

#define BTK_PRIV_BEGIN namespace BTK_NAMESPACE { \
                        namespace {
#define BTK_PRIV_END    } \
                       }

// Compiler specific macros
#if   defined(_MSC_VER)
#define BTK_ATTRIBUTE(...) __declspec(__VA_ARGS__)
#elif defined(__GNUC__)
#define BTK_ATTRIBUTE(...) __attribute__((__VA_ARGS__))
#else
#define BTK_ATTRIBUTE(...)
#endif

#if   defined(_WIN32)
#define BTK_DLLEXPORT dllexport
#define BTK_DLLIMPORT dllimport
#elif defined(__linux__)
#define BTK_DLLEXPORT visibility("default")
#define BTK_DLLIMPORT 
#else
#define BTK_DLLEXPORT
#define BTK_DLLIMPORT
#endif

// Source check
#if     defined(_BTK_SOURCE) && !defined(_BTK_STATIC)
#define BTKAPI BTK_ATTRIBUTE(BTK_DLLEXPORT)
#else
#define BTKAPI // BTK_ATTRIBUTE(BTK_DLLIMPORT)
#endif

// Macro utilities
#define BTK_UNUSED(x) (void)(x)
#define BTK_STRINGIFY(x) #x
#define BTK_STRINGIFY_EXPAND(x) BTK_STRINGIFY(x)
#define BTK_NOEXCEPT_IF(x) noexcept(noexcept(x))
#define BTK_ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))

// Exception macros
#if defined(BTK_NO_EXCEPTIONS)
#define BTK_THROW(...) do { ::abort(); } while (0)
#define BTK_CATCH(...) if constexpr(0)
#define BTK_NOEXCEPT noexcept(true)
#define BTK_TRY      if constexpr(1)
#else
#define BTK_THROW(...) throw (__VA_ARGS__)
#define BTK_CATCH(...) catch (__VA_ARGS__)
#define BTK_NOEXCEPT noexcept(false)
#define BTK_TRY      try
#endif

// RTTI macros
#if defined(BTK_NO_RTTI)
#define BTK_CASTABLE(type, ptr) true
#define BTK_TYPEID(ref)         BTK_THROW(std::runtime_error("NO RTTI"))
#else
#define BTK_CASTABLE(type, ptr) dynamic_cast<type*>(ptr)
#define BTK_TYPEID(ref)         typeid(ref)
#endif

// Bultin functions for speed optimization
#if defined(__GNUC__)
#define Btk_memmove(dst, src, n)   __builtin_memmove(dst, src, n)
#define Btk_memcpy(dst, src, size) __builtin_memcpy(dst, src, size)
#define Btk_memset(dst, val, size) __builtin_memset(dst, val, size)
#define Btk_memcmp(a, b, size)     __builtin_memcmp(a, b, size)
#define Btk_memzero(dst, size)     __builtin_memset(dst, 0, size)
#define Btk_alloca(size)           __builtin_alloca(size)
#elif !defined(Btk_memmove) //< User already defined it
#define Btk_memmove(dst, src, n)   ::memmove(dst, src, n)
#define Btk_memcpy(dst, src, size) ::memcpy(dst, src, size)
#define Btk_memset(dst, val, size) ::memset(dst, val, size)
#define Btk_memcmp(a, b, size)     ::memcmp(a, b, size)
#define Btk_memzero(dst, size)     ::memset(dst, 0, size)
#endif

// Stack allocation, try platform
#if !defined(Btk_alloca) && defined(_WIN32)
#define Btk_alloca(size)           ::_alloca(size)
#endif

// Memory management macros for easy replacement
#if !defined(Btk_malloc)
#define Btk_realloc(ptr, size) ::realloc(ptr, size)
#define Btk_malloc(size)       ::malloc(size)
#define Btk_free(ptr)          ::free(ptr)
#endif

// Assume
#if   defined(__clang__)
#define Btk_assume(cond) __builtin_assume(cond)
#elif defined(_MSC_VER)
#define Btk_assume(cond) _assume(cond)
#else
#define Btk_assume(cond)
#endif

// Stack alloc type macros
#define Btk_stack_new(type)    new (Btk_alloca(sizeof(type))) type
#define Btk_stack_delete(p)    do {                                            \
                                    using _ptype = std::decay_t<decltype(*p)>; \
                                    if (p) {                                   \
                                        p->~_ptype();                          \
                                    }                                          \
                               } while (0)

// Assert / Debug macros
#define  BTK_ASSERT_MSG(cond, msg) assert(cond && (msg))
#define  BTK_ASSERT(cond) assert(cond)
#include <cassert>

#if   defined(_WIN32) && defined(_MSC_VER)
#define  BTK_BTRAKPOINT() __debugbreak()
#elif defined(_WIN32) && defined(__GNUC__)
#define  BTK_BTRAKPOINT() __asm__("int $3")
#elif defined(__linux__)
#define  BTK_BTRAKPOINT() signal(SIGTRAP)
#include <csignal>
#endif


// Debugging console output Support
#if  !defined(NDEBUG)
#define BTK_LOG(...)  ::printf(__VA_ARGS__);
#else
#define BTK_LOG(...)
#endif

// C++ Version
#define BTK_CXX20 (__cplusplus > 201703L)

// C++ Function name
#if   defined(__GNUC__)
#define BTK_FUNCTION __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define BTK_FUNCTION __FUNCSIG__
#else
#define BTK_FUNCTION __FUNCTION__
#endif


// Simple Once call 
#define BTK_ONCE(...)               \
    {                               \
        static bool __once = false; \
        if (!__once) {              \
            __once = true;          \
            __VA_ARGS__;            \
        }                           \
    }
// Simple Call on load
#define BTK_INITCALL(fn)                \
    static int _##__COUNTER__ = []() {  \
        fn();                           \
        return 0;                       \
    } ();                               \

// Macro for enum class to flag

#define BTK_ENUM_OPERATOR(en, base, op)                      \
    constexpr inline en operator op(en a1, en a2) noexcept { \
        return static_cast<en>(                              \
            static_cast<base>(a1) op static_cast<base>(a2)   \
        );                                                   \
    }
#define BTK_ENUM_OPERATOR1(en, base, op)                     \
    constexpr inline en operator op(en a) noexcept {         \
        return static_cast<en>(                              \
            op static_cast<base>(a)                          \
        );                                                   \
    }
#define BTK_ENUM_OPERATOR2(en, base, op)                        \
    constexpr inline en operator op##=(en &a1, en a2) noexcept{ \
        a1 = static_cast<en>(                                   \
            static_cast<base>(a1) op static_cast<base>(a2)      \
        );                                                      \
        return a1;                                              \
    }
#define BTK_ENUM_ALIAS(en, alias, op)                           \
    constexpr inline en operator alias(en a1, en a2) noexcept{  \
        return a1 op a2;                                        \
    }

// Make enum class flag

#define BTK_FLAGS_OPERATOR(en, base)   \
    BTK_ENUM_OPERATOR(en, base, &);    \
    BTK_ENUM_OPERATOR(en, base, |);    \
    BTK_ENUM_OPERATOR(en, base, ^);    \
    BTK_ENUM_OPERATOR2(en, base, &);   \
    BTK_ENUM_OPERATOR2(en, base, |);   \
    BTK_ENUM_OPERATOR2(en, base, ^);   \
    BTK_ENUM_OPERATOR1(en, base, ~);   \
    BTK_ENUM_ALIAS(en, +,  |);         \
    BTK_ENUM_ALIAS(en, +=, |=);

#define BTK_DECLARE_FLAGS(en) \
    BTK_FLAGS_OPERATOR(en, std::underlying_type_t<en>)