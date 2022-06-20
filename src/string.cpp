#include "build.hpp"

#include <Btk/string.hpp>

// Utf8 header 

#include "libs/utf8/unchecked.h"

BTK_NS_BEGIN

// Helper function 

void Utf8Next(const char_t *&p) {
    utf8::unchecked::next(p);
}
void Utf8Prior(const char_t *&p) {
    utf8::unchecked::prior(p);
}
uchar_t Utf8Peek(const char_t *p) {
    return utf8::unchecked::peek_next(p);
}
size_t  Utf8Strlen(const char_t *begin, size_t size) {
    const char_t *end = begin + size;
    size_t len = 0;
    while (begin != end) {
        Utf8Next(begin);
        len ++;
    }
    return len;
}
bool    Utf8Isvalid(const char_t *p, size_t size) {
    return utf8::is_valid(p, p + size);
}
size_t  Utf8Locate(const char_t *str, const char_t *p) {
    const char_t *prev = str;
    size_t size = 0;
    while (str != p) {
        Utf8Next(str);
        size += (str - prev);
        prev = str;

        if (str > p) {
            // It probably means the string is not valid utf8 or p is not in the string.
            BTK_THROW(std::runtime_error("Invalid string"));
        }
    }
    return size;
}
size_t  Utf8Encode(char_t buf[4], uchar_t c) {
    // TODO : optimize this function
    stdu8string str;
    utf8::unchecked::append(c, std::back_insert_iterator(str));

    // Copy the string to buf
    size_t size = str.size();
    Btk_memcpy(buf, str.c_str(), size);
    return size;
}

// u8string implementation

void u8string::store_uchar(const char_t *&where, uchar_t c) {
    auto pos = where - c_str();
    auto plen = Utf8ToNext(where) - where;

    // Write to tmp buffer
    char_t buf[4];
    auto blen = Utf8Encode(buf, c);

    // Replace the string
    _str.replace(pos, plen, buf, blen);

    // Done
    where = c_str() + pos;
}

BTK_NS_END