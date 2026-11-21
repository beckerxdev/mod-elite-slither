#pragma once
#include <string>
namespace me {
struct Rgb { float r, g, b; };
// Skin local. 'gameSkinId' é o ID enviado ao servidor (o que os outros jogadores veem);
// o padrão/cores personalizadas são a aparência LOCAL (somente no seu cliente).
struct SkinSpec {
    enum Pattern { Solid = 0, Alternating, Stripes, Gradient, PatternCount };
    int gameSkinId = 0;
    int pattern = Solid;
    Rgb c[3] = {{0.35f, 0.85f, 0.35f}, {0.1f, 0.5f, 0.15f}, {1.f, 1.f, 1.f}};
    int stripeLen = 4;
    Rgb colorAt(int i, int total) const;
    void setDefault() { *this = SkinSpec(); }
    bool load(const std::string& path);
    bool save(const std::string& path) const;
    static const char* patternName(int p);
};
struct SkinPreset { const char* name; int pattern; Rgb c[3]; };
extern const SkinPreset kSkinPresets[];
extern const int kSkinPresetCount;
}  // namespace me
