#pragma once
#include <cmath>
#include <vector>
namespace me {
// Pontuação exibida pelo jogo: derivada do comprimento (sct) e da fração (fam),
// com as tabelas fpsls/fmlts calculadas a partir de mscps (enviado pelo servidor).
// ATENÇÃO: fórmula conforme comportamento público do cliente; validar contra um servidor real.
class ScoreTable {
public:
    explicit ScoreTable(int mscps = 411) { init(mscps); }
    void init(int mscps) {
        if (mscps < 2) mscps = 2;
        mscps_ = mscps;
        fm_.assign(mscps + 1, 1.0); fp_.assign(mscps + 1, 0.0);
        for (int i = 0; i <= mscps; ++i) fm_[i] = std::pow(1.0 - double(i) / mscps, 2.25);
        for (int i = 1; i <= mscps; ++i) fp_[i] = fp_[i - 1] + 1.0 / fm_[i - 1];
    }
    int score(int sct, double fam) const {
        if (sct < 0) sct = 0;
        if (sct >= mscps_) sct = mscps_ - 1;
        double s = std::floor(15.0 * (fp_[sct] + fam / fm_[sct] - 1.0) - 5.0);
        return s < 0 ? 0 : int(s);
    }
private:
    int mscps_ = 411;
    std::vector<double> fm_, fp_;
};
}  // namespace me
