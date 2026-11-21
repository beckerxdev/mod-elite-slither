# MOD ELITE 1.0

Cliente nativo (C++17, GLFW + Dear ImGui + OpenGL 3) para Slither.io, inspirado no V.Lither 2.5.
Identidade visual própria (nenhum recurso do V.Lither foi copiado). Arquivos originais do V.Lither não foram alterados.

## Instalação / execução (Windows 64 bits)
1. Instale **CMake 3.20+**, **Git** e **Visual Studio 2022 (carga "Desenvolvimento para desktop com C++")** *ou* **MinGW-w64**.
2. Dê dois cliques em `build_windows.bat` (precisa de internet na 1ª vez: baixa GLFW 3.4 e Dear ImGui v1.92.0).
3. O executável sai em `build\Release\MOD_ELITE_1.0.exe` (VS) ou `build\MOD_ELITE_1.0.exe` (MinGW). Execute-o.
4. (Opcional) testes: `ctest --test-dir build -C Release --output-on-failure`.

Dados do usuário ficam em `%APPDATA%\MOD_ELITE\` (`settings.ini`, `stats.ini`, `skin.ini`). Nenhuma senha/credencial é coletada, nenhum processo em segundo plano é criado.
A única rede usada é a conexão WebSocket ao servidor que VOCÊ digitar em IP:porta.

## Uso
- Menu: nome, IP:porta (obrigatório informar a porta), PLAY, SKIN EDITOR, SETTINGS, QUIT. Tudo é salvo automaticamente.
- Atalhos padrão (todos configuráveis em Settings > Atalhos; duplicatas são recusadas; ESC reservado = voltar):
  `L` linha de assistência (liga/desliga; **sempre inicia desligada**), `R` Auto Run, `=`/`-` zoom, roda do mouse = zoom,
  `M` minimapa, `H` HUD, `F` FPS, `X` limpar marcador de morte, `Espaço` boost.
- Auto Run: mover o mouse mais de ~40 px (configurável) ou apertar `R` devolve o controle imediatamente.

## STATUS REAL (leia)
| Item | Estado |
|---|---|
| Núcleo: configurações, atalhos sem conflito, estatísticas, recorde, cronômetro, kills, marcador de morte, minimapa, FPS, skin, linha (toggle), Auto Run | **Implementado e testado** (3086 verificações automatizadas, Linux) |
| Cliente WebSocket (RFC 6455) | **Implementado e testado** contra servidor independente (handshake com Origin, eco 5/300/70000 B, fragmentação, ping, close) |
| Interface (menu, skin editor, settings, HUD, minimapa, desenho) | **Escrita; NÃO compilada nem executada** (sem GLFW/ImGui/Windows no ambiente de desenvolvimento). Sintaxe do código próprio verificada com stubs. Pode exigir pequenos ajustes de API ao compilar. |
| Executável Windows `.exe` | **Não gerado aqui** (sem toolchain Windows). Gere com `build_windows.bat`. |
| Login/handshake do protocolo do Slither.io e leitura do mundo (cobras, comida, posição, pontuação) | **NÃO implementado.** Ver abaixo. |
| Benchmarks de FPS/GPU/memória | **Não realizados** (precisam do app rodando no Windows). Só há medições de CPU do núcleo (`tools/bench_core.cpp`). |

### Por que o jogo ainda não é jogável
O ambiente de desenvolvimento não tinha acesso à internet, então não foi possível observar o protocolo atual do Slither.io
(handshake/desafio, formato dos pacotes de posição e comida). Não inventei esses formatos. Hoje o app conecta ao servidor por WebSocket,
entra na tela de jogo e mostra o HUD, **mas sem login nem dados de mundo** (banner amarelo avisa). Implementados e testados somente:
ping (`p`), morte (`v`) e kills do nosso snake (`k`, layout a validar). Auto Run, minimapa, linha e marcador funcionam sobre o `World`
e já estão prontos para receber dados reais assim que o parser de pacotes for concluído.

### Como destravar (próximo passo)
1. Em Settings > Dados, ative **"gravar pacotes recebidos"**; clique PLAY em um servidor real (ip:porta da lista oficial do jogo).
2. Envie-me o `capture.log` (em `%APPDATA%\MOD_ELITE\`). Com ele implemento e testo o handshake e o parser de mundo
   (`src/core/protocol.cpp`), ligando `App::playing`, e então as demais funções ficam ativas.

## Estrutura
```
src/core   lógica sem GPU/SO (testada)      src/net   cliente WebSocket (testado)
src/app    janela/UI (GLFW+ImGui)           tests/    testes + servidor WS de teste + stubs
tools/     bench_core.cpp                   docs/     análise do V.Lither
```

## Gerar o .exe sem instalar nada (GitHub Actions)
1. Crie um repositório no GitHub e envie esta pasta (arrastar os arquivos pelo site funciona).
2. Aba **Actions** > "Build MOD ELITE 1.0" > **Run workflow**.
3. Ao terminar (~5 min), baixe o artefato **MOD_ELITE_1.0_windows_x64** (contém o `.exe`).
Se a compilação falhar, copie o log de erro e me envie: corrijo.
