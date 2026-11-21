# Checklist honesto dos requisitos

Legenda: ✅ implementado e testado · 🟡 escrito, NÃO executado (UI/Windows) · ⛔ bloqueado (precisa de captura de protocolo)

| # | Requisito | Estado | Observação |
|---|---|---|---|
| 3 | Menu (stats, nome, IP:porta, PLAY/SKIN/SETTINGS/QUIT, "MOD ELITE 1.0") | 🟡 | Persistência do nome/servidor/stats ✅ (núcleo). Layout ImGui não executado. |
| 3 | Validação IP:porta | ✅ | Porta obrigatória; sem inventar servidores. |
| 4 | Skin Editor | 🟡 | Padrões/cores/persistência ✅ (núcleo); tela não executada. Cores/padrão são locais; só o ID é aceito pelo servidor. |
| 5 | Settings + atalhos configuráveis sem conflito | ✅/🟡 | Lógica e persistência ✅; tela 🟡. |
| 6 | Zoom (roda, atalhos, min/max, suave) | 🟡 | Parâmetros ✅; aplicação no app 🟡. |
| 7 | Linha de assistência (toggle, inicia DESLIGADA, 0 cálculo desligada) | ✅ | Testado: 1000 chamadas desligadas = 0 cálculos; não persiste ligada. Desenho 🟡; precisa do ângulo real (⛔ dados). |
| 8 | Auto Run | ✅ lógica / ⛔ dados | Simulação: sem colisão com borda nem parede de corpo, explora 68 células, interrompível. Precisa de dados reais do mundo. Envio do ângulo ao servidor ⛔ (login). |
| 9 | Kills confirmadas | ✅ lógica / ⛔ layout | Só conta contagem do servidor do NOSSO id; layout do pacote `k` a validar. |
| 10 | FPS | ✅/🟡 | |
| 11 | Cronômetro (zera ao iniciar, para ao morrer) | ✅ | |
| 12 | Recorde | ✅ lógica / ⛔ métrica | Fórmula de pontuação conforme comportamento público; validar com servidor real. |
| 13 | Minimapa | ✅ lógica / 🟡 desenho / ⛔ dados | Atualiza a 10 Hz; oculto = sem trabalho. |
| 14 | Marcador da última morte (persistente, remover, distância) | ✅ | Opção de apagar ao iniciar nova partida. |
| 15 | Tamanho visual da comida | 🟡 | Só escala local no desenho. |
| 16 | HUD configurável | 🟡 | |
| 17 | Desempenho | parcial | Medições de CPU do núcleo (Linux): linha off 0,003 µs, Auto Run 7,4 µs/frame médio, minimapa oculto 0,002 µs. FPS/GPU/memória **não medidos**. |
| 18 | Tecnologias | decisão | OpenGL 3 + ImGui DrawList em vez de Vulkan: sem poder compilar/testar aqui, Vulkan cru aumentaria muito o risco; 2D em lote via ImGui é suficiente. Pode migrar para o backend Vulkan do ImGui depois. |
| 19 | Segurança | ✅ | Sem credenciais, sem processos ocultos; log de pacotes é opt-in e local. |
| 21 | Agentes especializados | n/a | Não havia agentes disponíveis neste ambiente; todo o trabalho foi feito diretamente. |
