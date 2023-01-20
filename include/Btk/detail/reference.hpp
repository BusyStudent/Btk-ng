#pragma once

#include <Btk/detail/macro.hpp>
#include <type_traits>
#include <memory>

BTK_NS_BEGIN

// Traits for Check the class has ref & unref methods

template <typename T, typename = void>
class _IsRefableImpl : public std::false_type { };

template <typename T>
class _IsRefableImpl<T, std::void_t<decltype(&T::ref)> > : public std::true_type { };

template <typename T>
constexpr auto _IsRefable = _IsRefableImpl<T>::value;

// Begin smart pointer

template <typename T, bool IsRefable>
class _Ref;


template <typename T>
class _Ref<T, false> : public std::shared_ptr<T> {
    public:
        static_assert(!_IsRefable<T>, "Should never has ref method");

        using std::shared_ptr<T>::shared_ptr;
};

template <typename T>
class _Ref<T, true> {
    public:
        static_assert(_IsRefable<T>, "Should have ref method");

        constexpr _Ref() noexcept = default;
        constexpr _Ref(std::nullptr_t) noexcept { };

        _Ref(T *p) {
            ptr = p;
            do_ref();
        }
        _Ref(_Ref && other) {
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        _Ref(const _Ref &other) : _Ref(other.ptr) { }
        ~_Ref() {
            reset();
        }

        void reset(T *n = nullptr) {
            do_unref();
            ptr = n;
            do_ref();
        }
        T   *release(T *n = nullptr) {
            T *prev = ptr;
            ptr = n;
            return prev;
        }
        T   *get() {
            return ptr; 
        }
        const T *get() const {
            return ptr;
        }


        _Ref &operator =(const _Ref &other) {
            if (&other == this) {
                return *this;
            }
            reset(other.get());
            return *this;
        }
        _Ref &operator =(_Ref &&other) {
            if (&other == this) {
                return *this;
            }
            reset(other.release());
            return *this;
        }
        _Ref &operator =(std::nullptr_t) {
            reset();
            return *this;
        }
        _Ref &operator =(T *p) {
            reset(p);
            return *this;
        }

        T   *operator ->() noexcept {
            return ptr;
        }
        T   &operator *() noexcept {
            return *ptr;
        }

        const T *operator ->() const noexcept {
            return ptr;
        }
        const T &operator *() const noexcept {
            return *ptr;
        }

        operator bool() const noexcept {
            return ptr != nullptr;
        }
        bool operator !() const noexcept {
            return ptr == nullptr;
        }
    private:
        void do_ref() {
            if (ptr) {
                ptr->ref();
            }
        }
        void do_unref() {
            if (ptr) {
                ptr->unref();
            }
        }

        T *ptr = nullptr;
};

template <typename T>
using Ref = _Ref<T, _IsRefable<T>>;

// End smart pointer

BTK_NS_END