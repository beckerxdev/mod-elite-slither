#include "stats.h"
#include <cstdio>
#include "ini.h"
namespace me {
void Stats::load(const std::string& p) {
    Ini i; if (!i.load(p)) return;
    bestScore = (int)i.getInt("bestScore", 0); lastKills = (int)i.getInt("lastKills", 0);
    games = (int)i.getInt("games", 0); totalKills = (int)i.getInt("totalKills", 0);
    lastTimeSec = i.getFloat("lastTimeSec", 0);
    marker.valid = i.getBool("marker.valid", false);
    marker.x = (float)i.getFloat("marker.x", 0); marker.y = (float)i.getFloat("marker.y", 0);
}
bool Stats::save(const std::string& p) const {
    Ini i;
    i.setInt("bestScore", bestScore); i.setInt("lastKills", lastKills); i.setInt("games", games);
    i.setInt("totalKills", totalKills); i.setFloat("lastTimeSec", lastTimeSec);
    i.setBool("marker.valid", marker.valid); i.setFloat("marker.x", marker.x); i.setFloat("marker.y", marker.y);
    return i.save(p);
}
std::string formatTime(double s) {
    if (s < 0) s = 0;
    long t = (long)s; char b[32];
    if (t >= 3600) std::snprintf(b, sizeof b, "%ld:%02ld:%02ld", t / 3600, (t / 60) % 60, t % 60);
    else std::snprintf(b, sizeof b, "%02ld:%02ld", t / 60, t % 60);
    return b;
}
}  // namespace me
