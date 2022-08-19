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
