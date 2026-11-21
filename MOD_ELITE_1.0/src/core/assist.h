#pragma once
#include "world.h"
namespace me {
// Linha de assistência: SEMPRE inicia desligada, só muda por toggle explícito e não é persistida.
// Desligada => compute() retorna imediatamente, sem nenhum cálculo.
class AssistLine {
public:
    bool active() const { return on_; }
    void toggle() { on_ = !on_; }
    void set(bool v) { on_ = v; }
    bool compute(Vec2 head, float angle, float length, Vec2& a, Vec2& b) {
        if (!on_) return false;
        ++computeCount_;
        a = head;
        b.x = head.x + std::cos(angle) * length;
        b.y = head.y + std::sin(angle) * length;
        return true;
    }
    unsigned long computeCount() const { return computeCount_; }
private:
    bool on_ = false;
    unsigned long computeCount_ = 0;
};
}  // namespace me
