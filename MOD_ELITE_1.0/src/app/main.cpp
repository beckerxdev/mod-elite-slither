// MOD ELITE 1.0 — cliente nativo para Slither.io
// GLFW (janela/entrada) + Dear ImGui (interface e desenho 2D em lote via ImDrawList) + OpenGL 3.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "../core/assist.h"
#include "../core/autorun.h"
#include "../core/fps.h"
#include "../core/minimap.h"
#include "../core/paths.h"
#include "../core/protocol.h"
#include "../core/server.h"
#include "../core/session.h"
#include "../core/settings.h"
#include "../core/skin.h"
#include "../core/stats.h"
#include "../net/ws_client.h"

using namespace me;

enum class Screen { Menu, SkinEditor, SettingsScr, Game };

struct App {
    Settings cfg; Stats stats; SkinSpec skin; World world;
    Session session{stats, cfg};
    GameClient client{session, world};
    AssistLine line; AutoRun autorun; Minimap minimap; FpsCounter fps;
    WsClient ws;
    std::string dir, cfgPath, statsPath, skinPath, capturePath;

    Screen screen = Screen::Menu;
    char nickBuf[32] = {0}, serverBuf[64] = {0};
    std::string status; bool statusError = false;
    int rebinding = -1; std::string rebindMsg;
    bool dirtyCfg = false, dirtySkin = false;

    float zoom = 1.f, zoomTarget = 1.f;
    double mouseX = 0, mouseY = 0, mouseAtAutoX = 0, mouseAtAutoY = 0;
    int winW = 1000, winH = 650;

    // Rede
    std::mutex qmu; std::vector<std::vector<uint8_t>> inbox;
    std::atomic<bool> netClosed{false}; std::string netCloseReason;
    std::thread connectThread; std::atomic<int> connectState{0};  // 0 ocioso, 1 conectando, 2 ok, 3 falhou
    std::string connectErr;
    bool playing = false;       // true só quando o login do protocolo estiver concluído (ainda não implementado)
    unsigned long captured = 0; double lastPing = 0, lastSend = 0; uint8_t lastAngle = 255;
    bool boostHeld = false;
};
static App* A = nullptr;

static void saveCfg() { A->cfg.save(A->cfgPath); }
static void saveStats() { A->stats.save(A->statsPath); }
static void saveSkin() { A->skin.save(A->skinPath); }

static ImU32 col(float r, float g, float b, float a = 1.f) { return IM_COL32(int(r * 255), int(g * 255), int(b * 255), int(a * 255)); }
static ImU32 hashColor(unsigned h) {
    float r = 0.35f + 0.65f * ((h * 2654435761u >> 8 & 255) / 255.f), g = 0.35f + 0.65f * ((h * 40503u >> 4 & 255) / 255.f),
          b = 0.35f + 0.65f * ((h * 2246822519u >> 12 & 255) / 255.f);
    return col(r, g, b);
}

// ---------------- Rede ----------------
static void stopNet() {
    A->ws.close();
    if (A->connectThread.joinable()) A->connectThread.join();
    A->playing = false; A->connectState = 0;
}

static void startConnect() {
    std::string host, err; int port = 0;
    A->cfg.nick = A->nickBuf; A->cfg.server = A->serverBuf; A->cfg.clamp(); saveCfg();  // salva nome e último servidor
    if (!parseServer(A->cfg.server, host, port, err)) { A->status = err; A->statusError = true; return; }
    stopNet();
    A->status = "Conectando a " + host + ":" + std::to_string(port) + " ..."; A->statusError = false;
    A->connectState = 1; A->netClosed = false; A->captured = 0;
    bool cap = A->cfg.captureLog; if (cap) std::remove(A->capturePath.c_str());
    A->connectThread = std::thread([host, port, cap] {
        std::string e;
        if (!A->ws.connect(host, port, "/slither", "https://slither.com", 5000, e)) { A->connectErr = e; A->connectState = 3; return; }
        A->ws.start(
            [cap](const uint8_t* d, size_t n, bool) {
                std::lock_guard<std::mutex> l(A->qmu); A->inbox.emplace_back(d, d + n);
                if (cap && A->captured < 300) {  // diagnóstico: primeiros 300 pacotes em hex
                    FILE* f = std::fopen(A->capturePath.c_str(), "a");
                    if (f) { std::fprintf(f, "#%lu len=%zu ", A->captured, n); for (size_t i = 0; i < n && i < 256; ++i) std::fprintf(f, "%02x", d[i]); std::fputc('\n', f); std::fclose(f); }
                }
                ++A->captured;
            },
            [](const std::string& r) { A->netCloseReason = r; A->netClosed = true; });
        A->connectState = 2;
    });
}

static void processNet(double now) {
    if (A->connectState == 2 && A->screen == Screen::Menu) {
        A->screen = Screen::Game; A->status.clear();
        A->world = World(); A->zoom = A->zoomTarget = A->cfg.zoomDefault;
    }
    if (A->connectState == 3) { A->status = "Falha: " + A->connectErr; A->statusError = true; A->connectState = 0; }
    if (A->netClosed.exchange(false)) {
        A->status = "Desconectado: " + A->netCloseReason; A->statusError = true;
        if (A->session.alive()) { A->session.onDeath(now); saveStats(); }
        A->playing = false;
    }
    std::vector<std::vector<uint8_t>> local;
    { std::lock_guard<std::mutex> l(A->qmu); local.swap(A->inbox); }
    for (auto& p : local) {
        char t = A->client.onPacket(p.data(), p.size(), now);
        if (t == 'v') saveStats();
    }
    if (A->ws.isOpen() && A->playing && now - A->lastPing > 2.0) {  // ping periódico (só após login concluído)
        uint8_t b = proto::kPing; A->ws.sendBinary(&b, 1); A->client.notePingSent(now); A->lastPing = now;
    }
}

// ---------------- Entrada ----------------
static void doAction(Action a, bool press) {
    Settings& c = A->cfg;
    switch (a) {
        case Action::ToggleLine: if (press && c.lineFeature) A->line.toggle(); break;
        case Action::ToggleAutoRun:
            if (press && c.autoRunFeature) { A->autorun.toggle(); A->mouseAtAutoX = A->mouseX; A->mouseAtAutoY = A->mouseY; }
            break;
        case Action::ZoomIn: if (press) A->zoomTarget = std::min(c.zoomMax, A->zoomTarget + c.zoomStep); break;
        case Action::ZoomOut: if (press) A->zoomTarget = std::max(c.zoomMin, A->zoomTarget - c.zoomStep); break;
        case Action::ToggleMinimap: if (press) { c.minimapShow = !c.minimapShow; saveCfg(); } break;
        case Action::ToggleHud: if (press) { c.hudShow = !c.hudShow; saveCfg(); } break;
        case Action::ToggleFps: if (press) { c.hudFps = !c.hudFps; saveCfg(); } break;
        case Action::ClearDeathMarker: if (press) { A->session.clearMarker(); saveStats(); } break;
        case Action::Boost:
            if (A->playing && A->ws.isOpen() && A->boostHeld != press) {
                A->boostHeld = press; uint8_t b = press ? proto::kBoostOn : proto::kBoostOff; A->ws.sendBinary(&b, 1);
            }
            break;
        default: break;
    }
}
static void keyCb(GLFWwindow*, int key, int, int action, int) {
    if (action == GLFW_REPEAT) return;
    if (A->rebinding >= 0 && action == GLFW_PRESS) {  // capturando novo atalho
        if (key == GLFW_KEY_ESCAPE) { A->rebinding = -1; A->rebindMsg = "Cancelado."; return; }
        Action conflict = Action::Count;
        if (A->cfg.setKey((Action)A->rebinding, key, &conflict)) { A->rebindMsg = ""; saveCfg(); }
        else A->rebindMsg = std::string("Tecla ") + keyName(key) + " já usada por: " + actionName(conflict);
        A->rebinding = -1; return;
    }
    if (ImGui::GetIO().WantTextInput) return;
    bool press = action == GLFW_PRESS;
    if (A->screen == Screen::Game) {
        if (press && key == GLFW_KEY_ESCAPE) { stopNet(); A->screen = Screen::Menu; A->status = "Desconectado."; A->statusError = false; return; }
        for (int i = 0; i < kActionCount; ++i) if (A->cfg.keys[i] == key) doAction((Action)i, press);
    } else if (press && key == GLFW_KEY_ESCAPE && A->screen != Screen::Menu) {
        A->screen = Screen::Menu;
    }
}
static void scrollCb(GLFWwindow*, double, double dy) {
    if (A->screen != Screen::Game || !A->cfg.wheelZoom || ImGui::GetIO().WantCaptureMouse) return;
    A->zoomTarget = std::max(A->cfg.zoomMin, std::min(A->cfg.zoomMax, A->zoomTarget + (float)dy * A->cfg.zoomStep));
}
static void cursorCb(GLFWwindow*, double x, double y) {
    A->mouseX = x; A->mouseY = y;
    if (A->autorun.active() && A->cfg.autoRunInterruptOnMouse) {  // jogador assume o controle ao mover o mouse
        double dx = x - A->mouseAtAutoX, dy = y - A->mouseAtAutoY;
        if (dx * dx + dy * dy > 40.0 * 40.0) A->autorun.interrupt();
    }
}

// ---------------- Estilo ----------------
static void setupStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    s.WindowRounding = 8; s.FrameRounding = 5; s.GrabRounding = 4; s.TabRounding = 4;
    s.FramePadding = ImVec2(10, 7); s.ItemSpacing = ImVec2(10, 9); s.WindowPadding = ImVec2(18, 16);
    s.FontScaleMain = 1.25f;
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.075f, 0.085f, 0.10f, 1.f);
    c[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.135f, 0.16f, 1.f);
    c[ImGuiCol_Button] = ImVec4(0.14f, 0.17f, 0.21f, 1.f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.24f, 0.30f, 1.f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.10f, 0.45f, 0.40f, 1.f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.16f, 0.68f, 0.58f, 1.f);
    c[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.80f, 0.68f, 1.f);
    c[ImGuiCol_Header] = ImVec4(0.14f, 0.30f, 0.30f, 1.f);
    c[ImGuiCol_Tab] = ImVec4(0.10f, 0.12f, 0.15f, 1.f);
    c[ImGuiCol_TabSelected] = ImVec4(0.12f, 0.45f, 0.40f, 1.f);
}
static const ImVec4 kAccent(0.20f, 0.80f, 0.68f, 1.f), kErr(1.f, 0.4f, 0.35f, 1.f);

// ---------------- Telas ----------------
static void drawMenu() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::GetForegroundDrawList()->AddText(ImVec2(io.DisplaySize.x - 170, 12), col(0.2f, 0.8f, 0.68f), "MOD ELITE 1.0");
    const float W = 440;
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(W, 0));
    ImGui::Begin("##menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    const Stats& st = A->stats;
    ImGui::TextColored(kAccent, "RECORDE"); ImGui::SameLine(); ImGui::Text("%d", st.bestScore);
    ImGui::SameLine(0, 22); ImGui::TextColored(kAccent, "KILLS"); ImGui::SameLine(); ImGui::Text("%d", st.lastKills);
    ImGui::SameLine(0, 22); ImGui::TextColored(kAccent, "TEMPO"); ImGui::SameLine(); ImGui::Text("%s", formatTime(st.lastTimeSec).c_str());
    ImGui::Separator();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##nickname_input", "Nome do jogador", A->nickBuf, sizeof A->nickBuf);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##ipv4_input", "IP:porta do servidor (ex.: 1.2.3.4:444)", A->serverBuf, sizeof A->serverBuf);
    if (ImGui::Button("PLAY", ImVec2(-1, 46))) startConnect();
    float half = (W - 36 - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
    if (ImGui::Button("SKIN EDITOR", ImVec2(half, 38))) A->screen = Screen::SkinEditor;
    ImGui::SameLine();
    if (ImGui::Button("SETTINGS", ImVec2(half, 38))) A->screen = Screen::SettingsScr;
    if (ImGui::Button("QUIT", ImVec2(-1, 38))) glfwSetWindowShouldClose(glfwGetCurrentContext(), 1);
    if (!A->status.empty()) ImGui::TextColored(A->statusError ? kErr : kAccent, "%s", A->status.c_str());
    ImGui::End();
}

static void drawSkinEditor() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(560, 0));
    ImGui::Begin("##skin", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextColored(kAccent, "SKIN EDITOR");
    SkinSpec& s = A->skin; bool ch = false;
    // Prévia
    ImVec2 p = ImGui::GetCursorScreenPos(); const float w = 520, h = 110;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), col(0.05f, 0.06f, 0.08f), 6);
    const int N = 34;
    for (int i = N - 1; i >= 0; --i) {
        float t = float(i) / (N - 1);
        Rgb c = s.colorAt(i, N);
        dl->AddCircleFilled(ImVec2(p.x + 40 + t * (w - 100), p.y + h * 0.5f + std::sin(t * 6.f) * 26.f), 15.f, col(c.r, c.g, c.b), 20);
    }
    ImGui::Dummy(ImVec2(w, h));
    // Presets
    for (int i = 0; i < kSkinPresetCount; ++i) {
        if (i) ImGui::SameLine();
        if (ImGui::Button((std::string(kSkinPresets[i].name) + "##pre" + std::to_string(i)).c_str())) {
            s.pattern = kSkinPresets[i].pattern; for (int k = 0; k < 3; ++k) s.c[k] = kSkinPresets[i].c[k]; ch = true;
        }
        if (i == 2) ImGui::NewLine();
    }
    static const char* pats[] = {"Sólido", "Alternado", "Listras (3 cores)", "Gradiente"};
    ch |= ImGui::Combo("Padrão", &s.pattern, pats, 4);
    ch |= ImGui::ColorEdit3("Cor 1", &s.c[0].r);
    if (s.pattern != SkinSpec::Solid) ch |= ImGui::ColorEdit3("Cor 2", &s.c[1].r);
    if (s.pattern == SkinSpec::Stripes) { ch |= ImGui::ColorEdit3("Cor 3", &s.c[2].r); ch |= ImGui::SliderInt("Tamanho da listra", &s.stripeLen, 1, 12); }
    ch |= ImGui::SliderInt("ID de skin do jogo (visto por outros)", &s.gameSkinId, 0, 255);
    ImGui::TextDisabled("As cores/padrões acima são locais (só você vê). O ID é o que o servidor aceita.");
    if (ImGui::Button("Skin padrão")) { s.setDefault(); ch = true; }
    ImGui::SameLine(); if (ImGui::Button("Voltar")) A->screen = Screen::Menu;
    if (ch) A->dirtySkin = true;
    if (A->dirtySkin && !ImGui::IsAnyItemActive()) { saveSkin(); A->dirtySkin = false; }  // salva automaticamente
    ImGui::End();
}

static void keyRow(Action a) {
    int i = (int)a;
    ImGui::Text("%s", actionName(a)); ImGui::SameLine(250);
    std::string lbl = (A->rebinding == i ? std::string("Pressione uma tecla...") : keyName(A->cfg.keys[i])) + "##key" + std::to_string(i);
    if (ImGui::Button(lbl.c_str(), ImVec2(190, 0))) { A->rebinding = i; A->rebindMsg.clear(); }
}

static void drawSettings() {
    ImGuiIO& io = ImGui::GetIO(); Settings& c = A->cfg; bool ch = false;
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(640, 0));
    ImGui::Begin("##settings", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextColored(kAccent, "SETTINGS");
    static const char* corners[] = {"Superior esquerdo", "Superior direito", "Inferior esquerdo", "Inferior direito"};
    if (ImGui::BeginTabBar("##tabs")) {
        if (ImGui::BeginTabItem("Zoom")) {
            ch |= ImGui::Checkbox("Zoom suave", &c.smoothZoom);
            ch |= ImGui::Checkbox("Roda do mouse controla o zoom", &c.wheelZoom);
            ch |= ImGui::SliderFloat("Zoom mínimo", &c.zoomMin, 0.1f, 1.f, "%.2f");
            ch |= ImGui::SliderFloat("Zoom máximo", &c.zoomMax, 1.f, 5.f, "%.2f");
            ch |= ImGui::SliderFloat("Passo do zoom", &c.zoomStep, 0.01f, 0.5f, "%.2f");
            ch |= ImGui::SliderFloat("Velocidade da suavização", &c.zoomSpeed, 1.f, 30.f, "%.1f");
            ch |= ImGui::SliderFloat("Zoom inicial", &c.zoomDefault, c.zoomMin, c.zoomMax, "%.2f");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Linha")) {
            ch |= ImGui::Checkbox("Recurso habilitado (alternância por tecla durante a partida)", &c.lineFeature);
            ImGui::TextDisabled("A linha sempre começa DESLIGADA e só liga/desliga com o atalho.");
            ch |= ImGui::ColorEdit4("Cor", c.lineColor);
            ch |= ImGui::SliderFloat("Espessura", &c.lineThickness, 1.f, 12.f, "%.1f");
            ch |= ImGui::SliderFloat("Comprimento", &c.lineLength, 50.f, 2000.f, "%.0f");
            keyRow(Action::ToggleLine);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Auto Run")) {
            ch |= ImGui::Checkbox("Recurso habilitado", &c.autoRunFeature);
            ch |= ImGui::Checkbox("Mover o mouse devolve o controle ao jogador", &c.autoRunInterruptOnMouse);
            keyRow(Action::ToggleAutoRun);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Minimapa")) {
            ch |= ImGui::Checkbox("Mostrar minimapa", &c.minimapShow);
            ch |= ImGui::SliderFloat("Tamanho", &c.minimapSize, 80.f, 400.f, "%.0f px");
            int cn = (int)c.minimapCorner; if (ImGui::Combo("Posição", &cn, corners, 4)) { c.minimapCorner = (Corner)cn; ch = true; }
            ch |= ImGui::Checkbox("Apagar marcador de morte ao iniciar nova partida", &c.markerClearOnNewGame);
            keyRow(Action::ToggleMinimap); keyRow(Action::ClearDeathMarker);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("HUD")) {
            ch |= ImGui::Checkbox("HUD visível", &c.hudShow);
            ch |= ImGui::Checkbox("FPS", &c.hudFps); ch |= ImGui::Checkbox("Ping", &c.hudPing);
            ch |= ImGui::Checkbox("Kills da partida", &c.hudKills); ch |= ImGui::Checkbox("Tempo de sobrevivência", &c.hudTime);
            ch |= ImGui::Checkbox("Recorde pessoal", &c.hudBest); ch |= ImGui::Checkbox("Pontuação/tamanho", &c.hudScore);
            int cn = (int)c.hudCorner; if (ImGui::Combo("Posição do HUD", &cn, corners, 4)) { c.hudCorner = (Corner)cn; ch = true; }
            keyRow(Action::ToggleHud); keyRow(Action::ToggleFps);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Comida")) {
            ch |= ImGui::SliderFloat("Tamanho visual das massinhas", &c.foodScale, 0.3f, 3.f, "%.2fx");
            if (ImGui::Button("Restaurar padrão")) { c.foodScale = 1.f; ch = true; }
            ImGui::TextDisabled("Altera só o desenho local; a massa real vinda do servidor não muda.");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Gráficos")) {
            ch |= ImGui::Checkbox("VSync", &c.vsync);
            ch |= ImGui::SliderInt("Limite de FPS (0 = sem limite; com VSync desligado)", &c.fpsCap, 0, 360);
            ch |= ImGui::SliderInt("Suavidade dos círculos", &c.circleSegments, 6, 32);
            ch |= ImGui::Checkbox("Grade de fundo", &c.showGrid);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Atalhos")) {
            for (int i = 0; i < kActionCount; ++i) keyRow((Action)i);
            if (!A->rebindMsg.empty()) ImGui::TextColored(kErr, "%s", A->rebindMsg.c_str());
            ImGui::TextDisabled("ESC é reservado (volta ao menu). Atalhos duplicados são recusados.");
            if (ImGui::Button("Restaurar atalhos padrão")) { c.resetKeys(); ch = true; }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Dados")) {
            ch |= ImGui::Checkbox("Diagnóstico: gravar pacotes recebidos em capture.log", &c.captureLog);
            ImGui::TextDisabled("Local ao seu PC; usado para validar o protocolo. Desligado por padrão.");
            ImGui::Text("Pasta de dados: %s", A->dir.c_str());
            if (ImGui::Button("Zerar estatísticas")) { A->stats.resetStats(); saveStats(); }
            ImGui::SameLine(); if (ImGui::Button("Remover marcador de morte")) { A->stats.marker.valid = false; saveStats(); }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    if (ImGui::Button("Voltar")) A->screen = Screen::Menu;
    if (ch) { c.clamp(); A->dirtyCfg = true; }
    if (A->dirtyCfg && !ImGui::IsAnyItemActive()) { saveCfg(); A->dirtyCfg = false; }
    ImGui::End();
}

// ---------------- Jogo ----------------
static ImVec2 cornerPos(Corner c, ImVec2 size, float margin, ImVec2 disp) {
    float x = (c == Corner::TopLeft || c == Corner::BottomLeft) ? margin : disp.x - size.x - margin;
    float y = (c == Corner::TopLeft || c == Corner::TopRight) ? margin : disp.y - size.y - margin;
    return ImVec2(x, y);
}

static void drawWorld(ImDrawList* dl, ImVec2 disp, double now) {
    const Settings& c = A->cfg; const Snake* me = A->world.me();
    if (!me) {
        const char* m = "Aguardando dados do servidor (protocolo parcial: login/mundo ainda nao implementados)";
        ImVec2 ts = ImGui::CalcTextSize(m);
        dl->AddText(ImVec2((disp.x - ts.x) * 0.5f, disp.y * 0.5f), col(0.8f, 0.8f, 0.8f), m);
        return;
    }
    const float z = A->zoom, cx = disp.x * 0.5f, cy = disp.y * 0.5f;
    auto sx = [&](float x) { return cx + (x - me->head.x) * z; };
    auto sy = [&](float y) { return cy + (y - me->head.y) * z; };
    dl->AddCircle(ImVec2(sx(0), sy(0)), A->world.gameRadius * z, col(0.9f, 0.2f, 0.2f, 0.8f), 160, 3.f);
    if (c.showGrid) {
        const float g = 300.f * z; if (g > 12.f) {
            float ox = std::fmod(cx - me->head.x * z, g), oy = std::fmod(cy - me->head.y * z, g);
            for (float x = ox; x < disp.x; x += g) dl->AddLine(ImVec2(x, 0), ImVec2(x, disp.y), col(1, 1, 1, 0.04f));
            for (float y = oy; y < disp.y; y += g) dl->AddLine(ImVec2(0, y), ImVec2(disp.x, y), col(1, 1, 1, 0.04f));
        }
    }
    const int seg = c.circleSegments;
    for (const Food& f : A->world.food) {  // massinhas: apenas escala visual local
        float x = sx(f.x), y = sy(f.y), r = 3.2f * f.size * c.foodScale * z;
        if (x < -r || y < -r || x > disp.x + r || y > disp.y + r) continue;
        dl->AddCircleFilled(ImVec2(x, y), r, hashColor(f.color + 17u), seg);
    }
    for (const auto& kv : A->world.snakes) {
        const Snake& s = kv.second; const bool mine = s.id == A->world.meId;
        const int n = (int)s.body.size() + 1; ImU32 base = hashColor((unsigned)s.id);
        for (int i = n - 1; i >= 0; --i) {
            Vec2 p = i == 0 ? s.head : s.body[i - 1];
            float x = sx(p.x), y = sy(p.y), r = s.radius * z;
            if (x < -r || y < -r || x > disp.x + r || y > disp.y + r) continue;
            ImU32 cc = base; if (mine) { Rgb k = A->skin.colorAt(i, n); cc = col(k.r, k.g, k.b); }
            dl->AddCircleFilled(ImVec2(x, y), r, cc, seg);
        }
    }
    Vec2 a, b;  // linha de assistência: nenhum cálculo/desenho quando desligada
    if (A->line.compute(me->head, me->angle, c.lineLength, a, b))
        dl->AddLine(ImVec2(sx(a.x), sy(a.y)), ImVec2(sx(b.x), sy(b.y)), col(c.lineColor[0], c.lineColor[1], c.lineColor[2], c.lineColor[3]), c.lineThickness);
    (void)now;
}

static void drawMinimap(ImDrawList* dl, ImVec2 disp, double now) {
    const Settings& c = A->cfg;
    A->minimap.update(A->world, now, c.minimapShow);
    if (!c.minimapShow) return;
    const float S = c.minimapSize; ImVec2 o = cornerPos(c.minimapCorner, ImVec2(S, S), 14.f, disp);
    ImVec2 ctr(o.x + S * 0.5f, o.y + S * 0.5f);
    dl->AddCircleFilled(ctr, S * 0.5f, col(0.03f, 0.04f, 0.06f, 0.75f), 48);
    dl->AddCircle(ctr, S * 0.5f, col(0.2f, 0.8f, 0.68f, 0.9f), 48, 2.f);
    auto to = [&](MapDot d) { return ImVec2(o.x + d.x * S, o.y + d.y * S); };
    for (const MapDot& d : A->minimap.others()) dl->AddCircleFilled(to(d), 2.f, col(0.9f, 0.5f, 0.2f), 6);
    const DeathMarker& m = A->stats.marker;
    if (m.valid) {  // marcador da última morte
        MapDot d = Minimap::norm({m.x, m.y}, A->world.gameRadius); ImVec2 p = to(d);
        dl->AddLine(ImVec2(p.x - 5, p.y - 5), ImVec2(p.x + 5, p.y + 5), col(1, 0.2f, 0.2f), 2.f);
        dl->AddLine(ImVec2(p.x - 5, p.y + 5), ImVec2(p.x + 5, p.y - 5), col(1, 0.2f, 0.2f), 2.f);
        if (A->minimap.hasMe()) {
            const Snake* me = A->world.me();
            char b[48]; std::snprintf(b, sizeof b, "morte: %.0f", Minimap::distanceToMarker(me->head, m));
            dl->AddText(ImVec2(o.x + 4, o.y + S + 2), col(1, 0.5f, 0.5f), b);
        }
    }
    if (A->minimap.hasMe()) {
        ImVec2 p = to(A->minimap.me()); float an = A->minimap.meAngle();
        ImVec2 t(p.x + std::cos(an) * 7, p.y + std::sin(an) * 7), l(p.x + std::cos(an + 2.5f) * 4, p.y + std::sin(an + 2.5f) * 4),
            r(p.x + std::cos(an - 2.5f) * 4, p.y + std::sin(an - 2.5f) * 4);
        dl->AddTriangleFilled(t, l, r, col(1, 1, 1));
    }
}

static void drawHud(ImDrawList* dl, ImVec2 disp, double now) {
    const Settings& c = A->cfg;
    // Estado das funções (sempre visível: LIGADA/DESLIGADA)
    char st[128]; std::snprintf(st, sizeof st, "LINHA [%s]: %s    AUTO RUN [%s]: %s", keyName(c.keys[(int)Action::ToggleLine]).c_str(),
                                A->line.active() ? "LIGADA" : "DESLIGADA", keyName(c.keys[(int)Action::ToggleAutoRun]).c_str(), A->autorun.active() ? "ON" : "OFF");
    ImVec2 ts = ImGui::CalcTextSize(st);
    dl->AddText(ImVec2((disp.x - ts.x) * 0.5f, disp.y - ts.y - 10), col(0.85f, 0.9f, 0.9f), st);
    if (A->statusError && !A->status.empty()) dl->AddText(ImVec2(12, disp.y - 36), col(1, 0.45f, 0.4f), A->status.c_str());
    if (!c.hudShow) return;
    char lines[8][64]; int n = 0;
    if (c.hudFps) std::snprintf(lines[n++], 64, "%d FPS", A->fps.fps());
    if (c.hudPing) { if (A->client.pingMs() >= 0) std::snprintf(lines[n++], 64, "Ping: %d ms", A->client.pingMs()); else std::snprintf(lines[n++], 64, "Ping: --"); }
    if (c.hudKills) std::snprintf(lines[n++], 64, "Kills: %d", A->session.kills());
    if (c.hudTime) std::snprintf(lines[n++], 64, "Tempo: %s", formatTime(A->session.elapsed(now)).c_str());
    if (c.hudBest) std::snprintf(lines[n++], 64, "Recorde: %d", std::max(A->stats.bestScore, A->session.score()));
    if (c.hudScore) std::snprintf(lines[n++], 64, "Pontos: %d", A->session.score());
    float lh = ImGui::GetTextLineHeightWithSpacing(), wmax = 0;
    for (int i = 0; i < n; ++i) wmax = std::max(wmax, ImGui::CalcTextSize(lines[i]).x);
    ImVec2 o = cornerPos(c.hudCorner, ImVec2(wmax + 16, lh * n + 8), 12.f, disp);
    if (n) dl->AddRectFilled(o, ImVec2(o.x + wmax + 16, o.y + lh * n + 8), col(0, 0, 0, 0.35f), 6);
    for (int i = 0; i < n; ++i) dl->AddText(ImVec2(o.x + 8, o.y + 4 + i * lh), col(1, 1, 1), lines[i]);
    if (!A->playing) {
        char b[160]; std::snprintf(b, sizeof b, "Conectado (WebSocket). Login do protocolo NAO implementado - pacotes recebidos: %lu (desconhecidos: %lu)",
                                   A->client.totalPackets(), A->client.unknownPackets());
        dl->AddText(ImVec2(12, disp.y - 60), col(1, 0.8f, 0.3f), b);
    }
}

static void drawGame(double now) {
    ImGuiIO& io = ImGui::GetIO(); ImDrawList* dl = ImGui::GetBackgroundDrawList();
    drawWorld(dl, io.DisplaySize, now);
    drawMinimap(dl, io.DisplaySize, now);
    drawHud(ImGui::GetForegroundDrawList(), io.DisplaySize, now);
}

// ---------------- main ----------------
int main() {
    App app; A = &app;
    app.dir = dataDir();
    app.cfgPath = app.dir + "/settings.ini"; app.statsPath = app.dir + "/stats.ini"; app.skinPath = app.dir + "/skin.ini";
    app.capturePath = app.dir + "/capture.log";
    app.cfg.load(app.cfgPath); app.stats.load(app.statsPath); app.skin.load(app.skinPath);
    std::snprintf(app.nickBuf, sizeof app.nickBuf, "%s", app.cfg.nick.c_str());
    std::snprintf(app.serverBuf, sizeof app.serverBuf, "%s", app.cfg.server.c_str());
    app.zoom = app.zoomTarget = app.cfg.zoomDefault;

    if (!glfwInit()) return 1;
    GLFWwindow* win = glfwCreateWindow(app.winW, app.winH, "MOD ELITE 1.0", nullptr, nullptr);
    if (!win) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(win);
    int appliedVsync = app.cfg.vsync ? 1 : 0; glfwSwapInterval(appliedVsync);
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); io.IniFilename = nullptr;  // não cria imgui.ini
    setupStyle();
    // Nossos callbacks primeiro: o backend do ImGui os encadeia (chama o ImGui e depois o nosso).
    glfwSetKeyCallback(win, keyCb); glfwSetScrollCallback(win, scrollCb); glfwSetCursorPosCallback(win, cursorCb);
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    double last = glfwGetTime();
    while (!glfwWindowShouldClose(win)) {
        if (app.screen == Screen::Game) glfwPollEvents(); else glfwWaitEventsTimeout(0.1);
        double now = glfwGetTime(); float dt = (float)std::min(now - last, 0.1); last = now;
        app.fps.frame(dt);
        processNet(now);
        if ((app.cfg.vsync ? 1 : 0) != appliedVsync) { appliedVsync = app.cfg.vsync ? 1 : 0; glfwSwapInterval(appliedVsync); }

        if (app.screen == Screen::Game) {
            app.zoomTarget = std::max(app.cfg.zoomMin, std::min(app.cfg.zoomMax, app.zoomTarget));
            if (app.cfg.smoothZoom) app.zoom += (app.zoomTarget - app.zoom) * std::min(1.f, dt * app.cfg.zoomSpeed); else app.zoom = app.zoomTarget;
            // Direção: Auto Run (se ativo) ou mouse. Só é enviada ao servidor após o login do protocolo (ainda não implementado).
            float ang; bool have = false;
            if (app.autorun.steer(app.world, dt, ang)) have = true;
            else if (!app.autorun.active()) { int w, h; glfwGetWindowSize(win, &w, &h); ang = std::atan2((float)(app.mouseY - h * 0.5), (float)(app.mouseX - w * 0.5)); have = true; }
            if (have && app.playing && app.ws.isOpen() && now - app.lastSend > 0.04) {
                uint8_t b = proto::angleToByte(ang);
                if (b != app.lastAngle) { app.ws.sendBinary(&b, 1); app.lastAngle = b; }
                app.lastSend = now;
            }
        }

        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        switch (app.screen) {
            case Screen::Menu: drawMenu(); break;
            case Screen::SkinEditor: drawSkinEditor(); break;
            case Screen::SettingsScr: drawSettings(); break;
            case Screen::Game: drawGame(now); break;
        }
        ImGui::Render();
        int fw, fh; glfwGetFramebufferSize(win, &fw, &fh);
        glViewport(0, 0, fw, fh); glClearColor(0.045f, 0.05f, 0.065f, 1.f); glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
        if (!app.cfg.vsync && app.cfg.fpsCap > 0) {  // limitador de FPS opcional
            double target = 1.0 / app.cfg.fpsCap, spent = glfwGetTime() - now;
            if (spent < target) std::this_thread::sleep_for(std::chrono::duration<double>(target - spent));
        }
    }
    if (app.session.alive()) { app.session.onDeath(glfwGetTime()); }
    app.cfg.nick = app.nickBuf; app.cfg.server = app.serverBuf; saveCfg(); saveStats(); saveSkin();
    stopNet();
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    glfwDestroyWindow(win); glfwTerminate();
    return 0;
}
