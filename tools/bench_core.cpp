// Medições REAIS de CPU do núcleo (single thread). Não mede GPU/FPS de renderização.
#include <chrono>
#include <cstdio>
#include "../src/core/assist.h"
#include "../src/core/autorun.h"
#include "../src/core/minimap.h"
using namespace me;
using Clk = std::chrono::steady_clock;
static double us(Clk::time_point a, Clk::time_point b) { return std::chrono::duration<double, std::micro>(b - a).count(); }
int main() {
    World w; w.gameRadius = 21600; w.meId = 1;
    Snake m; m.id = 1; m.radius = 15; m.head = {0, 0}; w.snakes[1] = m;
    for (int s = 2; s <= 40; ++s) {  // 39 cobras, 300 pontos cada, espalhadas perto de nós (pior caso)
        Snake o; o.id = s; o.radius = 15; o.head = {float(s * 37 % 800 - 400), float(s * 91 % 800 - 400)};
        for (int i = 0; i < 300; ++i) o.body.push_back({o.head.x + i * 8.f, o.head.y + (i % 7) * 3.f});
        w.snakes[s] = o;
    }
    const int N = 20000; volatile float sink = 0; float t;
    { AssistLine l; Vec2 a, b; auto t0 = Clk::now(); for (int i = 0; i < N * 50; ++i) { l.compute({0, 0}, 1, 400, a, b); sink = sink + a.x; } auto t1 = Clk::now();
      std::printf("Linha DESLIGADA : %.4f us/chamada\n", us(t0, t1) / (N * 50)); }
    { AssistLine l; l.toggle(); Vec2 a, b; auto t0 = Clk::now(); for (int i = 0; i < N * 50; ++i) { l.compute({0, 0}, i * 0.001f, 400, a, b); sink = sink + b.x; } auto t1 = Clk::now();
      std::printf("Linha LIGADA    : %.4f us/chamada\n", us(t0, t1) / (N * 50)); }
    { AutoRunParams p; p.interval = 0.f; AutoRun a(p); a.toggle(); auto t0 = Clk::now(); for (int i = 0; i < 2000; ++i) { a.steer(w, 0.016f, t); sink = sink + t; } auto t1 = Clk::now();
      std::printf("AutoRun (reavalia todo frame, 39 cobras x 300 pts): %.1f us/chamada\n", us(t0, t1) / 2000); }
    { AutoRun a; a.toggle(); auto t0 = Clk::now(); for (int i = 0; i < 20000; ++i) { a.steer(w, 0.016f, t); sink = sink + t; } auto t1 = Clk::now();
      std::printf("AutoRun (padrão, reavalia a cada 50 ms), média por frame a 60 FPS: %.1f us\n", us(t0, t1) / 20000); }
    { Minimap mm; int upd = 0; auto t0 = Clk::now(); for (int i = 0; i < 20000; ++i) upd += mm.update(w, i * 0.016, true); auto t1 = Clk::now();
      std::printf("Minimapa (10 Hz): %.2f us/frame médio (%d recálculos em 20000 frames)\n", us(t0, t1) / 20000, upd); }
    { Minimap mm; auto t0 = Clk::now(); for (int i = 0; i < 2000000; ++i) mm.update(w, i * 0.016, false); auto t1 = Clk::now();
      std::printf("Minimapa oculto : %.4f us/frame\n", us(t0, t1) / 2000000); }
    std::printf("Compilado com -O2, máquina do sandbox Linux. Resultados NÃO representam o PC do usuário.\n");
    return 0;
}
