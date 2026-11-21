#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
namespace me {
std::string sha1(const std::string& data);            // 20 bytes brutos
std::string base64(const std::string& data);
std::string wsAcceptKey(const std::string& clientKey);  // RFC 6455

// Cliente WebSocket mínimo (ws://, sem TLS), Winsock/POSIX. Recebe em thread própria.
class WsClient {
public:
    using OnMessage = std::function<void(const uint8_t*, size_t, bool binary)>;
    using OnClose = std::function<void(const std::string& reason)>;
    ~WsClient() { close(); }
    bool connect(const std::string& host, int port, const std::string& path, const std::string& origin,
                 int timeoutMs, std::string& err);
    void start(OnMessage m, OnClose c);
    bool sendBinary(const uint8_t* d, size_t n);
    void close();
    bool isOpen() const { return open_; }
private:
    bool fill();
    bool readBytes(uint8_t* dst, size_t n);
    bool sendAll(const uint8_t* d, size_t n);
    void loop();
    intptr_t sock_ = -1;
    std::atomic<bool> open_{false};
    std::thread th_;
    std::mutex sendMu_;
    std::vector<uint8_t> rx_; size_t rxPos_ = 0;
    OnMessage onMsg_; OnClose onClose_;
};
}  // namespace me
