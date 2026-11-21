#pragma once
#include <string>
namespace me {
struct DeathMarker { bool valid = false; float x = 0, y = 0; };
// Estatísticas persistentes entre execuções.
struct Stats {
    int bestScore = 0, lastKills = 0, games = 0, totalKills = 0;
    double lastTimeSec = 0;
    DeathMarker marker;
    void load(const std::string& path);
    bool save(const std::string& path) const;
    void resetStats() { bestScore = lastKills = games = totalKills = 0; lastTimeSec = 0; }  // não apaga o marcador
};
std::string formatTime(double sec);  // mm:ss ou h:mm:ss
}  // namespace me
