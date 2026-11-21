#pragma once
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>
namespace me {
struct Vec2 { float x = 0, y = 0; };
struct Snake {
    int id = 0;
    Vec2 head;
    float angle = 0;        // rad
    float radius = 14;
    std::vector<Vec2> body; // pontos do corpo conhecidos (do mais próximo da cabeça ao rabo)
    bool alive = true;
};
struct Food { float x = 0, y = 0; uint8_t color = 0; float size = 1; };
// Estado do mundo conforme recebido do servidor. Nada aqui é simulado.
struct World {
    float gameRadius = 21600;
    int meId = -1;
    std::unordered_map<int, Snake> snakes;
    std::vector<Food> food;
    const Snake* me() const { auto it = snakes.find(meId); return it == snakes.end() ? nullptr : &it->second; }
};
}  // namespace me
