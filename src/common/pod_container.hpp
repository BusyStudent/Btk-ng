#pragma once

#include "build.hpp"
#include <type_traits>

BTK_NS_BEGIN

// Use in factory register
template <typename T>
class CompressedDict {
    private:
        struct Elem {
            T second;
            char first[];

            size_t size() {
                return sizeof(second) + std::strlen(first) + 1;
            }
        };
        // For struct binding like this auto [first, second] = xxx
        struct Result {
            const char *first;
            T         &second;
        };

        Elem *index(int pos) {
            return reinterpret_cast<Elem*>(
                reinterpret_cast<uint8_t*>(_values) + pos
            );
        }

        Elem *_values = nullptr;
        int   _end   = 0; //< Current End in byte
    public:
        inline CompressedDict() = default;
        inline ~CompressedDict() = default;

        void push_back(u8string_view view, const T &value) {
            // Realloc the size
            size_t elem_size = sizeof(T) + view.size() + 1;
            _values =  (Elem*) Btk_realloc(_values, _end + elem_size);

            // AddValue
            auto cur = index(_end);
            new(&cur->second) T(value);
            Btk_memcpy(cur->first, view.data(), view.size());
            cur->first[view.size()] = '\0';

            _end += elem_size;
        }
        bool  empty() {
            return _values == nullptr;
        }

        struct iterator {

            void operator ++() {
                reinterpret_cast<uint8_t*&>(cur) += cur->size();
            }
            bool operator ==(iterator a) const {
                return cur == a.cur;
            }
            bool operator !=(iterator a) const {
                return cur != a.cur;
            }
            Result operator *() {
                return {
                    cur->first,
                    cur->second
                };
            }

            Elem *cur;
        };
        Result back() {
            iterator iter = begin();
            iterator prev;
            for (; iter != end(); ++iter) {
                prev = iter;
            }
            return *prev;
        }
        Result front() {
            auto cur = index(0);
            return {
                cur->first,
                cur->Second
            };
        }

        iterator begin() {
            return { index(0) };
        }
        iterator end() {
            return { index(_end) };
        }
};

// For erase ptr type
template <typename T>
class CompressedPtrDict {
    public:
        inline CompressedPtrDict() = default;
        inline ~CompressedPtrDict() = default;

        struct Result {
            const char *first;
            T           second;
        };
        struct iterator {
            typename CompressedDict<void*>::iterator i;
            void operator ++() {
                ++i;
            }
            bool operator !=(iterator other) const {
                return i != other.i;
            }
            bool operator ==(iterator other) const {
                return i == other.i;
            }
            Result operator *() {
                auto [first, second] = *i;
                return {
                    first,
                    reinterpret_cast<T>(second)
                };
            }
        };
        iterator begin() {
            return {d.begin()};
        }
        iterator end() {
            return {d.end()};
        }
        Result back() {
            auto [first, second] = d.back();
            return {
                first,
                reinterpret_cast<T>(second)
            };
        }
        bool empty() {
            return d.empty();
        }
        void push_back(u8string_view view, const T &value) {
            d.push_back(view, reinterpret_cast<void*>(value));
        }
    private:
        static_assert(std::is_pointer_v<T>);
        CompressedDict<void*> d;
};
BTK_NS_END