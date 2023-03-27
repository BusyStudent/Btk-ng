#pragma once

#include <Btk/defs.hpp>
#include <condition_variable>
#include <atomic>
#include <mutex>

BTK_NS_BEGIN

class Latch {
    public:
        Latch(uint32_t expected) : count(expected) { }
        Latch(const Latch &) = delete;
        ~Latch() = default;

        void count_down(uint32_t n = 1) {
            count -= n;
            if (count == 0) {
                cond.notify_all();
            }
        }
        void wait() {
            std::unique_lock lock(mtx);
            cond.wait(lock);
        }
    private:
        std::condition_variable cond;
        std::mutex               mtx;
        std::atomic<uint32_t>  count;
};

BTK_NS_END