#include "ws_client.h"
#include <cstring>
#include <random>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET sock_t;
static void closeSock(SOCKET s) { ::closesocket(s); }
static bool netInit() { static bool ok = [] { WSADATA d; return WSAStartup(MAKEWORD(2, 2), &d) == 0; }(); return ok; }
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int sock_t;
static void closeSock(int s) { ::close(s); }
static bool netInit() { return true; }
#endif
namespace me {

// ---------- SHA-1 / Base64 (necessários para validar o handshake RFC 6455) ----------
static uint32_t rol(uint32_t v, int b) { return (v << b) | (v >> (32 - b)); }
std::string sha1(const std::string& msg) {
    uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476, h4 = 0xC3D2E1F0;
    std::string m = msg; uint64_t bits = (uint64_t)msg.size() * 8;
    m.push_back((char)0x80);
    while (m.size() % 64 != 56) m.push_back(0);
    for (int i = 7; i >= 0; --i) m.push_back((char)((bits >> (i * 8)) & 0xFF));
    for (size_t off = 0; off < m.size(); off += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i)
            w[i] = ((uint32_t)(uint8_t)m[off + i * 4] << 24) | ((uint32_t)(uint8_t)m[off + i * 4 + 1] << 16) |
                   ((uint32_t)(uint8_t)m[off + i * 4 + 2] << 8) | (uint32_t)(uint8_t)m[off + i * 4 + 3];
        for (int i = 16; i < 80; ++i) w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }
            uint32_t t = rol(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rol(b, 30); b = a; a = t;
        }
        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
    }
    std::string out; uint32_t hs[5] = {h0, h1, h2, h3, h4};
    for (uint32_t h : hs) for (int i = 3; i >= 0; --i) out.push_back((char)((h >> (i * 8)) & 0xFF));
    return out;
}
std::string base64(const std::string& in) {
    static const char* T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string o; size_t i = 0;
    for (; i + 2 < in.size(); i += 3) {
        uint32_t v = ((uint8_t)in[i] << 16) | ((uint8_t)in[i + 1] << 8) | (uint8_t)in[i + 2];
        o += T[v >> 18]; o += T[(v >> 12) & 63]; o += T[(v >> 6) & 63]; o += T[v & 63];
    }
    if (i + 1 == in.size()) { uint32_t v = (uint8_t)in[i] << 16; o += T[v >> 18]; o += T[(v >> 12) & 63]; o += "=="; }
    else if (i + 2 == in.size()) {
        uint32_t v = ((uint8_t)in[i] << 16) | ((uint8_t)in[i + 1] << 8);
        o += T[v >> 18]; o += T[(v >> 12) & 63]; o += T[(v >> 6) & 63]; o += '=';
    }
    return o;
}
std::string wsAcceptKey(const std::string& k) { return base64(sha1(k + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")); }

// ---------- Socket ----------
bool WsClient::sendAll(const uint8_t* d, size_t n) {
    while (n) {
        int r = (int)::send((sock_t)sock_, (const char*)d, (int)n, 0);
        if (r <= 0) return false;
        d += r; n -= (size_t)r;
    }
    return true;
}
bool WsClient::fill() {
    if (rxPos_ > 0 && rxPos_ == rx_.size()) { rx_.clear(); rxPos_ = 0; }
    uint8_t buf[8192];
    int r = (int)::recv((sock_t)sock_, (char*)buf, sizeof buf, 0);
    if (r <= 0) return false;
    rx_.insert(rx_.end(), buf, buf + r);
    return true;
}
bool WsClient::readBytes(uint8_t* dst, size_t n) {
    while (rx_.size() - rxPos_ < n) if (!fill()) return false;
    std::memcpy(dst, rx_.data() + rxPos_, n); rxPos_ += n;
    return true;
}

bool WsClient::connect(const std::string& host, int port, const std::string& path, const std::string& origin,
                       int timeoutMs, std::string& err) {
    close();
    if (!netInit()) { err = "Falha ao iniciar a rede."; return false; }
    addrinfo hints{}, *res = nullptr; hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0 || !res) { err = "Endereço não resolvido."; return false; }
    sock_t s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
#ifdef _WIN32
    if (s == INVALID_SOCKET) { freeaddrinfo(res); err = "Falha ao criar socket."; return false; }
    u_long nb = 1; ioctlsocket(s, FIONBIO, &nb);
#else
    if (s < 0) { freeaddrinfo(res); err = "Falha ao criar socket."; return false; }
    fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
#endif
    ::connect(s, res->ai_addr, (int)res->ai_addrlen);
    freeaddrinfo(res);
    fd_set wf; FD_ZERO(&wf); FD_SET(s, &wf);
    timeval tv{timeoutMs / 1000, (timeoutMs % 1000) * 1000};
    if (select((int)s + 1, nullptr, &wf, nullptr, &tv) <= 0) { closeSock(s); err = "Tempo de conexão esgotado."; return false; }
    int soerr = 0; socklen_t sl = sizeof soerr; getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&soerr, &sl);
    if (soerr) { closeSock(s); err = "Conexão recusada."; return false; }
#ifdef _WIN32
    nb = 0; ioctlsocket(s, FIONBIO, &nb);
    DWORD to = timeoutMs; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&to, sizeof to);
#else
    fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) & ~O_NONBLOCK);
    timeval rt{timeoutMs / 1000, (timeoutMs % 1000) * 1000}; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &rt, sizeof rt);
#endif
    int one = 1; setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)&one, sizeof one);
    sock_ = (intptr_t)s; rx_.clear(); rxPos_ = 0;

    std::mt19937 rng{std::random_device{}()};
    std::string raw(16, 0); for (auto& c : raw) c = (char)(rng() & 0xFF);
    std::string key = base64(raw);
    std::string req = "GET " + path + " HTTP/1.1\r\nHost: " + host + ":" + std::to_string(port) +
                      "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: " + key +
                      "\r\nSec-WebSocket-Version: 13\r\nOrigin: " + origin + "\r\n\r\n";
    if (!sendAll((const uint8_t*)req.data(), req.size())) { err = "Falha ao enviar handshake."; closeSock(s); sock_ = -1; return false; }
    // Lê cabeçalho HTTP até \r\n\r\n (bytes extras ficam no buffer para o 1º frame).
    std::string hdr; size_t endPos = std::string::npos;
    while (endPos == std::string::npos) {
        if (rxPos_ == rx_.size() && !fill()) { err = "Servidor fechou durante o handshake."; closeSock(s); sock_ = -1; return false; }
        hdr.assign((const char*)rx_.data(), rx_.size());
        endPos = hdr.find("\r\n\r\n");
        if (endPos == std::string::npos && !fill()) { err = "Handshake incompleto."; closeSock(s); sock_ = -1; return false; }
        if (hdr.size() > 16384) { err = "Resposta de handshake inválida."; closeSock(s); sock_ = -1; return false; }
    }
    rxPos_ = endPos + 4;
    hdr.resize(endPos);
    if (hdr.find(" 101") == std::string::npos) { err = "Servidor recusou o upgrade WebSocket."; closeSock(s); sock_ = -1; return false; }
    std::string want = wsAcceptKey(key), low = hdr;
    for (auto& c : low) c = (char)std::tolower((unsigned char)c);
    auto p = low.find("sec-websocket-accept:");
    if (p == std::string::npos) { err = "Resposta sem Sec-WebSocket-Accept."; closeSock(s); sock_ = -1; return false; }
    std::string got = hdr.substr(p + 21, hdr.find("\r\n", p) == std::string::npos ? std::string::npos : hdr.find("\r\n", p) - (p + 21));
    while (!got.empty() && got.front() == ' ') got.erase(got.begin());
    while (!got.empty() && (got.back() == ' ' || got.back() == '\r')) got.pop_back();
    if (got != want) { err = "Sec-WebSocket-Accept inválido."; closeSock(s); sock_ = -1; return false; }
    // Sem timeout de leitura durante o jogo (thread de recepção bloqueia).
#ifdef _WIN32
    DWORD z = 0; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&z, sizeof z);
#else
    timeval zt{0, 0}; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &zt, sizeof zt);
#endif
    open_ = true;
    return true;
}

bool WsClient::sendBinary(const uint8_t* d, size_t n) {
    if (!open_) return false;
    std::vector<uint8_t> f; f.reserve(n + 14);
    f.push_back(0x82);
    if (n < 126) f.push_back(uint8_t(0x80 | n));
    else if (n < 65536) { f.push_back(0x80 | 126); f.push_back(uint8_t(n >> 8)); f.push_back(uint8_t(n)); }
    else { f.push_back(0x80 | 127); for (int i = 7; i >= 0; --i) f.push_back(uint8_t((uint64_t)n >> (i * 8))); }
    static thread_local std::mt19937 rng{std::random_device{}()};
    uint8_t mk[4]; for (auto& b : mk) b = (uint8_t)(rng() & 0xFF);
    f.insert(f.end(), mk, mk + 4);
    for (size_t i = 0; i < n; ++i) f.push_back(d[i] ^ mk[i & 3]);
    std::lock_guard<std::mutex> lk(sendMu_);
    return sendAll(f.data(), f.size());
}

void WsClient::start(OnMessage m, OnClose c) {
    onMsg_ = std::move(m); onClose_ = std::move(c);
    th_ = std::thread([this] { loop(); });
}

void WsClient::loop() {
    std::vector<uint8_t> msg; int msgOp = 0; std::vector<uint8_t> pl; std::string reason = "Conexão encerrada.";
    while (open_) {
        uint8_t h[2];
        if (!readBytes(h, 2)) break;
        bool fin = h[0] & 0x80; int op = h[0] & 0x0F; bool masked = h[1] & 0x80; uint64_t len = h[1] & 0x7F;
        if (len == 126) { uint8_t e[2]; if (!readBytes(e, 2)) break; len = (e[0] << 8) | e[1]; }
        else if (len == 127) { uint8_t e[8]; if (!readBytes(e, 8)) break; len = 0; for (int i = 0; i < 8; ++i) len = (len << 8) | e[i]; }
        if (len > (16u << 20)) { reason = "Frame grande demais."; break; }
        uint8_t mk[4] = {0, 0, 0, 0};
        if (masked && !readBytes(mk, 4)) break;
        pl.resize((size_t)len);
        if (len && !readBytes(pl.data(), (size_t)len)) break;
        if (masked) for (size_t i = 0; i < pl.size(); ++i) pl[i] ^= mk[i & 3];
        if (op == 0x8) { reason = "Servidor encerrou a conexão."; break; }
        if (op == 0x9) {  // ping -> pong
            std::vector<uint8_t> f{0x8A, uint8_t(0x80 | pl.size())}; uint8_t m4[4] = {1, 2, 3, 4};
            if (pl.size() < 126) { f.insert(f.end(), m4, m4 + 4); for (size_t i = 0; i < pl.size(); ++i) f.push_back(pl[i] ^ m4[i & 3]); std::lock_guard<std::mutex> lk(sendMu_); sendAll(f.data(), f.size()); }
            continue;
        }
        if (op == 0xA) continue;
        if (op == 0x1 || op == 0x2) { msg = pl; msgOp = op; } else if (op == 0x0) msg.insert(msg.end(), pl.begin(), pl.end());
        if (fin && onMsg_) { onMsg_(msg.data(), msg.size(), msgOp == 0x2); msg.clear(); }
    }
    bool wasOpen = open_.exchange(false);
    if (wasOpen && onClose_) onClose_(reason);
}

void WsClient::close() {
    bool was = open_.exchange(false);
    if (sock_ != -1) {
#ifdef _WIN32
        shutdown((sock_t)sock_, SD_BOTH);
#else
        shutdown((sock_t)sock_, SHUT_RDWR);
#endif
    }
    if (th_.joinable() && std::this_thread::get_id() != th_.get_id()) th_.join();
    if (sock_ != -1) { closeSock((sock_t)sock_); sock_ = -1; }
    (void)was;
}
}  // namespace me
