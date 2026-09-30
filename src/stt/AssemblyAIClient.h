#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include "stt/StreamingMessages.h"

namespace ix {
class WebSocket;
}

namespace ss {

struct Config;

// Thin lifecycle wrapper around the AssemblyAI v3 streaming WebSocket.
// - connect() opens wss://streaming.assemblyai.com/v3/ws with session params
// - sendAudio() pushes raw binary PCM frames (50 ms s16le mono)
// - terminate() sends {"type":"Terminate"} and closes cleanly (billing!)
class AssemblyAIClient {
public:
    using EventCallback = std::function<void(const ServerEvent&)>;

    AssemblyAIClient();
    ~AssemblyAIClient();

    bool connect(const Config& cfg, EventCallback cb);
    void sendAudio(const int16_t* samples, size_t count);
    void terminate();

    bool isOpen() const { return open_.load(); }
    const std::string& lastError() const { return last_error_; }

    AssemblyAIClient(const AssemblyAIClient&) = delete;
    AssemblyAIClient& operator=(const AssemblyAIClient&) = delete;

private:
    std::unique_ptr<ix::WebSocket> ws_;
    EventCallback cb_;
    std::atomic<bool> open_{false};
    std::atomic<uint64_t> bytes_sent_{0};
    std::string last_error_;
};

} // namespace ss
