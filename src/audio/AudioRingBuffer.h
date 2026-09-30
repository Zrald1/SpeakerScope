#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

namespace ss {

// Single-producer / single-consumer lock-free ring of int16 samples.
// Producer: the miniaudio audio callback (must never block or allocate).
// Consumer: the sender thread teeing to STT + diarization.
class AudioRingBuffer {
public:
    explicit AudioRingBuffer(size_t capacity_samples = 16000 * 60)
        : buf_(capacity_samples) {}

    // Returns samples actually written (may be less than n when full).
    size_t push(const int16_t* data, size_t n) {
        const size_t w = write_.load(std::memory_order_relaxed);
        const size_t r = read_.load(std::memory_order_acquire);
        const size_t free = buf_.size() - (w - r);
        const size_t count = n < free ? n : free;
        for (size_t i = 0; i < count; ++i)
            buf_[(w + i) % buf_.size()] = data[i];
        write_.store(w + count, std::memory_order_release);
        return count;
    }

    size_t pop(int16_t* out, size_t n) {
        const size_t r = read_.load(std::memory_order_relaxed);
        const size_t w = write_.load(std::memory_order_acquire);
        const size_t avail = w - r;
        const size_t count = n < avail ? n : avail;
        for (size_t i = 0; i < count; ++i)
            out[i] = buf_[(r + i) % buf_.size()];
        read_.store(r + count, std::memory_order_release);
        return count;
    }

    size_t available() const {
        return write_.load(std::memory_order_acquire) -
               read_.load(std::memory_order_acquire);
    }

    void clear() {
        read_.store(write_.load(std::memory_order_acquire),
                    std::memory_order_release);
    }

private:
    std::vector<int16_t> buf_;
    std::atomic<size_t> write_{0};
    std::atomic<size_t> read_{0};
};

} // namespace ss
