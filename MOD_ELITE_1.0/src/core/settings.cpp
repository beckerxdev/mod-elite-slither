#include "settings.h"
#include <algorithm>
#include "ini.h"
namespace me {
const char* actionName(Action a) {
    static const char* n[] = {"Linha de assistência", "Auto Run", "Zoom +", "Zoom -", "Minimapa", "HUD", "FPS",
                              "Limpar marcador de morte", "Boost"};
    return (int)a >= 0 && (int)a < kActionCount ? n[(int)a] : "?";
}
std::string keyName(int k) {
    if (k >= 'A' && k <= 'Z') return std::string(1, char(k));
    if (k >= '0' && k <= '9') return std::string(1, char(k));
    if (k == 32) return "Espaço";
    if (k == 61) return "=";
    if (k == 45) return "-";
    if (k == 44) return ",";
    if (k == 46) return ".";
    if (k == 59) return ";";
    if (k == 47) return "/";
    if (k == 258) return "Tab";
    if (k == 257) return "Enter";
    if (k >= 290 && k <= 301) return "F" + std::to_string(k - 289);
    if (k >= 320 && k <= 329) return "Num" + std::to_string(k - 320);
    if (k == 332) return "Num*";
    if (k == 334) return "Num+";
    if (k == 333) return "Num-";
    if (k == 340 || k == 344) return "Shift";
    if (k == 341 || k == 345) return "Ctrl";
    if (k == 342 || k == 346) return "Alt";
    if (k == -1) return "(nenhuma)";
    return "KEY" + std::to_string(k);
}
void Settings::resetKeys() {
    keys[(int)Action::ToggleLine] = 'L';
    keys[(int)Action::ToggleAutoRun] = 'R';
    keys[(int)Action::ZoomIn] = 61;   // '='
    keys[(int)Action::ZoomOut] = 45;  // '-'
    keys[(int)Action::ToggleMinimap] = 'M';
    keys[(int)Action::ToggleHud] = 'H';
    keys[(int)Action::ToggleFps] = 'F';
    keys[(int)Action::ClearDeathMarker] = 'X';
    keys[(int)Action::Boost] = 32;
}
int Settings::findKeyConflict(int key, int except) const {
    for (int i = 0; i < kActionCount; ++i)
        if (i != except && keys[i] == key) return i;
    return -1;
}
bool Settings::setKey(Action a, int key, Action* conflict) {
    if (key == kKeyEscape || key < 0) return false;
    int c = findKeyConflict(key, (int)a);
    if (c >= 0) { if (conflict) *conflict = (Action)c; return false; }
    keys[(int)a] = key;
    return true;
}
static float cl(float v, float lo, float hi) { return std::min(hi, std::max(lo, v)); }
void Settings::clamp() {
    zoomMin = cl(zoomMin, 0.1f, 1.f); zoomMax = cl(zoomMax, 1.f, 5.f);
    zoomStep = cl(zoomStep, 0.01f, 0.5f); zoomSpeed = cl(zoomSpeed, 1.f, 30.f);
    zoomDefault = cl(zoomDefault, zoomMin, zoomMax);
    for (float& c : lineColor) c = cl(c, 0.f, 1.f);
    lineThickness = cl(lineThickness, 1.f, 12.f); lineLength = cl(lineLength, 50.f, 2000.f);
    minimapSize = cl(minimapSize, 80.f, 400.f);
    foodScale = cl(foodScale, 0.3f, 3.f);
    fpsCap = std::max(0, std::min(1000, fpsCap));
    circleSegments = std::max(6, std::min(32, circleSegments));
    if (nick.size() > 24) nick.resize(24);
}
void Settings::load(const std::string& path) {
    Ini i; if (!i.load(path)) return;
    nick = i.getStr("nick", nick); server = i.getStr("server", server);
    smoothZoom = i.getBool("smoothZoom", smoothZoom); wheelZoom = i.getBool("wheelZoom", wheelZoom);
    zoomMin = (float)i.getFloat("zoomMin", zoomMin); zoomMax = (float)i.getFloat("zoomMax", zoomMax);
    zoomStep = (float)i.getFloat("zoomStep", zoomStep); zoomSpeed = (float)i.getFloat("zoomSpeed", zoomSpeed);
    zoomDefault = (float)i.getFloat("zoomDefault", zoomDefault);
    lineFeature = i.getBool("lineFeature", lineFeature);
    for (int c = 0; c < 4; ++c) lineColor[c] = (float)i.getFloat("lineColor" + std::to_string(c), lineColor[c]);
    lineThickness = (float)i.getFloat("lineThickness", lineThickness);
    lineLength = (float)i.getFloat("lineLength", lineLength);
    autoRunFeature = i.getBool("autoRunFeature", autoRunFeature);
    autoRunInterruptOnMouse = i.getBool("autoRunInterruptOnMouse", autoRunInterruptOnMouse);
    minimapShow = i.getBool("minimapShow", minimapShow); minimapSize = (float)i.getFloat("minimapSize", minimapSize);
    minimapCorner = (Corner)std::max(0, std::min(3, (int)i.getInt("minimapCorner", (int)minimapCorner)));
    hudShow = i.getBool("hudShow", hudShow); hudFps = i.getBool("hudFps", hudFps); hudPing = i.getBool("hudPing", hudPing);
    hudKills = i.getBool("hudKills", hudKills); hudTime = i.getBool("hudTime", hudTime);
    hudBest = i.getBool("hudBest", hudBest); hudScore = i.getBool("hudScore", hudScore);
    hudCorner = (Corner)std::max(0, std::min(3, (int)i.getInt("hudCorner", (int)hudCorner)));
    foodScale = (float)i.getFloat("foodScale", foodScale);
    vsync = i.getBool("vsync", vsync); fpsCap = (int)i.getInt("fpsCap", fpsCap);
    circleSegments = (int)i.getInt("circleSegments", circleSegments); showGrid = i.getBool("showGrid", showGrid);
    markerClearOnNewGame = i.getBool("markerClearOnNewGame", markerClearOnNewGame);
    captureLog = i.getBool("captureLog", captureLog);
    // Atalhos: carrega e valida (se o arquivo tiver conflito/ESC, volta ao padrão de tudo).
    Settings def; int loaded[kActionCount]; bool ok = true;
    for (int a = 0; a < kActionCount; ++a) {
        loaded[a] = (int)i.getInt("key" + std::to_string(a), def.keys[a]);
        if (loaded[a] < 0 || loaded[a] == kKeyEscape) ok = false;
        for (int b = 0; b < a; ++b) if (loaded[a] == loaded[b]) ok = false;
    }
    if (ok) for (int a = 0; a < kActionCount; ++a) keys[a] = loaded[a];
    clamp();
}
bool Settings::save(const std::string& path) const {
    Ini i;
    i.setStr("nick", nick); i.setStr("server", server);
    i.setBool("smoothZoom", smoothZoom); i.setBool("wheelZoom", wheelZoom);
    i.setFloat("zoomMin", zoomMin); i.setFloat("zoomMax", zoomMax); i.setFloat("zoomStep", zoomStep);
    i.setFloat("zoomSpeed", zoomSpeed); i.setFloat("zoomDefault", zoomDefault);
    i.setBool("lineFeature", lineFeature);
    for (int c = 0; c < 4; ++c) i.setFloat("lineColor" + std::to_string(c), lineColor[c]);
    i.setFloat("lineThickness", lineThickness); i.setFloat("lineLength", lineLength);
    i.setBool("autoRunFeature", autoRunFeature); i.setBool("autoRunInterruptOnMouse", autoRunInterruptOnMouse);
    i.setBool("minimapShow", minimapShow); i.setFloat("minimapSize", minimapSize); i.setInt("minimapCorner", (int)minimapCorner);
    i.setBool("hudShow", hudShow); i.setBool("hudFps", hudFps); i.setBool("hudPing", hudPing);
    i.setBool("hudKills", hudKills); i.setBool("hudTime", hudTime); i.setBool("hudBest", hudBest);
    i.setBool("hudScore", hudScore); i.setInt("hudCorner", (int)hudCorner);
    i.setFloat("foodScale", foodScale);
    i.setBool("vsync", vsync); i.setInt("fpsCap", fpsCap); i.setInt("circleSegments", circleSegments);
    i.setBool("showGrid", showGrid); i.setBool("markerClearOnNewGame", markerClearOnNewGame); i.setBool("captureLog", captureLog);
    for (int a = 0; a < kActionCount; ++a) i.setInt("key" + std::to_string(a), keys[a]);
    return i.save(path);
}
}  // namespace me
