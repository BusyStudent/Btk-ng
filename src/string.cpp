#include "build.hpp"

#include <Btk/string.hpp>
#include <stdexcept>
#include <ostream>
#include <cstdarg>

// Utf8 header 

#include "libs/utf8/unchecked.h"

// Platform header
#if defined(_WIN32)
#include <Windows.h> //< For GetACP() and MultiByteToWideChar()
#endif

namespace {
    // Helper
    using namespace BTK_NAMESPACE;

    std::u16string btkus_to_utf16(const char_t *s, size_t n) {
        std::u16string ret;
        utf8::unchecked::utf8to16(s, s + n, std::back_inserter(ret));
        return ret;
    }
    std::u32string btkus_to_utf32(const char_t *s, size_t n) {
        std::u32string ret;
        utf8::unchecked::utf8to32(s, s + n, std::back_inserter(ret));
        return ret;
    }

    template <typename T>
    struct fake_back_inserter {
        T callback;

        fake_back_inserter(T cb) : callback(std::forward<T>(cb)) {}

        fake_back_inserter &operator *() {
            return *this;
        }
        fake_back_inserter &operator ++() {
            return *this;
        }
        fake_back_inserter  operator ++(int) {
            return *this;
        }

        template <typename U>
        fake_back_inserter &operator =(U &&v) {
            callback(std::forward<U>(v));
            return *this;
        }
    };

    template <typename Ret>
    Ret btkus_split(stdu8string_view str, stdu8string_view delim, size_t limit) {
        Ret ret;

        size_t pos = 0;
        while (pos != stdu8string_view::npos) {
            if (ret.size() == limit) {
                break;
            }
            size_t next = str.find(delim, pos);
            if (next == stdu8string_view::npos) {
                ret.push_back(str.substr(pos));
                break;
            }
            ret.push_back(str.substr(pos, next - pos));
            pos = next + delim.size();
        }
        return ret;
    }

    size_t btkus_find(stdu8string_view view, stdu8string_view str) {
        auto pos = view.find(str);
        if (pos == view.npos) {
            return pos;
        }
        // Get distance from the beginning of the string
        return Utf8Strlen(view.data(), pos);
    }
}



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
bool    Utf8IsValid(const char_t *p, size_t size) {
    return utf8::is_valid(p, p + size);
}
size_t  Utf8Locate(const char_t *str, const char_t *p) {
    const char_t *prev = str;
    size_t size = 0;
    while (str != p) {
        Utf8Next(str);
        size += 1;
        prev = str;

        if (str > p) {
            // It probably means the string is not valid utf8 or p is not in the string.
            BTK_THROW(std::runtime_error("Invalid string"));
        }
    }
    return size;
}
size_t  Utf8Encode(char_t buf[4], uchar_t c) {
    size_t size = 0;

    auto cb = [&](uint8_t c) {
        buf[size] = c;
        size ++;

        assert(size <= 4);
    };

    utf8::unchecked::append(c, fake_back_inserter{cb});

    return size;
}
// Seek forward / backward 
void   Utf8Seek(const char_t *str, size_t size, const char_t *&cur, ptrdiff_t dis) {
    auto begin = str;
    auto end   = str + size;

    if (dis > 0) {
        while (cur != end && dis > 0) {
            Utf8Next(cur);
            dis --;
        }
    }
    else if (dis < 0) {
        while (cur != begin && dis < 0) {
            Utf8Prior(cur);
            dis ++;
        }
    }

    if (dis != 0) {
        // Out of range
        BTK_THROW(std::out_of_range("Bad utf8 seek position"));
    }
}

// u8string implementation

void u8string::replace(u8string_view from, u8string_view to, size_t limit) {
    // size_t(-1) is used to indicate no limit
    // It should be bigger enough to hold the largest possible number of replacements.
    // So we didnot check it was size(-1) to make a loop that runs until the end of the string.

    size_t n_replaced = 0;
    size_t pos = _str.find(from);

    while (pos != _str.npos) {
        _str.replace(pos, from.size(), to);
        pos += to.size();
        n_replaced ++;

        if (limit != size_t(-1) && n_replaced >= limit) {
            break;
        }
        pos = _str.find(from, pos);
    }
}
void u8string::pop_back() {
    // Find last uchar begin
    if (size() == 0) {
        return;
    }
    auto str = _str.data();
    auto end = _str.data() + _str.size();
    auto last = end;

    last = Utf8ToPrior(last);

    _str.erase(
        last - str,
        end - last
    );
}

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

// Find
size_t u8string::find(u8string_view v) const {
    return btkus_find(_str, v);
}

// Convert
std::u16string u8string::to_utf16() const {
    return btkus_to_utf16(c_str(), size());
}
std::u32string u8string::to_utf32() const {
    return btkus_to_utf32(c_str(), size());
}

// Split
StringList u8string::split(u8string_view delim, size_t limit) const {
    return btkus_split<StringList>(_str, delim, limit);
}
StringRefList u8string::split_ref(u8string_view delim, size_t limit) const {
    return btkus_split<StringRefList>(_str, delim, limit);
}

// Sub
u8string u8string::substr(size_t pos, size_t len) const {
    if (len == 0) {
        return u8string();
    }
    const_iterator beg = begin() + pos;
    const_iterator ed;
    if (len == npos) {
        ed = end();
    }
    else {
        ed = beg + (len - 1);
    }

    pos = beg.range_begin();
    len = ed.range_end() - pos;

    return u8string(_str.substr(pos, len));
}

// Convert from
u8string u8string::from(const char16_t *data, size_t size) {
    u8string us;
    utf8::unchecked::utf16to8(data, data + size, std::back_inserter(us._str));
    return us;
}
u8string u8string::from(const char32_t *data, size_t size) {
    u8string us;
    utf8::unchecked::utf32to8(data, data + size, std::back_inserter(us._str));
    return us;
}

// Format
u8string u8string::format(const char_t *fmt, ...) {
    va_list varg;
    int s;
    
    va_start(varg, fmt);
#ifdef _WIN32
    s = _vscprintf(fmt, varg);
#else
    s = vsnprintf(nullptr, 0, fmt, varg);
#endif
    va_end(varg);

    u8string str;
    str.resize(s);

    va_start(varg, fmt);
    vsprintf(str.data(), fmt, varg);
    va_end(varg);

    return str;
}


// u8string_view implementation

std::u16string u8string_view::to_utf16() const {
    return btkus_to_utf16(data(), size());
}
std::u32string u8string_view::to_utf32() const {
    return btkus_to_utf32(data(), size());
}

// u8string_view::split
StringList u8string_view::split(u8string_view delim, size_t limit) const {
    return btkus_split<StringList>(*this, delim, limit);
}
StringRefList u8string_view::split_ref(u8string_view delim, size_t limit) const {
    return btkus_split<StringRefList>(*this, delim, limit);
}

// Stream output operator
std::ostream &operator <<(std::ostream &os, const u8string &str) {
    return operator <<(os, str.view());
}

std::ostream &operator <<(std::ostream &os, u8string_view str) {
    // Convert to local encoding
#if defined(_WIN32)
    auto u16 = str.to_utf16();
    auto u16len = u16.size();
    auto u16buf = (wchar_t*)u16.data();
    auto len = WideCharToMultiByte(GetACP(), 0, u16buf, u16len, nullptr, 0, nullptr, nullptr);

    // Allocate buffer
    char *buf = (char*) _malloca(len + 1);
    WideCharToMultiByte(GetACP(), 0, u16buf, u16len, buf, len, nullptr, nullptr);
    buf[len] = '\0';
    os << buf;
    _freea(buf);
    return os;
#else
    return os << std::string_view(str.data(), str.size());
#endif
}


BTK_NS_END