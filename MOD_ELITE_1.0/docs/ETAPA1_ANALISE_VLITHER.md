# MOD ELITE 1.0 — Etapa 1: Análise do V.Lither 2.5

ZIP original: intacto (SHA-256 61f2493257691f2b9d5b8a4c0dae10dae829e90ebffe6ec374674eaf4c09f77f).
Extração apenas em pasta de trabalho separada (MOD_ELITE_1.0/analysis/).

## 1. Conteúdo
- `vlither.exe` (3,3 MB): PE32+ x86-64, **sem código-fonte**, símbolos removidos.
- `app/res/fonts`: 7 TTF (família regular, mono, iconfont) + licenças OFL.
- `app/res/shaders/bin`: 16 shaders SPIR-V compilados (fdv/fdf, bstv/bstf, bgv/bgf, mmv/mmf, sprv/sprf, bdv/bdf, fdrv/fdrf, bpv/bpf). Nomes sugerem: food, boost, background, minimap, sprites, body, etc.
- `app/res/textures`: `tex_atlas_8k.png` (7168x9216 RGBA, 7,2 MB) e `background_4k.png` (4096x3548, 6,5 MB).

## 2. Tecnologias CONFIRMADAS no binário
| Item | Evidência | 
|---|---|
| Linguagem/compilador | C/C++ compilado com MinGW-w64 (GCC 13/14), winpthreads |
| Vulkan | importa `vulkan-1.dll`; 104 símbolos `vk*`; VMA (Vulkan Memory Allocator) embutido |
| GLFW | GLFW 3.4 linkado estaticamente (backend Win32/WGL/EGL/OSMesa) |
| Dear ImGui | v1.92.5 WIP, via **cimgui** + backends `imgui_impl_glfw` (docking/viewports compilados) |
| Rede | **Mongoose** (cliente WebSocket nativo) + WS2_32; Origin `https://slither.com`; TLS **não habilitado** no build |
| Imagens | stb_image |
| Motor interno | módulo "thermite" (renderizador próprio) |

Ou seja: a suposição do enunciado (Vulkan + GLFW + ImGui) **está correta**, e há ainda Mongoose para o WebSocket.

## 3. Interface e recursos inferidos (IDs de widgets / textos)
Menu: Play, Skin editor, Settings, Quit; campos `nickname`, `ipv4_input`.
Settings: smooth zoom, zoom step, minimap size, boost effect (type/strength), death effect,
food (type/scale/float/flicker/uniform color), snake name font size, snake scores, player names outline,
leaderboard/stats/UI font size, laser color/thickness, crosshair, routes, instant restart, vsync, render mode, hotkeys (code/mode).
→ "laser" é o equivalente da nossa linha de assistência.

## 4. Texturas
O atlas contém sprites do próprio Slither.io (esferas coloridas dos corpos, bandeiras, acessórios, cruz de mira).
**São ativos protegidos do jogo/terceiros: NÃO serão copiados.** O MOD ELITE usará arte própria (esferas procedurais via shader).
Fontes: licença OFL (reutilizáveis com atribuição), mas usaremos fontes próprias/OFL escolhidas por nós.

## 5. Viabilidade de reprodução independente
Reproduzível com C++17 + GLFW + Vulkan + Dear ImGui + WebSocket nativo: menu, editor de skin (sobre cores/padrões válidos do jogo),
settings, zoom, linha de assistência (toggle), FPS, cronômetro, kills (apenas com confirmação), recorde, minimapa,
marcador de morte, tamanho visual da comida, auto-run (usa só dados que o servidor já envia ao cliente).

## 6. LIMITAÇÕES TÉCNICAS (importantes)
1. **Ambiente atual sem rede**: não consigo acessar slither.com, logo não posso verificar o protocolo atual (lista de servidores,
   handshake/desafio, formato dos pacotes) nem testar conexão real. Não vou inventar o protocolo.
2. **Sem toolchain Windows**: não há MinGW/MSVC, CMake, headers Vulkan/GLFW nem como instalá-los sem rede.
   Não consigo compilar nem gerar o `.exe` aqui, e não posso rodar/benchmarkar (FPS/CPU/memória) em Windows.
3. Skin: o servidor só aceita IDs de skin/cores existentes; "padrões" livres só podem ser visuais locais.
4. Kills: o protocolo só confirma morte de cobra; atribuir ao jogador exige critério conservador (ex.: colisão fatal causada por nós).
5. Servidores: só servidores reais do Slither.io (IP:porta obtidos da lista oficial); sem suporte a servidores arbitrários.
6. Mongoose sem TLS no V.Lither: indica conexão ws:// direta aos servidores de jogo.

## 7. Arquitetura proposta
C++17, Vulkan 1.1 (instance/swapchain simples, instanced quads para comida/corpo), GLFW 3.4, Dear ImGui (docking off),
Mongoose ou IXWebSocket para WS, nlohmann/json (config), CMake + MinGW-w64/MSVC.
Núcleo separado em `core/` (settings, stats, recordes, kills, minimapa, linha, auto-run) **sem dependência de GPU/SO** → testável em Linux aqui.
