#pragma once
#include <cmath>
#include <vector>
#include "stats.h"
#include "world.h"
namespace me {
struct MapDot { float x, y; };  // coordenadas normalizadas 0..1
class Minimap {
public:
    void setInterval(double s) { interval_ = s; }
    // Só recalcula se visível e se passou o intervalo (padrão 10 Hz). Retorna true se recalculou.
    bool update(const World& w, double now, bool visible) {
        if (!visible) return false;
        if (haveData_ && now - last_ < interval_) return false;
        last_ = now; haveData_ = true; ++updates_;
        others_.clear();
        hasMe_ = false;
        const Snake* me = w.me();
        if (me) { hasMe_ = true; me_ = norm(me->head, w.gameRadius); meAngle_ = me->angle; }
        for (const auto& kv : w.snakes)
            if (kv.first != w.meId && kv.second.alive) others_.push_back(norm(kv.second.head, w.gameRadius));
        return true;
    }
    static MapDot norm(Vec2 p, float R) { return {0.5f + p.x / (2 * R), 0.5f + p.y / (2 * R)}; }
    const std::vector<MapDot>& others() const { return others_; }
    bool hasMe() const { return hasMe_; }
    MapDot me() const { return me_; }
    float meAngle() const { return meAngle_; }
    unsigned long updates() const { return updates_; }
    static float distanceToMarker(Vec2 me, const DeathMarker& m) {
        float dx = me.x - m.x, dy = me.y - m.y; return std::sqrt(dx * dx + dy * dy);
    }
private:
    double interval_ = 0.1, last_ = 0; bool haveData_ = false, hasMe_ = false;
    MapDot me_{}; float meAngle_ = 0; unsigned long updates_ = 0;
    std::vector<MapDot> others_;
};
}  // namespace me
