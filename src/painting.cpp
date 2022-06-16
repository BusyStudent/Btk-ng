#include "build.hpp"
#include <Btk/painter.hpp>

BTK_NS_BEGIN

// Brush::Brush() : Brush(GLColor {
//     0.0f,
//     0.0f,
//     0.0f,
//     0.0f
// }) {}

// Brush::Brush(GLColor c) {
//     _color = c;
//     _pixbuffer = nullptr;
//     _type = BrushType::Solid;
// }
// Brush::Brush(Brush &&b) {
//     Btk_memcpy(this, &b, sizeof(Brush));
//     Btk_memset(&b, 0, sizeof(Brush));
// }
// Brush::~Brush() {
//     clear();
// }

// void Brush::clear() {
//     if (_pixowned) {
//         delete _pixbuffer;
//         _pixowned = false;
//     }
//     _color = GLColor {
//         0.0f,
//         0.0f,
//         0.0f,
//         0.0f
//     };
//     _pixbuffer = nullptr;
//     _type = BrushType::Solid;
// }
// void Brush::assign(Brush &&b) {
//     clear();
//     Btk_memcpy(this, &b, sizeof(Brush));
//     Btk_memset(&b, 0, sizeof(Brush));
// }

BTK_NS_END