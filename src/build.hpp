#pragma once

#define _BTK_SOURCE

#include <Btk/detail/macro.hpp>
#include <Btk/detail/types.hpp>
#include <Btk/string.hpp>
#include <Btk/defs.hpp>


#if !defined(NDEBUG)
#define BTK_LOG(...) ::printf(__VA_ARGS__);
#else
#define BTK_LOG(...)
#endif