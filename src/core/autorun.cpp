#include "autorun.h"
#include <algorithm>
#include <cmath>
namespace me {
static const float PI = 3.14159265358979f;
static float wrapA(float a) { while (a > PI) a -= 2 * PI; while (a < -PI) a += 2 * PI; return a; }

int AutoRun::cell(float x, float y) const {
    int cx = (int)((x + R_) / p_.cellSize), cy = (int)((y + R_) / p_.cellSize);
    cx = std::max(0, std::min(gridN_ - 1, cx)); cy = std::max(0, std::min(gridN_ - 1, cy));
    return cy * gridN_ + cx;
}

float AutoRun::clearance(Vec2 o, float th, float myR, float R) const {
    const float step = 30.f, L = p_.lookAhead;
    const float cx = std::cos(th), sy = std::sin(th);
    const float lim = R - myR - 20.f, lim2 = lim * lim;
    for (float t = step; t <= L; t += step) {
        float px = o.x + cx * t, py = o.y + sy * t;
        if (px * px + py * py > lim2) return t - step;
        for (const Obst& q : near_) {
            float dx = px - q.x, dy = py - q.y, l = myR + q.r;
            if (dx * dx + dy * dy < l * l) return t - step;
        }
    }
    return L;
}

bool AutoRun::steer(const World& w, float dt, float& out) {
    if (!active_) return false;
    const Snake* me = w.me();
    if (!me) return false;
    float R = w.gameRadius;
    if (R_ != R || visited_.empty()) {
        R_ = R; gridN_ = (int)std::ceil(2 * R / p_.cellSize) + 1;
        visited_.assign((size_t)gridN_ * gridN_, 0.f);
    }
    visited_[cell(me->head.x, me->head.y)] += dt;
    decayAcc_ += dt;
    if (decayAcc_ >= 1.f) { float k = 1.f - p_.visitedDecay; for (float& v : visited_) v *= k; decayAcc_ = 0; }

    accum_ += dt;
    if (haveTarget_ && accum_ < p_.interval) { out = target_; return true; }
    accum_ = 0;

    // Obstáculos relevantes (buffer reutilizado).
    near_.clear();
    const float reach = p_.lookAhead + me->radius + 150.f;
    for (const auto& kv : w.snakes) {
        const Snake& s = kv.second;
        if (s.id == me->id || !s.alive) continue;
        auto add = [&](Vec2 p, float extra) {
            if (std::fabs(p.x - me->head.x) <= reach && std::fabs(p.y - me->head.y) <= reach)
                near_.push_back({p.x, p.y, s.radius + extra});
        };
        add(s.head, p_.headMargin);
        Vec2 pred{s.head.x + std::cos(s.angle) * p_.lookAhead * 0.5f, s.head.y + std::sin(s.angle) * p_.lookAhead * 0.5f};
        add(pred, p_.headMargin);
        for (const Vec2& b : s.body) add(b, p_.margin);
    }

    const float heading = me->angle;
    float bestScore = -1e9f, bestAng = heading;
    const int n = p_.rays & ~1;
    for (int k = 0; k < n; ++k) {
        float th = heading + (k - n / 2) * (2 * PI / n);
        float c = clearance(me->head, th, me->radius, R) / p_.lookAhead;
        float ex = me->head.x + std::cos(th) * p_.lookAhead * 0.8f, ey = me->head.y + std::sin(th) * p_.lookAhead * 0.8f;
        float vis = std::min(visited_[cell(ex, ey)] / 3.f, 1.f);
        float rr = std::sqrt(ex * ex + ey * ey);
        float bp = std::min(1.5f, std::max(0.f, (rr - (R - p_.borderMargin)) / p_.borderMargin));
        float turn = std::fabs(wrapA(th - heading)) / PI;
        float sc = 5.f * c - 1.0f * vis - 2.0f * bp - 0.6f * turn;
        if (sc > bestScore) { bestScore = sc; bestAng = th; }
    }
    float aheadClear = clearance(me->head, heading, me->radius, R) / p_.lookAhead;
    float rate = p_.maxTurnRate * (aheadClear < 0.6f ? p_.emergencyFactor : 1.f);
    float maxStep = rate * std::max(p_.interval, 0.001f);
    float d = std::max(-maxStep, std::min(maxStep, wrapA(bestAng - heading)));
    target_ = heading + d;
    haveTarget_ = true;
    out = target_;
    return true;
}
}  // namespace me
