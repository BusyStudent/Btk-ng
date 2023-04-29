#pragma once

#include <Btk/defs.hpp>

#if !BTK_CXX20
#include <condition_variable>
#include <atomic>
#include <mutex>

BTK_NS_BEGIN

class Latch {
    public:
        Latch(ptrdiff_t expected) noexcept : count(expected) { }
        Latch(const Latch &) = delete;
        ~Latch() = default;

        void count_down(ptrdiff_t n = 1) noexcept {
            ptrdiff_t current = count.fetch_sub(n) - n;
            if (current == 0) {
                cond.notify_all();
            }
        }
        void wait() const {
            for (;;) {
                if (count.load() == 0) {
                    return;
                }

                std::unique_lock lock(mtx);
                cond.wait(lock);
            }
        }
        bool try_wait() const noexcept {
            return count.load() == 0;
        }
        void arrive_and_wait(ptrdiff_t n) {
            ptrdiff_t current = count.fetch_sub(n) - n;
            if (current == 0) {
                cond.notify_all();
            }
            else {
                wait();
            }
        }
    private:
        mutable std::condition_variable cond;
        mutable std::mutex               mtx;
        std::atomic<ptrdiff_t>          count;
};

BTK_NS_END

#else
#include <barrier>
#include <latch>

BTK_NS_BEGIN

using Latch   = std::latch;
using Barrier = std::barrier;

BTK_NS_END

#endif