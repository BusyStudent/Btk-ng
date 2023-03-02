#pragma once

#define _BTK_SOURCE

#include <Btk/detail/macro.hpp>
#include <Btk/detail/types.hpp>
#include <Btk/string.hpp>
#include <Btk/defs.hpp>

#if !defined(BTK_NO_EXCEPTIONS)
#include <stdexcept>
#endif

// Count leading zeros
#if   defined(__GNUC__)
#define Btk_countl_zero(x) x ? __builtin_clz(x) : 32
#elif defined(_MSC_VER)
#define Btk_countl_zero(x) x ? __lzcnt(x) : 32
#endif

// MinGW, update WINVER
#if   defined(_WIN32) && defined(__GNUC__)
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#define WINVER       _WIN32_WINNT_WIN10
#endif

// Debug color macro
#define BTK_CCOLOR_END "\033[0m"
#define BTK_RED(...) "\033[31m" __VA_ARGS__ "\033[41m"   BTK_CCOLOR_END
#define BTK_BLACK(...) "\033[30m" __VA_ARGS__ "\033[40m" BTK_CCOLOR_END
#define BTK_GREEN(...) "\033[32m" __VA_ARGS__ "\033[42m" BTK_CCOLOR_END
#define BTK_YELLOW(...) "\033[33m" __VA_ARGS__ "\033[43m" BTK_CCOLOR_END
#define BTK_BLUE(...) "\033[34m" __VA_ARGS__ "\033[44m" BTK_CCOLOR_END
#define BTK_MAGENTA(...) "\033[35m" __VA_ARGS__ "\033[45m" BTK_CCOLOR_END
#define BTK_CYAN(...) "\033[36m" __VA_ARGS__ "\033[46m" BTK_CCOLOR_END
#define BTK_WHITE(...) "\033[37m" __VA_ARGS__ "\033[47m" BTK_CCOLOR_END
