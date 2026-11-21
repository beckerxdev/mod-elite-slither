#include "protocol.h"
namespace me {
char GameClient::onPacket(const uint8_t* d, size_t n, double now) {
    if (n < 3) return 0;
    ++total_;
    char t = (char)d[2];
    switch (t) {
        case 'p':
            if (waiting_) { pingMs_ = (int)((now - pingSent_) * 1000.0 + 0.5); waiting_ = false; }
            break;
        case 'v':
            sess_.onDeath(now);
            break;
        case 'k':  // [id_hi][id_lo][kills(3 bytes, big-endian)]  -- layout a validar com tráfego real
            if (n >= 8) {
                int id = (d[3] << 8) | d[4];
                int kills = (d[5] << 16) | (d[6] << 8) | d[7];
                if (id == world_.meId) sess_.onServerKillCount(kills);
            }
            break;
        default:
            ++unknown_;
    }
    return t;
}
}  // namespace me
