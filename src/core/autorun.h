#pragma once
#include <vector>
#include "world.h"
namespace me {
struct AutoRunParams {
    float lookAhead = 320.f;       // alcance dos raios de segurança
    int rays = 36;                 // direções avaliadas
    float maxTurnRate = 2.4f;      // rad/s em condição normal (movimento suave)
    float emergencyFactor = 3.0f;  // multiplicador quando há perigo à frente
    float margin = 40.f;           // folga em volta de outros corpos
    float headMargin = 70.f;       // folga extra em volta de cabeças
    float borderMargin = 700.f;    // distância da borda em que começa a repulsão
    float cellSize = 300.f;        // grade de "áreas já visitadas"
    float visitedDecay = 0.15f;    // esquecimento por segundo
    float interval = 0.05f;        // reavalia a cada 50 ms (limita CPU)
};
// Piloto automático de sobrevivência. Usa SOMENTE o estado que o servidor já envia ao cliente
// (posição/corpo de outras cobras, raio do mapa). Interrompível a qualquer momento.
class AutoRun {
public:
    AutoRun() = default;
    explicit AutoRun(const AutoRunParams& p) : p_(p) {}
    bool active() const { return active_; }
    void toggle() { if (active_) { interrupt(); } else { active_ = true; accum_ = 1e9f; haveTarget_ = false; } }
    void interrupt() { active_ = false; haveTarget_ = false; }
    // Retorna false se inativo ou sem dados; senão devolve o ângulo alvo (rad).
    bool steer(const World& w, float dt, float& outAngle);
    const AutoRunParams& params() const { return p_; }
private:
    struct Obst { float x, y, r; };
    float clearance(Vec2 o, float th, float myR, float R) const;
    int cell(float x, float y) const;
    AutoRunParams p_;
    bool active_ = false, haveTarget_ = false;
    float accum_ = 0, decayAcc_ = 0, target_ = 0, R_ = 0;
    int gridN_ = 0;
    std::vector<float> visited_;
    std::vector<Obst> near_;  // reutilizado: sem alocação por frame
};
}  // namespace me
