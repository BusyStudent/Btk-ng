/**
 * @file win32.hpp
 * @author BusyStudent (fyw90mc@gmail.com)
 * @brief Simple wrapper of Win32 APIS
 * @version 0.1
 * @date 2023-04-11
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#include "build.hpp"
#include <windows.h>

#undef min
#undef max

BTK_NS_BEGIN2(BTK_NAMESPACE::Win32)

class CriticalSection {
    public:
        CriticalSection() {
            ::InitializeCriticalSection(&section);
        }
        ~CriticalSection() {
            ::DeleteCriticalSection(&section);
        }

        void lock() {
            ::EnterCriticalSection(&section);
        }
        void unlock() {
            ::LeaveCriticalSection(&section);
        }
        BOOL try_lock() {
            return ::TryEnterCriticalSection(&section);
        }
    private:
        CriticalSection(const CriticalSection &) = delete;

        CRITICAL_SECTION section;
};
class RegKeyIterator {
    public:
        RegKeyIterator(HKEY key, DWORD idx) : key(key), idx(idx) { }
        RegKeyIterator(const RegKeyIterator &) = default;
        ~RegKeyIterator() = default;

        void operator ++() {
            valuelen = sizeof(value);
            auto ls = ::RegEnumKeyExW(key, idx, value, &valuelen, nullptr, nullptr, nullptr, nullptr);
            if (ls == ERROR_SUCCESS) {
                // Ok next
                idx++;
            }
            else {
                // Eof or error
                idx = ~DWORD(0);
            }
        }
        bool operator ==(const RegKeyIterator &it) const noexcept {
            return idx == it.idx; // We compare indices, not values.
        }
        bool operator !=(const RegKeyIterator &it) const noexcept {
            return idx != it.idx;
        }
        std::wstring_view operator *() const noexcept {
            return std::wstring_view(value, valuelen);
        }
    private:
        HKEY key = nullptr;
        DWORD idx = 0;
        wchar_t value[MAX_PATH + 1];
        DWORD valuelen = sizeof(value);
};
// For enum values
class RegValueIterator {
    public:
        RegValueIterator(HKEY key, DWORD idx) : key(key), idx(idx) { }
        RegValueIterator(const RegValueIterator &) = default;
        ~RegValueIterator() = default;

        void operator ++() {
            valuelen = sizeof(value);
            auto ls = ::RegEnumValueW(key, idx, value, &valuelen, nullptr, nullptr, nullptr, nullptr);
            if (ls == ERROR_SUCCESS) {
                // Ok next
                idx++;
            }
            else {
                // Eof or error
                idx = ~DWORD(0);
            }
        }
        bool operator ==(const RegValueIterator &it) const noexcept {
            return idx == it.idx; // We compare indices, not values.
        }
        bool operator !=(const RegValueIterator &it) const noexcept {
            return idx != it.idx;
        }
        std::wstring_view operator *() const noexcept {
            return std::wstring_view(value, valuelen);
        }
    private:
        HKEY key = nullptr;
        DWORD idx = 0;
        wchar_t value[MAX_PATH + 1];
        DWORD valuelen = MAX_PATH + 1;
};
class RegKeyValues {
    public:
        using Iterator = RegValueIterator;

        HKEY key;

        Iterator begin() const {
            Iterator it(key, 0);
            ++it;
            return it;
        }
        Iterator end() const {
            return Iterator(key, ~DWORD(0));
        }
};

class RegKey {
    public:
        using Iterator = RegKeyIterator;
        using Values   = RegKeyValues;
    public:
        RegKey() = default;
        RegKey(HKEY s, LPCWSTR subkey) {
            if (::RegOpenKeyW(s, subkey, &key) != ERROR_SUCCESS) {
                key = nullptr;
            }
        }
        RegKey(HKEY s, LPCSTR subkey) {
            if (::RegOpenKeyA(s, subkey, &key) != ERROR_SUCCESS) {
                key = nullptr;
            }
        }
        RegKey(HKEY s, LPCSTR subkey, DWORD opt, REGSAM sam) {
            if (::RegOpenKeyExA(s, subkey, opt, sam, &key) != ERROR_SUCCESS) {
                key = nullptr;
            }
        }
        RegKey(HKEY s, LPCWSTR subkey, DWORD opt, REGSAM sam) {
            if (::RegOpenKeyExW(s, subkey, opt, sam, &key) != ERROR_SUCCESS) {
                key = nullptr;
            }
        }

        RegKey(const RegKey &) = delete;
        RegKey(RegKey &&k) {
            key = k.release();
        }
        ~RegKey() { reset(); }

        void reset(HKEY new_k = nullptr) {
            if (key) {
                ::RegCloseKey(key);
            }
            key = new_k;
        }
        HKEY release(HKEY new_k = nullptr) {
            HKEY prev = key;
            key = new_k;
            return prev;
        }
        bool ok() const {
            return key;
        }

        std::wstring query_value_string(LPCWSTR subkey) const {
            DWORD buffersize = 0;
            LSTATUS lstatus;

            lstatus = ::RegQueryValueExW(key, subkey, nullptr, nullptr, nullptr, &buffersize);
            if (lstatus != ERROR_SUCCESS && lstatus != ERROR_MORE_DATA) {
                return std::wstring();
            }
            std::wstring buffer;
            buffer.resize(buffersize / sizeof(wchar_t));
            if (::RegQueryValueExW(key, subkey, nullptr, nullptr, (LPBYTE) buffer.data(), &buffersize) != ERROR_SUCCESS) {
                return std::wstring();
            }
            if (buffer.back() == L'\0') {
                buffer.resize(buffer.size() - 1); // remove the trailing \0
            }

            return buffer;
        }


        // In here string_view treated as 0 terminated
        std::wstring query_value_string(std::wstring_view v) const {
            return query_value_string(v.data());
        }

        // SubItem
        Iterator begin() const {
            Iterator it(key, 0);
            ++it;
            return it;
        }
        Iterator end() const {
            return Iterator(key, ~DWORD(0));
        }

        Values values() const noexcept {
            return {key};
        }
        RegKey subkey(LPCWSTR sub, DWORD opt = 0, REGSAM sam = KEY_READ) const {
            return RegKey(key, sub, opt, sam);
        }
        RegKey subkey(LPSTR sub, DWORD opt = 0, REGSAM sam = KEY_READ) const {
            return RegKey(key, sub, opt, sam);
        }

        RegKey &operator =(RegKey &&k) {
            reset(k.release());
            return *this;
        }


        operator HKEY() const noexcept {
            return key;
        }
        operator bool() const noexcept {
            return key;
        }
    private:
        HKEY key = nullptr;
};
class GdiBitmap {
    public:
        GdiBitmap(HBITMAP hb = nullptr) : bitmap(hb) { }
        GdiBitmap(const GdiBitmap &nb) = delete;
        GdiBitmap(GdiBitmap &&b) {
            bitmap = b.release();
        }
        ~GdiBitmap() { reset(); }

        void reset(HBITMAP new_b = nullptr) {
            if (bitmap) {
                DeleteObject(bitmap);
            }
            bitmap = new_b;
        }
        HBITMAP release(HBITMAP new_b = nullptr) {
            auto prev = bitmap;
            bitmap = new_b;
            return prev;
        }
        void    *pixels() const {
            BITMAP bm;
            GetObject(bitmap, sizeof(BITMAP), &bm);
            return bm.bmBits;
        }
        GdiBitmap &operator =(GdiBitmap &&b) {
            reset(b.release());
            return *this;
        }
    private:
        HBITMAP bitmap = nullptr;
};
class GdiGraphics {
    public:

    private:
        HDC dc;
};
class FormatedError {
    public:
        FormatedError(DWORD errocde) : errcode(errcode) {
            DWORD result = FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                nullptr,
                errocde,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPWSTR>(&msg),
                0,
                nullptr
            );
        }
        FormatedError(const FormatedError &) = delete;
        ~FormatedError() {
            ::LocalFree(msg);
        }

        wchar_t *raw_message() const noexcept {
            return msg;
        }
        u8string u8_message() const noexcept {
            return u8string::from(msg);
        }
        DWORD    code() const noexcept {
            return errcode;
        }
    private:
        wchar_t *msg = nullptr;
        DWORD    errcode = 0;
};

BTK_NS_END2(BTK_NAMESPACE::Win32)