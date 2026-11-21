#pragma once
#include <string>
namespace me {
enum class Action : int {
    ToggleLine, ToggleAutoRun, ZoomIn, ZoomOut, ToggleMinimap, ToggleHud, ToggleFps,
    ClearDeathMarker, Boost, Count
};
constexpr int kActionCount = (int)Action::Count;
enum class Corner : int { TopLeft, TopRight, BottomLeft, BottomRight };
const char* actionName(Action a);
std::string keyName(int key);  // códigos compatíveis com GLFW (ASCII p/ letras, dígitos, espaço)
constexpr int kKeyEscape = 256;  // reservado: volta ao menu, não pode ser atalho

struct Settings {
    // Geral
    std::string nick = "Player";
    std::string server;  // "ip:porta"
    // Zoom
    bool smoothZoom = true, wheelZoom = true;
    float zoomMin = 0.4f, zoomMax = 2.5f, zoomStep = 0.1f, zoomSpeed = 8.f, zoomDefault = 1.f;
    // Linha de assistência (o estado ligado/desligado NÃO é salvo: sempre inicia desligada)
    bool lineFeature = true;
    float lineColor[4] = {1.f, 0.85f, 0.1f, 0.8f};
    float lineThickness = 2.f, lineLength = 400.f;
    // Auto Run
    bool autoRunFeature = true, autoRunInterruptOnMouse = true;
    // Minimapa
    bool minimapShow = true;
    float minimapSize = 160.f;
    Corner minimapCorner = Corner::BottomRight;
    // HUD
    bool hudShow = true, hudFps = true, hudPing = true, hudKills = true, hudTime = true, hudBest = true, hudScore = true;
    Corner hudCorner = Corner::TopLeft;
    // Comida (apenas visual local)
    float foodScale = 1.f;
    // Gráficos
    bool vsync = true;
    int fpsCap = 0, circleSegments = 12;
    bool showGrid = false;
    // Marcador de morte
    bool markerClearOnNewGame = false;
    // Diagnóstico: grava pacotes recebidos (hex) em capture.log. Desligado por padrão.
    bool captureLog = false;
    // Atalhos
    int keys[kActionCount];

    Settings() { resetKeys(); }
    void resetKeys();
    // Retorna o índice da ação que já usa a tecla (exceto 'except'), ou -1.
    int findKeyConflict(int key, int except) const;
    // Define atalho; recusa conflitos e ESC. Retorna true se aplicado; em conflito preenche *conflict.
    bool setKey(Action a, int key, Action* conflict = nullptr);
    void clamp();
    void load(const std::string& path);
    bool save(const std::string& path) const;
};
}  // namespace me
