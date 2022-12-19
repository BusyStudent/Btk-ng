/**
 * @file debug.hpp
 * @author BusyStudent (fyw90mc@gmail.com)
 * @brief Helper headerfor tmply disable / enable debugging at test
 * @version 0.1
 * @date 2022-12-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#if !defined(NDEBUG)

#include <cstdlib>

class _BtkDebugStates {
    public:
        static inline bool Disabled = ::getenv("BTK_NO_DEBUG");
};

// Enable if
#define BTK_ENABLE_ON_DEBUG if (!_BtkDebugStates::Disabled)
#define BTK_DISABLE_DEBUG() _BtkDebugStates::Disabled = true
#define BTK_ENABLE_DEBUG()  _BtkDebugStates::Disabled = false
#else
// Directly disabled
#define BTK_ENABLE_ON_DEBUG if constexpr (false)
#endif