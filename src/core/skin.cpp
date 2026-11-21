#include "skin.h"
#include <algorithm>
#include "ini.h"
namespace me {
const char* SkinSpec::patternName(int p) {
    static const char* n[] = {"Sólido", "Alternado", "Listras (3 cores)", "Gradiente"};
    return p >= 0 && p < PatternCount ? n[p] : "?";
}
const SkinPreset kSkinPresets[] = {
    {"Verde clássico", SkinSpec::Solid, {{0.35f, 0.85f, 0.35f}, {0.1f, 0.5f, 0.15f}, {1, 1, 1}}},
    {"Azul/Ciano", SkinSpec::Alternating, {{0.2f, 0.4f, 1.f}, {0.2f, 0.9f, 0.95f}, {1, 1, 1}}},
    {"Fogo", SkinSpec::Gradient, {{1.f, 0.85f, 0.1f}, {0.9f, 0.1f, 0.05f}, {1, 1, 1}}},
    {"Roxo neon", SkinSpec::Alternating, {{0.7f, 0.2f, 1.f}, {1.f, 0.2f, 0.8f}, {1, 1, 1}}},
    {"Listras tricolor", SkinSpec::Stripes, {{0.9f, 0.1f, 0.1f}, {1.f, 1.f, 1.f}, {0.1f, 0.2f, 0.9f}}},
    {"Mono escuro", SkinSpec::Gradient, {{0.9f, 0.9f, 0.9f}, {0.15f, 0.15f, 0.15f}, {1, 1, 1}}},
};
const int kSkinPresetCount = sizeof(kSkinPresets) / sizeof(kSkinPresets[0]);

static Rgb mix(Rgb a, Rgb b, float t) { return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t}; }
Rgb SkinSpec::colorAt(int i, int total) const {
    switch (pattern) {
        case Alternating: return (i & 1) ? c[1] : c[0];
        case Stripes: return c[(i / std::max(1, stripeLen)) % 3];
        case Gradient: return mix(c[0], c[1], total > 1 ? float(i) / float(total - 1) : 0.f);
        default: return c[0];
    }
}
bool SkinSpec::load(const std::string& path) {
    Ini i; if (!i.load(path)) return false;
    gameSkinId = std::max(0, std::min(255, (int)i.getInt("gameSkinId", 0)));
    pattern = std::max(0, std::min((int)PatternCount - 1, (int)i.getInt("pattern", 0)));
    stripeLen = std::max(1, std::min(32, (int)i.getInt("stripeLen", 4)));
    for (int k = 0; k < 3; ++k) {
        std::string p = "c" + std::to_string(k);
        c[k] = {(float)i.getFloat(p + "r", c[k].r), (float)i.getFloat(p + "g", c[k].g), (float)i.getFloat(p + "b", c[k].b)};
    }
    return true;
}
bool SkinSpec::save(const std::string& path) const {
    Ini i; i.setInt("gameSkinId", gameSkinId); i.setInt("pattern", pattern); i.setInt("stripeLen", stripeLen);
    for (int k = 0; k < 3; ++k) {
        std::string p = "c" + std::to_string(k);
        i.setFloat(p + "r", c[k].r); i.setFloat(p + "g", c[k].g); i.setFloat(p + "b", c[k].b);
    }
    return i.save(path);
}
}  // namespace me
