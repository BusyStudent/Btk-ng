#pragma once

#include <Btk/defs.hpp>

#if __has_include(<pthread.h>)
#define BTK_HAS_PTHREAD
#include <pthread.h>
#endif

#include <atomic>
#include <thread>
#include <mutex>

BTK_NS_BEGIN

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

#elif !defined(BTK_NO_BUILTIN_SPINLOCK)
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

#else

using SpinLock = std::mutex;

#endif

static_assert(std::atomic<std::thread::id>::is_always_lock_free, "atomic<thread::id> should be lock free");

/**
 * @brief Recursive spinlock
 * 
 */
class RSpinLock {
    public:
        RSpinLock() = default;
        RSpinLock(const RSpinLock &) = default; 
        ~RSpinLock() = default;

        void lock() noexcept {
            auto this_id = std::this_thread::get_id();
            if (_owner != this_id && _counting > 0) {
                // Wait release 
                while (_counting != 0) {
                    std::this_thread::yield();
                }
            }
            _owner = this_id;
            ++_counting;
        }
        void unlock() noexcept {
            --_counting;
        }
        bool try_lock() {
            auto this_id = std::this_thread::get_id();
            if (_counting > 0 && _owner != this_id) {
                return false;
            }
            lock();
            return true;
        }
    private:
        std::atomic<std::thread::id> _owner = {};
        std::atomic_int32_t          _counting = 0;
};

BTK_NS_END