#pragma once
namespace me {
// Contador de FPS barato: uma soma por frame; o valor só é recalculado a cada 0,5 s.
class FpsCounter {
public:
    void frame(double dt) {
        acc_ += dt; ++n_;
        if (acc_ >= 0.5) { fps_ = int(n_ / acc_ + 0.5); acc_ = 0; n_ = 0; }
    }
    int fps() const { return fps_; }
private:
    double acc_ = 0; int n_ = 0, fps_ = 0;
};
}  // namespace me
