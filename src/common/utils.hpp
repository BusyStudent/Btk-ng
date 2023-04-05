#pragma once

#include <Btk/detail/reference.hpp>

// Helper macro for COW

// Make some utility macros
#define COW_MUT(priv)                              \
    {                                              \
        using Type = std::decay_t<decltype(*priv)>;\
        if (!priv) {                               \
            priv = new Type();                     \
            priv->set_refcount(1);                 \
        }                                          \
        else if (priv->refcount() != 1) {          \
            auto n = new Type(*priv);              \
            priv->unref();                         \
            priv = n;                              \
            priv->set_refcount(1);                 \
        }                                          \
    } // Copy on write

// Make begin mut fn
#define COW_MUT_IMPL(type)   \
    void type::begin_mut() { \
        COW_MUT(priv);       \
    } // Copy on write
// Make copy constructor
#define COW_COPY_IMPL(type) \
    type::type(const type &other) { \
        priv = other.priv; \
        if (priv) { \
            priv->ref(); \
        } \
    }
// Make move constructor
#define COW_MOVE_IMPL(type) \
    type::type(type &&other) { \
        priv = other.priv; \
        other.priv = nullptr; \
    }
// Make destructor
#define COW_RELEASE_IMPL(type) \
    type::~type() { \
        if (priv) { \
            priv->unref(); \
        } \
    }
// Make copy assignment
#define COW_COPY_ASSIGN_IMPL(type) \
    type &type::operator =(const type &other) { \
        if (priv != other.priv) { \
            if (priv) { \
                priv->unref(); \
            } \
            priv = other.priv; \
            if (priv) { \
                priv->ref(); \
            } \
        } \
        return *this; \
    }
// Make move assignment
#define COW_MOVE_ASSIGN_IMPL(type) \
    type &type::operator =(type &&other) { \
        if (priv != other.priv) { \
            if (priv) { \
                priv->unref(); \
            } \
            priv = other.priv; \
            other.priv = nullptr; \
        } \
        return *this; \
    }
// Make nullptr assignment
#define COW_NULLPTR_ASSIGN_IMPL(type) \
    type &type::operator =(std::nullptr_t) { \
        if (priv) { \
            priv->unref(); \
        } \
        priv = nullptr; \
        return *this; \
    }
// Make swap
#define COW_SWAP_IMPL(type) \
    void type::swap(type &other) { \
        std::swap(priv, other.priv); \
    }
// SAFE Release / Reference
#define COW_REFERENCE(ptr) \
    ptr ? ptr->ref(), ptr : nullptr
#define COW_RELEASE(ptr) \
    if (ptr) { \
        ptr->unref(); \
        ptr = nullptr; \
    }

// Generate COW impl
#define COW_BASIC_IMPL(type) \
    COW_COPY_IMPL(type) \
    COW_MOVE_IMPL(type) \
    COW_RELEASE_IMPL(type) \
    COW_COPY_ASSIGN_IMPL(type) \
    COW_MOVE_ASSIGN_IMPL(type) \
    COW_NULLPTR_ASSIGN_IMPL(type) \
    COW_SWAP_IMPL(type) \

#define COW_IMPL(type) \
    COW_BASIC_IMPL(type) \
    COW_MUT_IMPL(type) \

// COM Helper
#define COM_RELEASE(ptr) \
    if (ptr) { \
        ptr->Release(); \
        ptr = nullptr; \
    }

// Helper templates

namespace {

class WeakData {
    public:
        int   _refcount = 0;
        bool  _die      = false;

        void ref() {
            ++_refcount;
        }
        void unref() {
            if (--_refcount == 0) {
                delete this;
            }
        }
};

template <typename T>
class WeakRef;
template <typename T>
class Ref;

// template <typename T>
// class WeakRefable : public Refable<T> {
//     public:
//         WeakRefable() : Refable<T>() {}
//         WeakRefable(const WeakRefable &r) : Refable<T>(r) {}
//         WeakRefable(WeakRefable &&r) : Refable<T>(r) {}
//         ~WeakRefable() {
//             // Destroy weak refs
//             if (_weak) {
//                 _weak->_die = true;
//                 _weak->unref();

//                 _weak = nullptr;
//             }
//         }

//         auto weakref() -> WeakData *{
//             if (!_weak) {
//                 _weak = new WeakData();
//                 _weak->ref();
//             }
//             return _weak;
//         }
//     private:
//         WeakData *_weak = nullptr; //< For weak reference
//     template <typename U>
//     friend class WeakRef;
// };

// Strong reference
// template <typename T>
// class Ref {
//     public:
//         Ref() : ptr(nullptr) {}
//         Ref(T *ptr) : ptr(ptr) {
//             if (ptr) {
//                 ptr->ref();
//             }
//         }
//         Ref(const Ref &other) : ptr(other.ptr) {
//             if (ptr) {
//                 ptr->ref();
//             }
//         }
//         Ref(Ref &&other) : ptr(other.ptr) {
//             other.ptr = nullptr;
//         }
//         ~Ref() {
//             if (ptr) {
//                 ptr->unref();
//             }
//         }
//         Ref &operator =(const Ref &other) {
//             if (ptr != other.ptr) {
//                 if (ptr) {
//                     ptr->unref();
//                 }
//                 ptr = other.ptr;
//                 if (ptr) {
//                     ptr->ref();
//                 }
//             }
//             return *this;
//         }
//         Ref &operator =(Ref &&other) {
//             if (ptr != other.ptr) {
//                 if (ptr) {
//                     ptr->unref();
//                 }
//                 ptr = other.ptr;
//                 other.ptr = nullptr;
//             }
//             return *this;
//         }
//         Ref &operator =(T *other) {
//             if (ptr != other) {
//                 if (ptr) {
//                     ptr->unref();
//                 }
//                 ptr = other;
//                 if (ptr) {
//                     ptr->ref();
//                 }
//             }
//             return *this;
//         }
//         Ref &operator =(std::nullptr_t) {
//             if (ptr) {
//                 ptr->unref();
//             }
//             ptr = nullptr;
//             return *this;
//         }
//         T *operator ->() {
//             return ptr;
//         }
//         const T *operator ->() const {
//             return ptr;
//         }
//         T &operator *() {
//             return *ptr;
//         }
//         const T &operator *() const {
//             return *ptr;
//         }
//         operator bool() const {
//             return ptr != nullptr;
//         }
//         bool operator !() const {
//             return ptr == nullptr;
//         }
//         T *get() {
//             return ptr;
//         }
//         const T *get() const {
//             return ptr;
//         }
//         T *release() {
//             T *tmp = ptr;
//             ptr = nullptr;
//             return tmp;
//         }
//         void reset() {
//             if (ptr) {
//                 ptr->unref();
//             }
//             ptr = nullptr;
//         }
//     private:
//         T *ptr = nullptr;
// };

// Weaks Reference
template <typename T>
class WeakRef {
    public:
        WeakRef() : ptr(nullptr) {};
        WeakRef(T *ptr) : ptr(ptr) {
            do_ref();
        }
        WeakRef(const WeakRef &other) : WeakRef(other.ptr) {}
        WeakRef(Ref<T> &r)       : WeakRef(r.get()) {}
        WeakRef(WeakRef &&other) : ptr(other.ptr), weak(other.weak) {
            other.weak = nullptr;
            other.ptr = nullptr;
        }
        ~WeakRef() {
            do_unref();
        }

        WeakRef &operator =(const WeakRef &other) {
            if (ptr != other.ptr) {
                do_unref();
                ptr = other.ptr;
                do_ref();
            }
            return *this;
        }
        WeakRef &operator =(WeakRef &&other) {
            if (ptr != other.ptr) {
                do_unref();
                ptr = other.ptr;
                weak = other.weak;
                other.weak = nullptr;
                other.ptr = nullptr;
            }
            return *this;
        }
        WeakRef &operator =(T *other) {
            if (ptr != other) {
                do_unref();
                ptr = other;
                do_ref();
            }
            return *this;
        }
        WeakRef &operator =(std::nullptr_t) {
            do_unref();
            ptr = nullptr;
            return *this;
        }
        WeakRef &operator =(Ref<T> &ref) {
            return operator =(ref.get());
        }

        bool empty() const {
            return ptr == nullptr;
        }
        bool expired() const {
            return weak == nullptr || weak->_die;
        }

        Ref<T> lock() {
            if (!expired()) {
                return Ref<T>(ptr);
            }
            return Ref<T>();
        }
    private:
        void do_ref() {
            if (!ptr) {
                return;
            }
            weak = ptr->weakref();
            weak->ref();
        }
        void do_unref() {
            if (!ptr) {
                return;
            }
            if (weak) {
                weak->unref();
                weak = nullptr;
                ptr  = nullptr;
            }
        }

        T        *ptr  = nullptr;
        WeakData *weak = nullptr;
};


// Helper for Make a instance
// template <typename T>
// template <typename ...Args>
// inline Ref<T> Refable<T>::New(Args &&... args) {
//     return Ref<T>(new T(std::forward<Args>(args)...));
// }


// Helper to calc the size
template <size_t V1, size_t V2, bool cond = (V1 > V2)>
class Max;

template <size_t V1, size_t V2>
class Max<V1, V2, false> {
    public:
        static constexpr size_t value = V2;
};

template <size_t V1, size_t V2>
class Max<V1, V2, true> {
    public:
        static constexpr size_t value = V1;
};

// [int, double, float] => types
// [1  , 2  , 3  ]      => int, double, float (id)

// For get the size to contain the given number of elements
template <typename T, typename ...Args>
class UnionCalcSize {
    public:
        static constexpr size_t value = Max<
            sizeof(T),
            UnionCalcSize<Args...>::value
        >::value;
};

// End of recursion
template <typename T>
class UnionCalcSize<T> {
    public:
        static constexpr size_t value = sizeof(T);
};

// Get type by id
template <size_t ID, typename T, typename ...Args>
class UnionGetTypeByID {
    public:
        static_assert(ID > 0, "ID must be greater than 0");
        static_assert(ID <= sizeof...(Args), "ID must be less than or equal to sizeof...(Args)");
        using type = typename UnionGetTypeByID<ID - 1, Args...>::type;
};

template <typename T, typename ...Args>
class UnionGetTypeByID<1, T, Args...> {
    public:
        using type = T;
};

// Get id by type
template <size_t ID, typename Req, typename T, bool _cond, typename ...Args>
class UnionGetIDByTypeImpl ;
template <size_t ID, typename Req, typename T, typename ...Args>
class UnionGetIDByTypeForward ;

template <size_t ID, typename Req, typename T, typename ...Args>
class UnionGetIDByTypeImpl<ID, Req, T, true, Args...> {
    // Same types
    public:
        static constexpr size_t id = ID; 
};

template <size_t ID, typename Req, typename T, typename ...Args>
class UnionGetIDByTypeImpl<ID, Req, T, false, Args...> {
    // Same types
    public:
        static_assert(sizeof ...(Args) > 0, "Unmatched type");
        static constexpr size_t id = UnionGetIDByTypeForward<ID + 1, Req, Args...>::id;
};

template <size_t ID, typename Req, typename T, typename ...Args>
class UnionGetIDByTypeForward {
    public:
        static constexpr size_t id = UnionGetIDByTypeImpl<ID, Req, T, std::is_same<Req, T>::value, Args...>::id;
};

template <typename Req, typename ...Args>
class UnionGetIDByType {
    public:
        static_assert(sizeof...(Args) > 0, "Args must be greater than 0");
        static constexpr size_t value = UnionGetIDByTypeForward<1, Req, Args...>::id;
};

template <size_t ID, typename T, typename ...Args>
class UnionReleaser {
    public:
        static void Release(int id,  uint8_t *buf) {
            if (id == ID) {
                // Same id , call destructor
                reinterpret_cast<T*>(buf)->~T();
                return ;
            }
            return UnionReleaser<ID + 1, Args...>::Release(id, buf);
        }
};

template <typename T, typename ...Args>
class UnionReleaser<0, T, Args...> {
    public:
        static void Release(int id,  uint8_t *buf) {
            if (id == 0) {
                // None
                return ;
            }

            return UnionReleaser<1, T, Args...>::Release(id, buf);
        }
};

// End of recursion
template <size_t ID,typename T>
class UnionReleaser<ID, T> {
    public:
        static void Release(int id,  uint8_t *buf) {
            if (id == ID) {
                // Same id , call destructor
                reinterpret_cast<T*>(buf)->~T();
                return ;
            }
            // Should not be here
            assert(false);
        }
};

// Traits of it
template <typename ...Args>
class UnionTraits {
    public:
        static constexpr size_t bufsize = UnionCalcSize<Args...>::value;
        static constexpr size_t max_id  = sizeof...(Args);
        static constexpr size_t null_id = 0;

        template <typename T>
        static constexpr size_t id_from_type = UnionGetIDByType<T, Args...>::value;

        template <size_t ID>
        using type_from_id = typename UnionGetTypeByID<ID, Args...>::type;


        static void Release(int id, uint8_t *buf) {
            UnionReleaser<0, Args...>::Release(id, buf);
        }
};

/**
 * @brief No exception variant
 * 
 * @tparam Args 
 * 
 */
template <typename ...Args>
class Union {
    public:
        using Traits = UnionTraits<Args...>;

        /**
         * @brief Construct a new Union object (empty)
         * 
         */
        Union() = default;
        /**
         * @brief Construct a new Union object
         * 
         * @tparam T 
         * @param v 
         */
        template <typename T>
        Union(T &&v) {
            _id = Traits::template id_from_type<T>;
            new (_buffer) T(std::forward<T>(v));
        }
        Union(const Union &) = delete; //< TODO : Implement
        /**
         * @brief Destroy the Union object
         * 
         */
        ~Union() {
            reset();
        }


        void reset() {
            Traits::Release(_id, _buffer);
            _id = Traits::null_id;
        }

        template <typename T, typename ...NArgs>
        void emplace(NArgs &&...args) {
            reset();
            _id = Traits::template id_from_type<T>;
            new (_buffer) T(std::forward<NArgs>(args)...);
        }

        // Assignement
        template <typename T>
        Union &operator =(T &&v) {
            reset();
            _id = Traits::template id_from_type<T>;
            new (_buffer) T(std::forward<T>(v));
            return *this;
        }

        // Cast to type
        template <typename T>
        T &as() {
            assert(is_type<T>());
            return *reinterpret_cast<T*>(_buffer);
        }

        template <typename T>
        const T &as() const {
            assert(is_type<T>());
            return *reinterpret_cast<const T*>(_buffer);
        }

        template <typename T>
        bool is_type() const {
            return Traits::template id_from_type<T> == _id;
        }
    private:

        uint8_t _buffer[Traits::bufsize];
        int     _id   = Traits::null_id;
};

// Constructable to manmage object lifetime yourself
template <typename T>
class Constructable {
    public:
        Constructable() = default;
        Constructable(const Constructable &) = delete;
        Constructable(Constructable &&) = delete;
        ~Constructable() = default;

        template <typename ...Args>
        void construct(Args &&...args) {
            new (_buffer) T(std::forward<Args>(args)...);
        }
        void release() {
            reinterpret_cast<T*>(_buffer)->~T();
        }
    private:
        alignas(T) uint8_t _buffer[sizeof(T)];
};

// GlibPointer
#if __has_include(<glib.h>)

template <typename T>
class GPointer {
    public:
        explicit GPointer(T *ptr) : _ptr(ptr) {}
        GPointer() = default;
        GPointer(const GPointer & gp) : _ptr(gp._ptr) {
            if (_ptr) {
                g_object_ref(_ptr);
            }
        }
        GPointer(GPointer && gp) : _ptr(gp._ptr) {
            gp._ptr = nullptr;
        }
        ~GPointer() noexcept {
            reset();
        }

        T *get() const noexcept {
            return _ptr;
        }
        void reset() noexcept {
            if (_ptr) {
                g_object_unref(_ptr);
                _ptr = nullptr;
            }
        }
        T   *release() noexcept {
            auto p = _ptr;
            _ptr = nullptr;
            return p;
        }

        bool empty() const noexcept {
            return _ptr == nullptr;
        }

        GPointer &operator =(const GPointer &gp) noexcept {
            reset();
            _ptr = gp._ptr;
            if (_ptr) {
                g_object_ref(_ptr);
            }
            return *this;
        }
        GPointer &operator =(GPointer &&gp) noexcept {
            reset();
            _ptr = gp._ptr;
            gp._ptr = nullptr;
            return *this;
        }
        GPointer &operator =(T *p) noexcept {
            reset();
            _ptr = p;
            if (_ptr) {
                g_object_ref(_ptr);
            }
            return *this;
        }
    private:
        T *_ptr = nullptr;
};

#endif

} // namespace