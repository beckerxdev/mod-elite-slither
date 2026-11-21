#include "session.h"
namespace me {
void Session::begin(double now) {
    alive_ = true; started_ = true; start_ = now; end_ = now;
    kills_ = 0; score_ = 0; posKnown_ = false;
    if (cfg_.markerClearOnNewGame) st_.marker.valid = false;
}
bool Session::onDeath(double now) {
    if (!alive_) return false;
    alive_ = false; end_ = now;
    st_.lastKills = kills_; st_.lastTimeSec = end_ - start_;
    st_.games++; st_.totalKills += kills_;
    if (posKnown_) { st_.marker.valid = true; st_.marker.x = pos_.x; st_.marker.y = pos_.y; }
    bool rec = score_ > st_.bestScore;
    if (rec) st_.bestScore = score_;
    return rec;
}
}  // namespace me
