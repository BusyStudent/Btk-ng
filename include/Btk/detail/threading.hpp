#pragma once

#include <Btk/defs.hpp>

#if __has_include(<pthread.h>)
#define BTK_HAS_PTHREAD
#include <pthread.h>
#else
#include <atomic>
#include <thread>
#endif

BTK_NS_BEGIN

// class SpinLock {
//     public:
//         SpinLock() = default;
//         SpinLock(const SpinLock &) = default;
//         SpinLock(SpinLock &&) = default;
//         ~SpinLock() = default;

//         void lock() {
//             while (lock_.test_and_set(std::memory_order_acquire)) {
//                 while (lock_.test_and_set(std::memory_order_acquire)) {
//                     std::this_thread::yield();
//                 }
//             }
//         }
//         void unlock() {
//             lock_.clear(std::memory_order_release);
//         }
        
//     private:
//         std::atomic_bool _lock = false;
// };
#if defined(BTK_HAS_PTHREAD)
// Pthread spinlock
class SpinLock {
    public:
        SpinLock() noexcept {
            pthread_spin_init(&_lock, 0);
        }
        SpinLock(const SpinLock &) = delete;
        ~SpinLock() {
            pthread_spin_destroy(&_lock);
        }

        void lock() noexcept {
            pthread_spin_lock(&_lock);
        }
        void unlock() noexcept {
            pthread_spin_unlock(&_lock);
        }
        bool try_lock() noexcept {
            return pthread_spin_trylock(&_lock) == 0;
        }
    private:
        pthread_spinlock_t _lock;
};
#else
// Fallback spinlock by std::atomic
class SpinLock {
    public:
        SpinLock() noexcept = default;
        SpinLock(const SpinLock &) = delete;
        ~SpinLock() = default;

        void lock() noexcept {
            while (_lock.test_and_set(std::memory_order_acquire)) {
                while (_lock.test_and_set(std::memory_order_acquire)) {
                    std::this_thread::yield();
                }
            }
        }
        void unlock() noexcept {
            _lock.clear(std::memory_order_release);
        }
        bool try_lock() noexcept {
            return _lock.test_and_set(std::memory_order_acquire) == false;
        }
    private:
        std::atomic_flag _lock = ATOMIC_FLAG_INIT;
};
#endif

BTK_NS_END