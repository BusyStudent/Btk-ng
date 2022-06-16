#pragma once

#include <Btk/defs.hpp>

BTK_NS_BEGIN

// Blend mode / Bend Factor

enum class BlendMode : uint32_t {
    None = 0,
    Alpha = 1 << 0,
    Add = 1 << 1,
    Subtract = 1 << 2,
    Modulate = 1 << 3,
};


BTK_NS_END