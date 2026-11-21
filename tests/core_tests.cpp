
#include <cmath>
#include <cstdio>
#include <set>
#include "../src/core/assist.h"
#include "../src/core/autorun.h"
#include "../src/core/fps.h"
#include "../src/core/minimap.h"
#include "../src/core/protocol.h"
#include "../src/core/score.h"
#include "../src/core/server.h"
#include "../src/core/session.h"
#include "../src/core/settings.h"
#include "../src/core/skin.h"
#include "../src/core/stats.h"
#include "test_util.h"

using namespace me;

static const float PI = 3.14159265f;

static float wrapA(float a) {
    while (a > PI) a -= 2 * PI;
    while (a < -PI) a += 2 * PI;
    return a;
}

static void testSettings() {
    SECTION("Settings: padrões, atalhos, conflitos, persistência");

    Settings s;
    CHECK(s.keys[(int)Action::ToggleLine] == 'L');

    for (int a = 0; a < kActionCount; ++a)
        CHECK(s.findKeyConflict(s.keys[a], a) == -1);

    Action c;
    CHECK(!s.setKey(Action::ToggleAutoRun, 'L', &c) && c == Action::ToggleLine);
    CHECK(s.keys[(int)Action::ToggleAutoRun] == 'R');
    CHECK(!s.setKey(Action::ToggleAutoRun, kKeyEscape));
    CHECK(s.setKey(Action::ToggleLine, 'K'));

    s.nick = "Teste";
    s.server = "1.2.3.4:444";
    s.foodScale = 1.7f;
    s.lineThickness = 5;
    s.lineColor[0] = 0.25f;

    CHECK(s.save("me_settings_test.ini"));

    Settings t;
    t.load("me_settings_test.ini");

    CHECK(t.nick == "Teste" && t.server == "1.2.3.4:444");
    CHECK(std::fabs(t.foodScale - 1.7f) < 1e-4 &&
          t.lineThickness == 5 &&
          std::fabs(t.lineColor[0] - 0.25f) < 1e-4);
    CHECK(t.keys[(int)Action::ToggleLine] == 'K');

    Settings u;
    u.foodScale = 99;
    u.zoomMax = 100;
    u.circleSegments = 2;
    u.clamp();

    CHECK(u.foodScale <= 3.f && u.zoomMax <= 5.f && u.circleSegments >= 6);

    FILE* f = std::fopen("me_bad_test.ini", "w");
    CHECK(f != nullptr);

    if (f) {
        CHECK(std::fputs("key0=76\nkey1=76\n", f) >= 0);
        CHECK(std::fclose(f) == 0);
    }

    Settings v;
    v.load("me_bad_test.ini");
    CHECK(v.keys[0] == 'L' && v.keys[1] == 'R');
}

static void testAssist() {
    SECTION("Linha de assistência: inicia desligada, toggle, sem cálculo quando off");

    AssistLine l;
    Vec2 a, b;

    CHECK(!l.active());

    for (int i = 0; i < 1000; ++i)
        CHECK(!l.compute({0, 0}, 0, 100, a, b));

    CHECK(l.computeCount() == 0);

    l.toggle();
    CHECK(l.active());

    CHECK(l.compute({10, 20}, 0, 100, a, b) &&
          std::fabs(b.x - 110) < 1e-3 &&
          std::fabs(b.y - 20) < 1e-3);

    CHECK(l.computeCount() == 1);

    l.toggle();
    CHECK(!l.active());

    CHECK(!l.compute({0, 0}, 0, 100, a, b) && l.computeCount() == 1);

    Settings s;
    CHECK(s.save("me_s2_test.ini"));

    Settings t;
    t.load("me_s2_test.ini");

    AssistLine fresh;
    CHECK(!fresh.active());
}

static void testSessionStats() {
    SECTION("Sessão: cronômetro, kills confirmadas, recorde, marcador de morte");

    Settings cfg;
    Stats st;
    Session ss(st, cfg);

    CHECK(ss.elapsed(100) == 0);

    ss.begin(100.0);
    ss.updateScore(50);
    ss.updatePos({123, -456});

    CHECK(std::fabs(ss.elapsed(130) - 30) < 1e-9);

    ss.onServerKillCount(1);
    ss.onServerKillCount(1);
    ss.onServerKillCount(3);
    ss.onServerKillCount(2);

    CHECK(ss.kills() == 3);

    ss.updateScore(400);

    CHECK(ss.onDeath(160.0));
    CHECK(std::fabs(ss.elapsed(999) - 60) < 1e-9);

    CHECK(st.bestScore == 400 &&
          st.lastKills == 3 &&
          std::fabs(st.lastTimeSec - 60) < 1e-9 &&
          st.games == 1);

    CHECK(st.marker.valid && st.marker.x == 123 && st.marker.y == -456);
    CHECK(!ss.onDeath(170.0) && st.games == 1);

    ss.begin(200.0);
    ss.updateScore(100);
    ss.updatePos({1, 1});

    CHECK(ss.kills() == 0 && ss.elapsed(205) == 5 && st.marker.valid);
    CHECK(!ss.onDeath(210.0) && st.bestScore == 400);
    CHECK(st.marker.x == 1);

    CHECK(st.save("me_stats_test.ini"));

    Stats l;
    l.load("me_stats_test.ini");

    CHECK(l.bestScore == 400 && l.games == 2 && l.marker.valid);

    l.resetStats();
    CHECK(l.bestScore == 0 && l.lastKills == 0 && l.marker.valid);

    ss.clearMarker();
    CHECK(!st.marker.valid);

    Settings cfg2;
    cfg2.markerClearOnNewGame = true;

    Stats st2;
    st2.marker = {true, 5, 5};

    Session s2(st2, cfg2);
    s2.begin(0);

    CHECK(!st2.marker.valid);

    Stats st3;
    Session s3(st3, cfg);
    s3.begin(0);
    s3.onDeath(5);

    CHECK(!st3.marker.valid);

    CHECK(formatTime(75) == "01:15" &&
          formatTime(3725) == "1:02:05");
}

static void testScore() {
    SECTION("Pontuação");

    ScoreTable t(411);

    int a = t.score(2, 0.0);
    int b = t.score(50, 0.0);
    int c = t.score(200, 0.5);

    CHECK(a >= 0 && b > a && c > b);
    CHECK(t.score(10, 0.9) > t.score(10, 0.1));

    std::printf(
        "   (info) score(2,0)=%d score(50,0)=%d score(200,0.5)=%d\n",
        a, b, c
    );
}

static void testMinimap() {
    SECTION("Minimapa: normalização, taxa de atualização, nada quando oculto");

    World w;
    w.gameRadius = 1000;
    w.meId = 1;

    Snake me;
    me.id = 1;
    me.head = {0, 0};
    me.angle = 1.f;
    w.snakes[1] = me;

    Snake o;
    o.id = 2;
    o.head = {500, -500};
    w.snakes[2] = o;

    Minimap m;

    CHECK(!m.update(w, 0.0, false) && m.updates() == 0);
    CHECK(m.update(w, 1.0, true));

    CHECK(m.hasMe() &&
          std::fabs(m.me().x - 0.5f) < 1e-5 &&
          std::fabs(m.me().y - 0.5f) < 1e-5);

    CHECK(m.others().size() == 1 &&
          std::fabs(m.others()[0].x - 0.75f) < 1e-5 &&
          std::fabs(m.others()[0].y - 0.25f) < 1e-5);

    CHECK(!m.update(w, 1.05, true) && m.updates() == 1);
    CHECK(m.update(w, 1.2, true) && m.updates() == 2);

    DeathMarker dm{true, 300, 400};

    CHECK(std::fabs(Minimap::distanceToMarker({0, 0}, dm) - 500) < 1e-3);
}

static void testFps() {
    SECTION("FPS");

    FpsCounter f;

    for (int i = 0; i < 120; ++i)
        f.frame(1.0 / 60);

    CHECK(f.fps() >= 59 && f.fps() <= 61);
}

static void testServer() {
    SECTION("Parse de servidor");

    std::string h, e;
    int p;

    CHECK(parseServer("1.2.3.4:444", h, p, e) &&
          h == "1.2.3.4" && p == 444);

    CHECK(parseServer(" ws://host-1.example.com:80/slither ", h, p, e) &&
          h == "host-1.example.com" && p == 80);

    CHECK(!parseServer("", h, p, e));
    CHECK(!parseServer("1.2.3.4", h, p, e));
    CHECK(!parseServer("1.2.3.4:0", h, p, e));
    CHECK(!parseServer("1.2.3.4:70000", h, p, e));
    CHECK(!parseServer("1.2.3.4:abc", h, p, e));
    CHECK(!parseServer("a b:80", h, p, e));
}

static void testProtocol() {
    SECTION("Protocolo: ângulo, ping, morte, kills");

    CHECK(proto::angleToByte(0) == 0 &&
          proto::angleToByte(PI) == 125 &&
          proto::angleToByte(-0.0001f) <= 250);

    for (float a = -10; a < 10; a += 0.01f)
        CHECK(proto::angleToByte(a) <= 250);

    Settings cfg;
    Stats st;
    Session ss(st, cfg);
    World w;

    w.meId = 7;

    GameClient gc(ss, w);

    ss.begin(0);
    ss.updatePos({9, 9});

    uint8_t pong[] = {0, 0, 'p'};

    gc.notePingSent(1.0);
    CHECK(gc.onPacket(pong, 3, 1.05) == 'p' && gc.pingMs() == 50);

    uint8_t k[] = {0, 0, 'k', 0, 8, 0, 0, 5};

    gc.onPacket(k, sizeof k, 2);
    CHECK(ss.kills() == 0);

    uint8_t k2[] = {0, 0, 'k', 0, 7, 0, 0, 2};

    gc.onPacket(k2, sizeof k2, 2);
    CHECK(ss.kills() == 2);

    uint8_t unk[] = {0, 0, 'Z'};

    gc.onPacket(unk, 3, 2);
    CHECK(gc.unknownPackets() == 1);

    uint8_t dead[] = {0, 0, 'v', 0};

    gc.onPacket(dead, 4, 5.0);
    CHECK(!ss.alive() && st.lastKills == 2);

    uint8_t shortp[] = {0, 1};

    CHECK(gc.onPacket(shortp, 2, 6) == 0);
}

static void testSkin() {
    SECTION("Skin: padrões e persistência");

    SkinSpec s;
    s.pattern = SkinSpec::Alternating;

    CHECK(s.colorAt(0, 10).r == s.c[0].r &&
          s.colorAt(1, 10).r == s.c[1].r);

    s.pattern = SkinSpec::Gradient;

    Rgb e = s.colorAt(9, 10);

    CHECK(std::fabs(e.r - s.c[1].r) < 1e-5);

    s.pattern = SkinSpec::Stripes;
    s.stripeLen = 2;

    CHECK(s.colorAt(0, 9).r == s.c[0].r &&
          s.colorAt(2, 9).r == s.c[1].r &&
          s.colorAt(4, 9).r == s.c[2].r);

    s.gameSkinId = 12;
    s.c[0] = {0.1f, 0.2f, 0.3f};

    CHECK(s.save("me_skin_test.ini"));

    SkinSpec l;
    CHECK(l.load("me_skin_test.ini"));

    CHECK(l.gameSkinId == 12 &&
          l.pattern == SkinSpec::Stripes &&
          std::fabs(l.c[0].g - 0.2f) < 1e-4);

    l.setDefault();

    CHECK(l.gameSkinId == 0 && l.pattern == SkinSpec::Solid);
}

// ---- Auto Run: simulação cinemática (velocidade 200/s, giro máx. 5 rad/s) ----

struct Sim {
    World w;
    AutoRun ar;

    float x = 0;
    float y = 0;
    float hd = 0;
    float minDist = 1e9f;
    int steps = 0;

    Sim() {
        w.gameRadius = 3000;
        w.meId = 1;

        Snake m;
        m.id = 1;
        m.radius = 15;

        w.snakes[1] = m;
        ar.toggle();
    }

    void run(int n, const std::vector<Vec2>* obst = nullptr, float obstR = 15) {
        const float dt = 0.05f;

        for (int i = 0; i < n; ++i, ++steps) {
            Snake& m = w.snakes[1];

            m.head = {x, y};
            m.angle = hd;

            float t;

            if (ar.steer(w, dt, t))
                hd += std::max(-5.f * dt, std::min(5.f * dt, wrapA(t - hd)));

            x += std::cos(hd) * 200 * dt;
            y += std::sin(hd) * 200 * dt;

            minDist = std::min(minDist, std::sqrt(x * x + y * y) - 0);

            if (obst) {
                for (auto& o : *obst) {
                    float d = std::hypot(x - o.x, y - o.y) - 15 - obstR;
                    borderObst = std::min(borderObst, d);
                }
            }

            float edge = w.gameRadius - std::sqrt(x * x + y * y) - 15;

            borderMin = std::min(borderMin, edge);

            cells.insert(
                ((int)std::floor((x + 3000) / 300)) * 1000 +
                (int)std::floor((y + 3000) / 300)
            );
        }
    }

    float borderObst = 1e9f;
    float borderMin = 1e9f;
    std::set<int> cells;
};

static void testAutoRun() {
    SECTION("Auto Run");

    {
        World w;
        w.meId = 1;

        Snake m;
        m.id = 1;

        w.snakes[1] = m;

        AutoRun a;
        float t = 0;

        CHECK(!a.steer(w, 0.05f, t));

        a.toggle();
        CHECK(a.active());

        a.interrupt();

        CHECK(!a.active() && !a.steer(w, 0.05f, t));
    }

    {
        World w;
        w.meId = 5;

        AutoRun a;
        a.toggle();

        float t;

        CHECK(!a.steer(w, 0.05f, t));
    }

    {
        Sim s;
        s.run(2400);

        std::printf(
            "   (info) arena vazia: folga mínima da borda=%.1f, células distintas visitadas=%zu\n",
            s.borderMin, s.cells.size()
        );

        CHECK(s.borderMin > 0);
        CHECK(s.cells.size() >= 40);
    }

    {
        Sim s;

        s.x = 2700;
        s.y = 0;
        s.hd = 0;

        s.run(600);

        std::printf(
            "   (info) rumo à borda: folga mínima=%.1f\n",
            s.borderMin
        );

        CHECK(s.borderMin > 0);
    }

    {
        Sim s;

        Snake o;
        o.id = 2;
        o.radius = 15;
        o.head = {900, -600};
        o.angle = 1.57f;

        std::vector<Vec2> pts;

        for (int i = 0; i <= 40; ++i) {
            Vec2 p{900.f, -600.f + i * 30.f};

            o.body.push_back(p);
            pts.push_back(p);
        }

        s.w.snakes[2] = o;

        s.x = 0;
        s.y = 0;
        s.hd = 0;

        s.run(1200, &pts, 15);

        std::printf(
            "   (info) parede: menor distância entre bordas=%.1f\n",
            s.borderObst
        );

        CHECK(s.borderObst > 0);
    }
}

int main() {
    testSettings();
    testAssist();
    testSessionStats();
    testScore();
    testMinimap();
    testFps();
    testServer();
    testProtocol();
    testSkin();
    testAutoRun();

    std::printf(
        "\nResultado: %d verificações OK, %d falhas\n",
        g_pass, g_fail
    );

    return g_fail ? 1 : 0;
}
