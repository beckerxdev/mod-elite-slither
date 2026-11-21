#pragma once
#include "settings.h"
#include "stats.h"
#include "world.h"
namespace me {
// Controla uma partida: cronômetro, kills confirmadas, recorde e marcador da última morte.
// Todos os tempos vêm de fora (injeção de relógio) para ser testável e independente do SO.
class Session {
public:
    Session(Stats& st, const Settings& cfg) : st_(st), cfg_(cfg) {}
    void begin(double now);                 // nova partida: zera cronômetro e kills
    void updateScore(int s) { if (alive_) score_ = s; }
    void updatePos(Vec2 p) { if (alive_) { pos_ = p; posKnown_ = true; } }
    // Kills só são confirmadas quando o servidor informa o total do NOSSO snake (nunca por "cobra morreu").
    void onServerKillCount(int total) { if (alive_ && total > kills_) kills_ = total; }
    bool onDeath(double now);               // retorna true se bateu recorde
    bool alive() const { return alive_; }
    int kills() const { return kills_; }
    int score() const { return score_; }
    double elapsed(double now) const { return started_ ? (alive_ ? now - start_ : end_ - start_) : 0.0; }
    void clearMarker() { st_.marker.valid = false; }
    Vec2 pos() const { return pos_; }
    bool posKnown() const { return posKnown_; }
private:
    Stats& st_; const Settings& cfg_;
    bool alive_ = false, started_ = false, posKnown_ = false;
    double start_ = 0, end_ = 0; int kills_ = 0, score_ = 0; Vec2 pos_;
};
}  // namespace me
