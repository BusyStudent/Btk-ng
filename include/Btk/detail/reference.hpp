#pragma once

#include <Btk/detail/macro.hpp>
#include <type_traits>
#include <atomic>

BTK_NS_BEGIN

// Begin smart pointer
template <typename T>
class Ref {
    public:
        constexpr Ref() noexcept = default;
        constexpr Ref(std::nullptr_t) noexcept { };

        Ref(T *p) {
            ptr = p;
            doRef();
        }
        Ref(Ref && other) {
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        template <typename U>
        Ref(const Ref<U> &other) : Ref(other.get()) { }
        Ref(const Ref    &other) : Ref(other.get()) { }
        ~Ref() {
            reset();
        }

        void reset(T *n = nullptr) {
            do_unref();
            ptr = n;
            doRef();
        }
        T   *release(T *n = nullptr) {
            T *prev = ptr;
            ptr = n;
            return prev;
        }
        T   *get() const {
            return ptr; 
        }
        bool unique() const {
            if (!ptr) {
                return false;
            }
            return ptr->unique();
        }

        template <typename U>
        Ref<U> as() const {
            return static_cast<U*>(ptr);
        }

        Ref &operator =(const Ref &other) {
            if (&other == this) {
                return *this;
            }
            reset(other.get());
            return *this;
        }
        Ref &operator =(Ref &&other) {
            if (&other == this) {
                return *this;
            }
            reset(other.release());
            return *this;
        }
        Ref &operator =(std::nullptr_t) {
            reset();
            return *this;
        }
        Ref &operator =(T *p) {
            reset(p);
            return *this;
        }

        T   *operator ->() const noexcept {
            return ptr;
        }
        T   &operator *() const noexcept {
            return *ptr;
        }

        operator bool() const noexcept {
            return ptr != nullptr;
        }
        bool operator !() const noexcept {
            return ptr == nullptr;
        }
    private:
        void doRef() {
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
// End smart pointer

// Begin Cow
template <typename T>
class Cow {
    public:
        Cow() = default;
        Cow(T *p) : object(p) { }
        Cow(const Ref<T> &r) : object(r) { }
        Cow(const Cow &) = default;
        ~Cow() = default;

        Cow &mut() {
            if (!object.unique()) {
                object.reset(new T(*object));
            }
            return *this;
        }
        template <typename Callable>
        Cow &mut(Callable &&cb) {
            mut();
            cb(*object);
            return *this;
        }

        Cow &operator =(const Cow &) = default;
        Cow &operator =(Cow &&) = default;
        Cow &operator =(const Ref<T> &r) {
            object = r;
            return *this;
        }

        T  *get() const noexcept {
            return object.get();
        }
        T  * operator ->() const noexcept {
            return object.get();
        }
        T  & operator *() const noexcept {
            return *object.get();
        }
        bool operator !() const noexcept {
            return !object;
        }
        operator bool() const noexcept {
            return object;
        }
        operator const Ref<T> &() const noexcept {
            return object;
        }
    private:
        Ref<T> object;
};


// Begin Refable helper
template <typename T>
class Refable {
    public:
        // Constructor
        // Copy doesnot inhert refcount
        Refable() = default;
        Refable(const Refable &) : Refable() { }
        Refable(Refable &&) : Refable() { }

        void ref() {
            ++_refcount;
        }
        void unref() {
            if (--_refcount <= 0) {
                delete static_cast<T*>(this);
            }
        }
        void set_refcount(int c) noexcept {
            _refcount = c;
        }
        int  refcount() const noexcept {
            return _refcount;
        }
        bool unique() const noexcept {
            return _refcount <= 1;
        }
    private:
        std::atomic_int _refcount = 0;
};

/**
 * @brief Dyn Refcounting interface
 * 
 */
class DynRefable { 
    public:
        virtual void ref()   = 0;
        virtual void unref() = 0;
};


template <typename T, typename ...Args>
inline Ref<T> MakeRefable(Args &&...args) {
    return Ref<T>(new T(std::forward<Arg>(args)...));
}


template <typename T>
inline bool operator ==(const Ref<T> &a, const Ref<T> &b) noexcept {
    return a.get() == b.get();
}
template <typename T>
inline bool operator !=(const Ref<T> &a, const Ref<T> &b) noexcept {
    return a.get() != b.get();
}
template <typename T>
inline bool operator ==(const Ref<T> &a, std::nullptr_t) noexcept {
    return a.get() == nullptr;
}
template <typename T>
inline bool operator !=(const Ref<T> &a, std::nullptr_t) noexcept {
    return a.get() != nullptr;
}
template <typename T>
inline bool operator ==(const Ref<T> &a, T *ptr) noexcept {
    return a.get() == ptr;
}
template <typename T>
inline bool operator !=(const Ref<T> &a, T *ptr) noexcept {
    return a.get() != ptr;
}
// End refhelper


BTK_NS_END