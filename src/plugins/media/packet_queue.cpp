#include "build.hpp"
#include "media_priv.hpp"

BTK_NS_BEGIN2(BTK_NAMESPACE::FFmpeg)

PacketQueue::PacketQueue() = default;
PacketQueue::~PacketQueue() {
    flush();
}

void PacketQueue::put(AVPacket *packet) {
    std::lock_guard locker(mutex);
    packets.push(packet);
    // Sums packet to let us known the buffered video
    if (!IsSpecialPacket(packet)) {
        packets_duration += packet->duration;
    }
    cond.notify_one();
}
void PacketQueue::flush() {
    std::lock_guard locker(mutex);
    while (!packets.empty()) {
        auto p = packets.front();
        if (!IsSpecialPacket(p)) {
            av_packet_free(&p);
        }
        packets.pop();
    }
    packets_duration = 0;
}

size_t PacketQueue::size() {
    std::lock_guard locker(mutex);
    return packets.size();
}
int64_t PacketQueue::duration() {
    std::lock_guard locker(mutex);
    return packets_duration;
}

AVPacket *PacketQueue::get(bool block) {
    AVPacket *ret = nullptr;
    mutex.lock();
    while (packets.empty()) {
        if (!block) {
            mutex.unlock();
            return nullptr;
        }

        mutex.unlock();

        std::unique_lock lock(cond_mutex);
        cond.wait(lock);

        mutex.lock();
    }
    ret = packets.front();
    packets.pop();
    if (!IsSpecialPacket(ret)) {
        packets_duration -= ret->duration;
    }

    mutex.unlock();

    return ret;
}

BTK_NS_END2(BTK_NAMESPACE::FFmpeg)
