#pragma once

#include <Btk/defs.hpp>
#include <string_view>
#include <string>
#include <vector>
#include <iosfwd>
#include <cstddef>

// TODO List
// 0 - Add support for replace , insert, erase, etc.
// 1 - Improve emoji support


BTK_NS_BEGIN

using stdu8string      = std::basic_string<char_t>;
using stdu8string_view = std::basic_string_view<char_t>;

BTKAPI void    Utf8Prior(const char_t  *&p) BTK_NOEXCEPT;
BTKAPI void    Utf8Next(const char_t   *&p) BTK_NOEXCEPT;
BTKAPI uchar_t Utf8Peek(const char_t    *p) BTK_NOEXCEPT;
BTKAPI size_t  Utf8Strlen(const char_t  *p, size_t size) BTK_NOEXCEPT;
BTKAPI bool    Utf8IsValid(const char_t *p, size_t size) BTK_NOEXCEPT;

BTKAPI size_t  Utf8Encode(char_t buf[4], uchar_t c)      BTK_NOEXCEPT;

// Locate a codepoint in a string. return position (in logical).
BTKAPI size_t  Utf8Locate(const char_t *str, const char_t *p) BTK_NOEXCEPT;
// Seek to
BTKAPI void    Utf8Seek(const char_t *str, size_t size, const char_t *&cur, ptrdiff_t dis) BTK_NOEXCEPT;


// Move to next character in string.
template <typename T>
T    Utf8ToNext(T p) BTK_NOEXCEPT {
    Utf8Next(const_cast<const char_t *&>(p));
    return p;
}
// Move to previous character in string.
template <typename T>
T    Utf8ToPrior(T p) BTK_NOEXCEPT {
    Utf8Prior(const_cast<const char_t *&>(p));
    return p;
}

// Iterator / Codepoint reference
template <typename T>
class _Utf8Codepoint {
    public:
        _Utf8Codepoint(T *c, const char_t *p) : container(c), where(p) {}


        // Load / store

        void operator =(uchar_t c) {
            container->store_uchar(where, c);
        }

        operator uchar_t() const {
            return Utf8Peek(where);
        }
    private:
        const char_t *where;
        T *container;
};


template <typename T>
class _Utf8FastIteratorBase {
    public:
        _Utf8FastIteratorBase() = default;
        _Utf8FastIteratorBase(T *c, const char_t *p) : container(c), where(p) {}

        // Method for children
        size_t codepoint_size() const {
            auto next = Utf8ToNext(where);
            return next - where;
        }
        void   move_next() {
            where = Utf8ToNext(where);
        }
        void   move_prior() {
            where = Utf8ToPrior(where);
        }
        void   seek(ptrdiff_t dis) {
            Utf8Seek(container->data(), container->size(), where, dis);
        }

        // Method for string
        size_t range_begin() const {
            return where - container->data();
        }
        size_t range_end() const {
            return range_begin() + codepoint_size();
        }
        size_t range_len() const {
            return codepoint_size();
        }

        // Load / store
        _Utf8FastIteratorBase &operator =(uchar_t c) {
            container->store_uchar(where, c);
            return *this;
        }
        _Utf8Codepoint<T>      operator *() const {
            return {container, where};
        }

        // Comparison
        bool operator ==(const _Utf8FastIteratorBase &other) const {
            return where == other.where;
        }
        bool operator !=(const _Utf8FastIteratorBase &other) const {
            return where != other.where;
        }

        // Calculate distance
        ptrdiff_t operator -(const _Utf8FastIteratorBase &other) const {
            auto bigger = max(where, other.where);

            if (where == bigger) {
                return Utf8Strlen(other.where, where - other.where);
            }
            return -Utf8Strlen(where, bigger - where);
        }

    protected:
        T        *container = nullptr;
        const char_t *where = nullptr;
};

template <typename T>
class _Utf8StableIteratorBase {
    public:
        _Utf8StableIteratorBase() = default;
        _Utf8StableIteratorBase(T *c, const char_t *p) : 
            container(c), where(Utf8Locate(c->data(), p)) {}

        // Helper methods
        const char_t *address() const {
            auto target = container->data() + where;
            Utf8Seek(target, where, 0, container.size());
            return target;
        }

        // Method for children
        size_t codepoint_size() const {
            auto addr = address();
            auto next = Utf8ToNext(addr);
            return next - addr;
        }

        void   move_next() {
            where += 1;
        }
        void   move_prior() {
            where -= 1;
        }
        void   seek(ptrdiff_t dis) {
            where += dis;
        }

        // Method for u8string
        size_t range_begin() const {
            return address() - container->data();
        }
        size_t range_end() const {
            return range_begin() + codepoint_size();
        }
        size_t range_len() const {
            return codepoint_size();
        }

        // Load / store
        _Utf8StableIteratorBase &operator =(uchar_t c) {
            auto addr = address();
            container->store_uchar(addr, c);
            return *this;
        }
        uchar_t                  operator *() const {
            return Utf8Peek(address());
        }

        // Comparison
        bool operator ==(const _Utf8StableIteratorBase &other) const {
            return where == other.where;
        }
        bool operator !=(const _Utf8StableIteratorBase &other) const {
            return where != other.where;
        }

        // Calculate distance
        ptrdiff_t operator -(const _Utf8StableIteratorBase &other) const {
            return ptrdiff_t(where) - ptrdiff_t(other.where);
        }

    protected:
        T *container = nullptr;
        size_t where = 0;
};

// Template for container

template<
    template <typename> class Base,
    typename T
>
class _Utf8ComIterator : public Base<T> {
    public:
        using Base<T>::Base;

        _Utf8ComIterator &operator ++() {
            this->move_next();
            return *this;
        }
        _Utf8ComIterator &operator --() {
            this->move_prior();
            return *this;
        }
        _Utf8ComIterator  operator ++(int) {
            auto tmp = *this;
            this->move_next();
            return tmp;
        }
        _Utf8ComIterator  operator --(int) {
            auto tmp = *this;
            this->move_prior();
            return tmp;
        }
        // +
        _Utf8ComIterator operator +(ptrdiff_t dis) const {
            auto tmp = *this;
            tmp.seek(dis);
            return tmp;
        }
        _Utf8ComIterator operator -(ptrdiff_t dis) const {
            auto tmp = *this;
            tmp.seek(-dis);
            return tmp;
        }
        // +=
        _Utf8ComIterator &operator +=(ptrdiff_t dis) {
            this->seek(dis);
            return *this;
        }
        _Utf8ComIterator &operator -=(ptrdiff_t dis) {
            this->seek(-dis);
            return *this;
        }

};

// Define iterator template by configuration

#if defined(BTK_SAFER_ITERATIONS)
template <typename T>
using _Utf8IteratorBase = _Utf8StableIteratorBase<T>;
template <typename T>
using _Utf8Iterator     = _Utf8ComIterator<_Utf8StableIteratorBase, T>;
#else
template <typename T>
using _Utf8IteratorBase = _Utf8FastIteratorBase<T>;
template <typename T>
using _Utf8Iterator     = _Utf8ComIterator<_Utf8FastIteratorBase, T>;
#endif

// u8string container

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

        static constexpr auto npos = stdu8string::npos;

        using const_iterator  = _Utf8Iterator<const u8string>;
        using iterator        = _Utf8Iterator<u8string>;
        using const_reference = _Utf8Codepoint<const u8string>; 
        using reference       = _Utf8Codepoint<u8string>; 

        using List            = StringList;
        using RefList         = StringRefList;

        // Misc

        size_t size() const noexcept {
            return _str.size();
        }
        size_t length() const noexcept {
            return Utf8Strlen(_str.data(), _str.size());
        }
        const char_t *c_str() const noexcept {
            return _str.c_str();
        }
        char_t *data() noexcept {
            return _str.data();
        }
        const char_t *data() const noexcept {
            return _str.data();
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
        void shrink_to_fit() noexcept {
            _str.shrink_to_fit();
        }
        void reserve(size_t n) {
            _str.reserve(n);
        }
        void resize(size_t n) {
            _str.resize(n);
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

        // Append
        u8string &append(const char_t *str) {
            _str.append(str);
            return *this;
        }
        u8string &append(const char_t *str, size_t len) {
            _str.append(str, len);
            return *this;
        }
        u8string &append(const u8string &str) {
            _str.append(str._str);
            return *this;
        }
        u8string &append(stdu8string_view str) {
            _str.append(str);
            return *this;
        }
        u8string &append(std::initializer_list<char_t> list) {
            _str.append(list);
            return *this;
        }
        u8string &append(std::initializer_list<uchar_t> ul) {
            for (auto c : ul) {
                _str.push_back(c);
            }
            return *this;
        }
        template <typename ...Args>
        u8string &append_fmt(const char_t *fmt, Args &&...args) {
            append(format(fmt, std::forward<Args>(args)...));
            return *this;
        }

        // Replace
        void     replace(iterator start, iterator end, stdu8string_view str) {
            size_t beg = start.range_begin();
            size_t len = end.range_end() - beg;
            _str.replace(beg, len, str);
        }
        void     replace(iterator where, stdu8string_view str) {
            size_t beg = where.range_begin();
            size_t len = where.range_len();
            _str.replace(beg, len, str);
        }
        void     replace(size_t pos, size_t len, stdu8string_view str) {
            iterator beg;
            iterator ed;
            // No char to replace
            if (len == 0) {
                return;
            }
            beg = begin() + pos;
            if (len == npos) {
                ed = end();
            }
            else {
                ed = beg + (len - 1);
            }
            replace(beg, ed, str);
        }
        void     replace(size_t pos, stdu8string_view str) {
            replace(begin() + pos, str);
        }
        void     replace(u8string_view from, u8string_view to, size_t limit = size_t(-1));

        // Erase
        void     erase(iterator where) {
            size_t beg = where.range_begin();
            size_t len = where.range_len();
            _str.erase(beg, len);
        }
        void     erase(iterator start, iterator end) {
            size_t beg = start.range_begin();
            size_t len = end.range_end() - beg;
            _str.erase(beg, len);
        }
        void      erase(size_t pos, size_t len) {
            iterator beg;
            iterator ed;
            // No char to erase
            if (len == 0) {
                return;
            }
            beg = begin() + pos;
            if (len == npos) {
                ed = end();
            }
            else {
                ed = beg + (len - 1);
            }
            erase(beg, ed);
        }
        void     erase(size_t pos) {
            erase(begin() + pos);
        }

        // Insert
        void     insert(iterator iter, stdu8string_view str) {
            size_t beg = iter.range_begin();
            _str.insert(beg, str);
        }
        void     insert(size_t pos, stdu8string_view str) {
            iterator beg;
            beg = begin() + pos;
            insert(beg, str);
        }

        // Push back
        void     push_back(char_t ch) {
            _str.push_back(ch);
        }
        void     push_back(uchar_t ch) {
            char_t buf[4];
            auto len = Utf8Encode(buf, ch);
            _str.append(buf, len);
        }
        void     pop_back();

        // Find
        size_t   find(u8string_view what) const;
        size_t   find(size_t pos, u8string_view what) const;

        // Ranges
        iterator begin() {
            return iterator(this, data());
        }
        iterator end() {
            return iterator(this, data() + size());
        }
        const_iterator begin() const {
            return const_iterator(this, data());
        }
        const_iterator end() const {
            return const_iterator(this, data() + size());
        }

        // Index
        reference at(size_t pos) {
            auto where = c_str();
            Utf8Seek(where, size(), where, pos);
            return reference(this, where);
        }
        const_reference at(size_t pos) const {
            auto where = c_str();
            Utf8Seek(where, size(), where, pos);
            return const_reference(this, where);
        }

        // Utils
        List       split    (u8string_view what, size_t max = size_t(-1)) const;
        RefList    split_ref(u8string_view what, size_t max = size_t(-1)) const;
        bool       contains (u8string_view what)                          const;
        u8string   substr   (size_t start, size_t len = size_t(-1))       const;
        bool       ends_with(u8string_view what)                          const;
        bool       starts_with(u8string_view what)                        const;

        // Make a view of the string
        u8string_view view() const;

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
        u8string & operator =(stdu8string_view s) {
            _str = s;
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
        reference  operator [](size_t pos) {
            return at(pos);
        }
        const_reference  operator [](size_t pos) const {
            return at(pos);
        }

        // Cast
        std::u16string to_utf16() const;
        std::u32string to_utf32() const;

        // Auto cast to stdu8string const refernce
        operator const stdu8string &() const noexcept {
            return _str;
        }

        // Format
        static u8string format(const char_t *fmt, ...);

        // From another types
        static u8string from(const void *data, size_t size);
        static u8string from(const wchar_t  *data, size_t size);
        static u8string from(const char16_t *data, size_t size);
        static u8string from(const char32_t *data, size_t size);

        static u8string from(std::u16string_view str);
        static u8string from(std::u32string_view str);
        static u8string from(std::wstring_view   str);

    private:
        void store_uchar(const char_t *&where, uchar_t c);

        stdu8string _str;//< String data
    template <typename T>
    friend class _Utf8FastIteratorBase;
};

// View of utf8 string 

class BTKAPI u8string_view {
    public:
        u8string_view() = default;
        u8string_view(const char_t *str) : _str(str) {}
        u8string_view(const char_t *str, size_t len) : _str(str, len) {}
        u8string_view(const stdu8string &str) : _str(str) {}
        u8string_view(const u8string    &str) : _str(str.str()) {}
        u8string_view(stdu8string_view str)   : _str(str) {}

        static constexpr auto npos = stdu8string_view::npos;

        using const_reference = _Utf8Codepoint<const u8string_view>; 
        using reference       = _Utf8Codepoint<const u8string_view>;
        using const_iterator  = _Utf8Iterator<const u8string_view>;
        using iterator        = _Utf8Iterator<const u8string_view>;

        using List           = StringList;
        using RefList        = StringRefList;

        size_t size() const noexcept {
            return _str.size();
        }
        size_t length() const noexcept {
            return Utf8Strlen(_str.data(), _str.size());
        }
        bool   empty() const noexcept {
            return _str.empty();
        }
        const char_t *data() const noexcept {
            return _str.data();
        }

        // Iterators
        const_iterator begin() const {
            return const_iterator(this, _str.data());
        }
        const_iterator end() const {
            return const_iterator(this, _str.data() + _str.size());
        }
        const_reference front() const {
            return *begin();
        }
        const_reference back() const {
            return *--end();
        }

        // Index
        const_reference at(size_t pos) const {
            auto where = data();
            Utf8Seek(where, size(), where, pos);
            return const_reference(this, where);
        }

        // Utils
        List       split    (u8string_view what, size_t max = size_t(-1)) const;
        RefList    split_ref(u8string_view what, size_t max = size_t(-1)) const;
        bool       contains (u8string_view what)                          const;

        // Cast
        std::u16string to_utf16() const;
        std::u32string to_utf32() const;
        
        // Operators
        const_reference operator [](size_t pos) const {
            return at(pos);
        }

        // Auto cast to stdu8string const refernce
        operator const stdu8string_view &() const noexcept {
            return _str;
        }
    private:
        stdu8string_view _str;//< String data
};

class StringList    : public std::vector<u8string> {
    public:
        using std::vector<u8string>::vector;
};
class StringRefList : public std::vector<u8string_view> {
    public:
        using std::vector<u8string_view>::vector;
};

// Implementation for some method in u8string

inline u8string u8string::from(std::u16string_view str) {
    return u8string::from(str.data(), str.length());
}
inline u8string u8string::from(std::u32string_view str) {
    return u8string::from(str.data(), str.length());
}
inline u8string u8string::from(std::wstring_view   str) {
    // Check wstring in current platform is UTF-16 or UTF-32
    return u8string::from(str.data(), str.length());
}
inline u8string u8string::from(const wchar_t *str, size_t size) {
    using wchar = std::conditional_t<sizeof(wchar_t) == 2, char16_t, char32_t>;
    return u8string::from(reinterpret_cast<const wchar *>(str), size);
}

// Implmementation for some utils in u8string

inline bool u8string::contains(u8string_view what) const {
    return _str.find(what) != stdu8string::npos;
}

inline u8string_view u8string::view() const {
    return u8string_view(_str.data(), _str.size());
}

// Implmentations for some utils in u8string_view

inline bool u8string_view::contains(u8string_view what) const {
    return _str.find(what) != stdu8string_view::npos;
}

// Stream operator for u8string

BTKAPI std::ostream & operator <<(std::ostream &os, const u8string &str);
BTKAPI std::ostream & operator <<(std::ostream &os, u8string_view   str);

BTK_NS_END

// Provide Hash function for u8string
namespace std {
    template <>
    struct hash<BTK_NAMESPACE::u8string> {
        size_t operator()(const BTK_NAMESPACE::u8string &str) const noexcept {
            return hash<BTK_NAMESPACE::stdu8string>()(str.str());
        }
    };
    template <>
    struct hash<BTK_NAMESPACE::u8string_view> {
        size_t operator()(const BTK_NAMESPACE::u8string_view &str) const noexcept {
            return hash<BTK_NAMESPACE::stdu8string_view>()(str);
        }
    };
}