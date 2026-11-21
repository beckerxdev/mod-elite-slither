// Conecta, envia 3 mensagens binárias (pequena, 300 B, 70000 B) e confere o eco do servidor de teste.
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include "../src/net/ws_client.h"
#include "test_util.h"
using namespace me;
int main(int argc, char** argv) {
    SECTION("WebSocket: SHA-1 / Base64 / Accept (vetor RFC 6455)");
    CHECK(wsAcceptKey("dGhlIHNhbXBsZSBub25jZQ==") == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
    CHECK(base64("Man") == "TWFu" && base64("Ma") == "TWE=" && base64("M") == "TQ==");
    CHECK(sha1("abc").size() == 20);
    if (argc >= 2) {
        int port = std::atoi(argv[1]);
        SECTION("WebSocket: conexão real contra servidor de teste");
        WsClient c; std::string err;
        CHECK(c.connect("127.0.0.1", port, "/slither", "https://slither.com", 3000, err));
        if (!err.empty()) std::printf("  erro: %s\n", err.c_str());
        std::mutex m; std::condition_variable cv; std::vector<std::vector<uint8_t>> got; std::string closeReason;
        c.start([&](const uint8_t* d, size_t n, bool bin) { std::lock_guard<std::mutex> l(m); if (bin) got.emplace_back(d, d + n); cv.notify_all(); },
                [&](const std::string& r) { std::lock_guard<std::mutex> l(m); closeReason = r; cv.notify_all(); });
        size_t sizes[] = {5, 300, 70000};
        for (size_t sz : sizes) {
            std::vector<uint8_t> v(sz); for (size_t i = 0; i < sz; ++i) v[i] = uint8_t(i * 7 + sz);
            CHECK(c.sendBinary(v.data(), v.size()));
            std::unique_lock<std::mutex> l(m);
            bool ok = cv.wait_for(l, std::chrono::seconds(3), [&] { return !got.empty(); });
            CHECK(ok);
            if (ok) { CHECK(got.back() == v); got.clear(); }
        }
        // O servidor envia uma mensagem fragmentada + um ping e depois fecha
        const char cmd = 'F'; c.sendBinary((const uint8_t*)&cmd, 1);
        { std::unique_lock<std::mutex> l(m); bool ok = cv.wait_for(l, std::chrono::seconds(3), [&] { return !got.empty(); });
          CHECK(ok && got.back().size() == 10 && got.back()[0] == 'A' && got.back()[9] == 'J'); got.clear(); }
        const char cl = 'C'; c.sendBinary((const uint8_t*)&cl, 1);
        { std::unique_lock<std::mutex> l(m); bool ok = cv.wait_for(l, std::chrono::seconds(3), [&] { return !closeReason.empty(); });
          CHECK(ok); std::printf("   (info) close: %s\n", closeReason.c_str()); }
        c.close();
        // Conexão recusada
        WsClient bad; std::string e2; CHECK(!bad.connect("127.0.0.1", 1, "/", "x", 1000, e2)); std::printf("   (info) erro esperado: %s\n", e2.c_str());
    }
    std::printf("\nResultado: %d OK, %d falhas\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
