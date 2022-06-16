#pragma once

#include <Btk/defs.hpp>
#include <string_view>
#include <string>

BTK_NS_BEGIN

using stdu8string = std::basic_string<char_t>;
using stdu8string_view = std::basic_string_view<char_t>;

BTKAPI uchar_t Utf8Next(const char_t *&p) BTK_NOEXCEPT;
BTKAPI uchar_t Utf8Prev(const char_t *&p) BTK_NOEXCEPT;
BTKAPI uchar_t Utf8Peek(const char_t *p)  BTK_NOEXCEPT;
BTKAPI size_t  Utf8Strlen(const char_t *p, size_t size) BTK_NOEXCEPT;
BTKAPI bool    Utf8IsValid(const char_t *p, size_t size) BTK_NOEXCEPT;


class BTKAPI u8string {
    public:
        u8string() = default;
        u8string(const char_t *str) : _str(str) {}
        u8string(const char_t *str, size_t len) : _str(str, len) {}
        u8string(const char_t *str, const char_t *end) : _str(str, end) {}
        u8string(stdu8string_view str) : _str(str) {}
        u8string(const u8string &str) : _str(str._str) {}
        u8string(u8string &&str) : _str(std::move(str._str)) {}
        ~u8string() = default;

        size_t size() const noexcept {
            return _str.size();
        }
        size_t length() const noexcept {
            return Utf8Strlen(_str.data(), _str.size());
        }
        const char_t *c_str() const noexcept {
            return _str.c_str();
        }

        bool is_valid() const noexcept {
            return Utf8IsValid(_str.data(), _str.size());
        }
        bool empty() const noexcept {
            return _str.empty();
        }
        void clear() noexcept {
            _str.clear();
        }
        // Compare / Assign
        bool compare(const u8string &str) const noexcept {
            return _str == str._str;
        }
        bool compare(const char_t *str) const noexcept {
            return _str == str;
        }
        bool compare(const char_t *str, size_t len) const noexcept {
            return _str == stdu8string_view(str, len);
        }

        void assign(const u8string &str) noexcept {
            _str = str._str;
        }
        void assign(const char_t *str) noexcept {
            _str = str;
        }
        void assign(const char_t *str, size_t len) noexcept {
            _str = stdu8string_view(str, len);
        }

        // Implmentations
        stdu8string & str() noexcept {
            return _str;
        }
        const stdu8string & str() const noexcept {
            return _str;
        }

        // Operators
        u8string & operator =(const u8string &str) noexcept {
            _str = str._str;
            return *this;
        }
        u8string & operator =(u8string &&str) noexcept {
            _str = std::move(str._str);
            return *this;
        }
        u8string & operator =(const char_t *str) noexcept {
            _str = str;
            return *this;
        }
        bool       operator ==(const u8string &str) const noexcept {
            return compare(str);
        }
        bool       operator ==(const char_t *str) const noexcept {
            return compare(str);
        }
        bool       operator !=(const u8string &str) const noexcept {
            return !compare(str);
        }
        bool       operator !=(const char_t *str) const noexcept {
            return !compare(str);
        }

        // From another types
        static u8string from(const void *data, size_t size);
        static u8string from(const char16_t *data, size_t size);
        static u8string from(const char32_t *data, size_t size);

    private:
        stdu8string _str;//< String data
};

class BTKAPI u8string_view {
    public:
        u8string_view() = default;
        u8string_view(const char_t *str) : _str(str) {}
        u8string_view(const char_t *str, size_t len) : _str(str, len) {}

        size_t size() const noexcept {
            return _str.size();
        }
        size_t length() const noexcept {
            return Utf8Strlen(_str.data(), _str.size());
        }
        const char_t *data() const noexcept {
            return _str.data();
        }
    private:
        stdu8string_view _str;//< String data
};


BTK_NS_END

// Provide Hash function for u8string
namespace std {
    template <>
    struct hash<Btk::u8string> {
        size_t operator()(const BTK_NAMESPACE::u8string &str) const noexcept {
            return hash<BTK_NAMESPACE::stdu8string>()(str.str());
        }
    };
}