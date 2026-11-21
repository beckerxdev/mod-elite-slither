#!/bin/sh
# Testes do núcleo + cliente WebSocket (precisa de g++ e python3). Não depende de GLFW/ImGui.
set -e
cd "$(dirname "$0")"
C="src/core/settings.cpp src/core/stats.cpp src/core/session.cpp src/core/autorun.cpp src/core/protocol.cpp src/core/skin.cpp src/core/server.cpp"
g++ -std=c++17 -O2 -Wall -Wextra -pthread tests/core_tests.cpp $C -o /tmp/me_core_tests && /tmp/me_core_tests
g++ -std=c++17 -O2 -Wall -Wextra -pthread tests/ws_probe.cpp src/net/ws_client.cpp -o /tmp/me_ws_probe
python3 tests/mock_ws_server.py 18790 >/dev/null 2>&1 &
SRV=$!; sleep 1
/tmp/me_ws_probe 18790; kill $SRV 2>/dev/null || true
g++ -std=c++17 -O2 -pthread tools/bench_core.cpp src/core/autorun.cpp -o /tmp/me_bench && /tmp/me_bench
g++ -std=c++17 -Wall -Wextra -fsyntax-only -Itests/stubs src/app/main.cpp && echo "main.cpp: sintaxe OK (contra stubs; NAO compila com GLFW/ImGui reais aqui)"
