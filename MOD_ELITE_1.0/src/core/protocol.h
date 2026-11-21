#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include "session.h"
#include "world.h"
namespace me {
namespace proto {
// ---- Pacotes de SAÍDA (1 byte cada) ----
constexpr uint8_t kPing = 251, kBoostOn = 253, kBoostOff = 254;
inline uint8_t angleToByte(float rad) {  // 0..250
    const float TWO_PI = 6.28318530718f;
    rad = std::fmod(rad, TWO_PI); if (rad < 0) rad += TWO_PI;
    int v = (int)(rad * 251.f / TWO_PI);
    return (uint8_t)(v > 250 ? 250 : v);
}
}  // namespace proto

// Interpreta pacotes de ENTRADA. Formato: [dt_hi][dt_lo][tipo ASCII][payload...].
// Implementados: 'p' (pong -> ping), 'v' (morte), 'k' (contagem de kills do nosso snake).
// NÃO IMPLEMENTADO (requer captura de tráfego real para validar layout): handshake, 'a','s','g','n','+','f' etc.
// Tipos desconhecidos apenas incrementam um contador (nada é simulado).
class GameClient {
public:
    GameClient(Session& s, World& w) : sess_(s), world_(w) {}
    void notePingSent(double now) { pingSent_ = now; waiting_ = true; }
    // Retorna o tipo do pacote (char) ou 0 se inválido.
    char onPacket(const uint8_t* d, size_t n, double now);
    int pingMs() const { return pingMs_; }
    unsigned long unknownPackets() const { return unknown_; }
    unsigned long totalPackets() const { return total_; }
private:
    Session& sess_; World& world_;
    double pingSent_ = 0; bool waiting_ = false; int pingMs_ = -1;
    unsigned long unknown_ = 0, total_ = 0;
};
}  // namespace me
