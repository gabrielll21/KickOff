# KickOff — auditoria final, Stage 26

Data: 18/11/2026. Base inspecionada: `15b4b46` (`feat: allow players to recover balls beyond field boundaries`). Candidato à versão 1.0, sem criação de commit, tag ou publicação.

## 1. Resumo executivo

**Recomendação: B — PRONTO COM RESSALVAS.** O jogo compilou em diretórios novos, completou partidas simuladas e uma partida real e encerrou normalmente por ESC. Um defeito reproduzido nas laterais internas dos gols foi corrigido sem mudar os parâmetros físicos. Há uma ressalva reproduzida na recuperação de aprisionamento em aglomerações, além da necessidade de ensaio com dois humanos no computador da apresentação.

Não se afirma ausência universal de defeitos. Nenhuma funcionalidade foi adicionada. O código dos controles, da IA, dos estados e da renderização não foi alterado nesta auditoria.

## 2. Arquitetura

- `src/main.cpp`: janela GLFW, projeção, desenho, controles, seleção, ações, subpassos, colisões entre entidades, estados e IA. Na base inspecionada tem 1.876 linhas.
- `Player`: posição, movimento, raio, velocidade e identificação do time.
- `Ball`: posição/velocidade, integração do rolamento e limites/ricochetes.
- `Team`: identificação, nome, placar e até três referências não proprietárias a jogadores. Valida duplicação, capacidade e time na inclusão.
- `src/main.h`: cabeçalho histórico sem uso no fluxo atual; preservado.

Os jogadores são arrays locais criados antes dos times; os ponteiros mantidos por `Team` permanecem válidos durante a partida. Os únicos recursos gráficos próprios são a janela/contexto GLFW e estado OpenGL imediato; não há texturas, buffers de GPU próprios, fontes externas ou assets carregados. O diretório `assets/` está vazio.

## 3. Auditoria da física

A bola mantém componentes X/Y. A desaceleração é 5 unidades/s², integrada pela velocidade média e pelo tempo até parar. Testes cobriram magnitudes inicial, 20, 25 e 35, direções variadas, repouso exato, paredes, abertura/traves, correções de duração zero e ação próxima da parede.

O movimento usa subpassos limitados pelo raio e velocidades atuais. Jogadores têm raio 2 e a bola raio 1. Contatos usam normal circular, aproximação relativa e um único impulso por intervalo. Respostas suaves começam em 6; impactos fortes usam 20. Separação física e recuperação de aprisionamento permanecem ativas.

**Defeito corrigido — ALTO:** quando a bola já estava parcialmente na cavidade do gol e cruzava lateralmente sua abertura, o limite X voltava ao limite do gramado. Exemplo anterior: `(53,3; 4,1)`, velocidade `(0; 20)`, intervalo 0,01 s → `(49,7; 4,29975)`: salto de 3,60554 unidades, embora o movimento integrado fosse 0,19975. O caso simétrico ocorria nos quatro quadrantes. Uma trajetória iniciada validamente em `(50; 1)` também reproduziu o salto, sem posição inicial inválida.

Em `Ball::update`, o estado geométrico de entrada agora identifica a cavidade e rebate nas laterais Y antes de determinar os limites X. Depois da correção, o exemplo termina em `(53,3; 3,94825)`, com Y refletido e sem alteração de X. Trajetórias legais continuam marcando gol. Não foram alterados raios, velocidades, atrito, abertura ou profundidade.

**Ressalva — MÉDIO, preservada:** sob pressão de vários jogadores contra uma borda/canto, `releaseTrappedBall` pode escolher uma saída distante e reposicionar a bola visivelmente. Uma cópia temporária instrumentada registrou:

```text
antes=(-51.3842,-32.9979)
após separações=(-51.4100,-33.0000)
após recuperação=(-48.0693,-29.6594)
correção total=4.70471; subpasso=0.00694444
```

Isso é distinto do defeito dos gols. A causa foi localizada na busca radial de saída do aprisionamento existente. Não houve NaN, sobreposição persistente, gol indevido detectado ou impedimento de concluir a partida. Não foi aplicado limite arbitrário à correção: isso poderia deixar a bola presa ou sobreposta. Uma revisão desse mecanismo exige análise separada dos contatos, preservando as invariantes, e não foi incluída como refatoração da auditoria.

## 4. Auditoria dos controles

Aprovados por testes automatizados e eventos GLFW: WASD/setas, TAB/ENTER, SPACE/SHIFT direito, Q/CTRL direito, transferência após passe, ações fora do alcance consumidas e teclas mantidas.

Chutes nas oito direções e em repouso preservam a regra existente: Blue +X e Red −X sem input de movimento. Passes foram verificados em oito direções geométricas de destinatário para ambos os times; a direção é bola → companheiro, não a tecla direcional.

Ações mantêm alcance de 3,5 para os raios atuais, velocidade inicial 35/25 e proteção do jogador de origem. Arbitragem entre times mantém a ação mais próxima e cancela empate exato; chute tem prioridade sobre passe do mesmo time. O recebedor continua sendo o companheiro mais próximo do passador, com desempate por índice.

A seta compartilha `currentKickDirection` com o chute e `canKick` com seu destaque; não armazena direção independente.

## 5. Auditoria da IA

Testes cobriram posse com seis jogadores e margem linear de 3, Neutral/Attacking/Defending, distribuição conjunta dos defensores, histerese de marcação, alvo combinado 40%/60%, apoio por candidatos e pares, projeção no segmento finito, custos, empates, índices inválidos, estabilidade e exclusão do selecionado.

A marcação usa seis atribuições distintas; o apoio compara até 81 pares. Os dois times calculam alvos antes de mover companheiros. Os testes com entradas repetidas produziram resultados determinísticos na mesma cadência. A IA continua em 7,2, tolerância 0,5 e regiões ±12/±8. Não há busca de caminho: bloqueio físico é uma limitação, não foi substituído por outra IA.

## 6. Auditoria das laterais

Valores confirmados no código:

| Área | Limites dos centros |
|---|---|
| Gramado, linhas físicas | X ±52,5; Y ±34 |
| Jogadores | X ±55; Y ±36,5, com exclusão das caixas dos gols |
| Bola fora das aberturas | X ±51,5; Y ±33 |
| Bola no fundo do gol | X até ±53,94, respeitando abertura e raio |

Foram executados trajetos de contorno e recuperação nas quatro bordas e nos quatro cantos, para Blue e Red, a 30/60/144 FPS. Não são apenas testes com o jogador colocado atrás da bola: os harnesses percorrem caminhos ao redor dela. Eventos GLFW repetiram esses trajetos a 60 FPS. Também passaram aproximações legais junto às traves, limites externos, retorno de ex-selecionados ao gramado pela IA e reinícios.

## 7. Auditoria visual

Inspeção das primitivas, cores e ordem de desenho, complementada por capturas geradas nesta auditoria. Foram verificados menu, jogo, resultado e recuperação externa em 1280×720, 960×640 e 640×960. Campo centralizado, gols, círculos, setas, seleção, cronômetro e placar continuaram legíveis nas capturas. O HUD fica acima do envelope externo dos jogadores/setas.

O teste geométrico conta duas setas apenas em Playing, verifica posições, cores/intensidade, alcance, seleção e correspondência com o chute. Menu, Goal e Finished não desenham setas. Em janelas pequenas a interface diminui proporcionalmente; não foi redesenhada.

## 8. Estabilidade

**Execução real:** binário Release iniciado a partir de `/tmp`, sem depender de assets ou do diretório do projeto. ENTER foi enviado à janela identificada pelo PID. O processo permaneceu ativo por 185,237 s; capturas aos 30/60/90/120/150/185 s mostraram a evolução até `DRAW!`, 0–0 e 00:00. ESC encerrou com **código 0**. Essa partida real não recebeu movimentos após ENTER.

**Estresse simulado:** nove partidas, três consecutivas por cadência de FPS, com 1.620 segundos jogáveis no total, ambos os humanos sintéticos perseguindo a bola, seleção e tentativas frequentes de chute/passe. Foram observados 18 gols nesse conjunto, partidas concluídas e reinícios. Os testes validaram finitude, limites, separação dos jogadores, bola livre de sobreposição ao final dos frames e velocidade até 35. Não equivalem a nove partidas de duração real.

**SIGTERM anterior:** incidente não reproduzido, origem não determinada. A inspeção encontrou uma chamada a `glfwSetWindowShouldClose`, acionada por ESC. O loop também termina quando GLFW sinaliza fechamento da janela. Não há `exit`, `abort`, envio/tratamento de sinais ou thread de encerramento no código do jogo. Há `glfwDestroyWindow` e `glfwTerminate` no caminho normal e limpeza quando a criação da janela falha. O relato 143/SIGTERM não prova crash de memória nem autoriza atribuir a causa ao IDE, ao usuário ou ao sistema. Sem logs do episódio original, não há evidência para uma correção específica ou handler.

A sessão CLion criada para inspeção confirmou o caminho `main → updateMatch → updatePlaying → Ball::update`, com dados locais reais. Como logpoints nativos não forneceram saída observável, foi usado breakpoint suspensivo; a evidência do defeito específico veio dos probes reproduzíveis. Os dois breakpoints da auditoria foram removidos, os quatro breakpoints de exceção do usuário foram preservados e a sessão foi encerrada por ESC.

## 9. Desempenho e comparação de FPS

Condições simples equivalentes, após um segundo:

| Cadência simulada | Movimento Blue/Red | Relógio restante | Velocidade livre da bola | Distância livre da bola |
|---|---:|---:|---:|---:|
| 30 FPS | 12,0000458 / 12,0000458 | 179 s | 11,6433077 | 14,1433144 |
| 60 FPS | 12,0000458 / 12,0000458 | 179 s | 11,6433315 | 14,1433220 |
| 144 FPS | 12,0000563 / 12,0000563 | 179 s | 11,6433134 | 14,1433249 |

Diferenças pequenas são compatíveis com aritmética float. Os testes de rolamento também verificaram parada em aproximadamente 3,33 s e 27,7 unidades para `(14,9)`, com tolerância de um frame no instante amostrado e 0,004 unidade na distância.

Estresse com ASan/UBSan, compilação `-O1`, somente CPU da atualização:

| FPS | Frames em três partidas | Gols | Tentativas de ação | Média por atualização | Maior deslocamento da bola num frame |
|---|---:|---:|---:|---:|---:|
| 30 | 16.563 | 6 | 2.166 | 96,65 µs | 4,13083 |
| 60 | 33.483 | 9 | 2.166 | 48,43 µs | 3,11207 |
| 144 | 78.624 | 3 | 2.160 | 25,74 µs | 4,76965 |

As médias incluem a sobrecarga dos sanitizadores, não renderização, vsync ou input do sistema. Não são garantia de FPS no computador de apresentação. Resultados/placares diferentes numa disputa longa são esperados com colisões discretas e entradas amostradas por frame; o teste de repetição na mesma cadência foi determinístico. Os maiores deslocamentos estão associados à ressalva de aprisionamento, não são apresentados como movimento livre correto.

## 10. Compilação

Ambiente Linux; GCC 13.3.0, CMake 3.28.3 e GLFW 3.3.10. Build Debug e Release criados em diretórios novos, depois recompilados com `--clean-first` após a correção. C++14, extensões desabilitadas pelo CMake, flags `-Wall -Wextra -Wpedantic`. Ambos passaram sem warnings. CMakeLists.txt foi preservado.

```sh
cmake -S . -B /tmp/kickoff-audit26-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-Wall -Wextra -Wpedantic'
cmake --build /tmp/kickoff-audit26-debug --parallel
cmake -S . -B /tmp/kickoff-audit26-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS='-Wall -Wextra -Wpedantic'
cmake --build /tmp/kickoff-audit26-release --parallel
git diff --check
```

Dependências de desenvolvimento confirmadas localmente: CMake, compilador C++, GLFW e OpenGL. Não se dependeu do `cmake-build-debug` para os builds de entrega; ele foi usado somente pela sessão de depuração do IDE. Não foi executada reinstalação de pacotes nem compilação em outro sistema operacional.

## 11. Sanitizadores e evidências

Sete harnesses recompilados com `-O1 -g -std=c++14 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer`. Todos: compilação 0, execução 0, sem diagnóstico do compilador nem ASan/UBSan. O teste GLFW usou o contexto gráfico real e não desabilitou a detecção padrão de leaks. A suíte de estresse também passou com ASan/UBSan. Isso cobre os cenários executados, não prova ausência de qualquer vazamento em todos os drivers.

| Suíte | Evidência principal |
|---|---|
| control | Condução, oito direções, alcance, proteção e conflitos |
| ball | Rolamento, paredes, traves, gols e ações perto das paredes |
| arrows | Contagem, geometria, cores, alcance, estados e ausência de escrita física |
| recovery | Trajetos nas bordas/cantos e retorno da IA |
| tactics | Posse, marcação, apoio, estabilidade e contatos |
| lifecycle | 180 s, gols finais, estados, teclas mantidas, R/M e resets |
| glfw | Eventos X11 consumidos pelo GLFW, resize, navegação e recuperação |

Fontes e logs temporários estão em `/tmp/kickoff-audit26/` e nos harnesses `/tmp/kickoff_*`. O runner `run_suites.py` registra comandos, stdout/stderr e códigos em `*.log`/`suite-results.json`. Outros registros: `stress.log`, `trace_stress.log`, `fps_metrics.log`, `build-debug.log`, `build-release.log`, `real-match.log`, `real-match-result.json` e capturas `real-*.png`. Esses arquivos são evidências locais transitórias; não são dependências, testes versionados ou parte necessária da distribuição.

## 12. Documentação

README atualizado com requisitos, comandos verificados, controles completos, regras, estrutura, contexto acadêmico e limitações. APRESENTACAO.md criado com explicação técnica baseada no código e roteiro de demonstração. Este relatório conserva a distinção entre resultados executados, inspeção e limitações. Não foram criados slides, dados pessoais, autoria presumida ou dependências fictícias.

## 13. Problemas encontrados

| Gravidade | Achado | Resultado |
|---|---|---|
| ALTO | Salto da bola ao sair lateralmente da cavidade do gol | Corrigido e retestado |
| MÉDIO | Reposicionamento perceptível ao escapar de aglomerações contra bordas | Reproduzido; mecanismo existente preservado e documentado |
| MÉDIO — entrega | README incompleto e desatualizado | Corrigido |
| BAIXO | Concentração da lógica em main.cpp e cabeçalho histórico sem uso | Registrado; sem refatoração |
| Não determinada | Episódio isolado de SIGTERM relatado | Não reproduzido; sem atribuição de causa |

Nenhum crash reproduzível, corrupção de memória ou compilação quebrada foi encontrado nos testes executados.

## 14. Correções realizadas

1. `src/entities/Ball.cpp`: corrigido o limite lateral da cavidade do gol; prova anterior e posterior em §3. A alteração foi feita isoladamente, antes de repetir as suítes. Não houve ajuste de parâmetros.
2. `README.md`: substituído conteúdo de planejamento por instruções e funcionalidades reais.
3. `APRESENTACAO.md`: novo guia solicitado.
4. `AUDITORIA.md`: novo registro de evidências, classificação e revisão final.

Nenhuma alteração em Player, Team, main.cpp, CMake, controles, IA, cores ou interface. Não foi removido arquivo existente.

## 15. Limitações conhecidas

- Recuperação radial de aprisionamento pode reposicionar a bola em aglomerações; não foi certificada ausência de teletransporte nesse cenário.
- Física e traves são aproximadas por subpassos e limites, não por detecção contínua de círculos contra cilindros.
- IA de posicionamento sem busca de caminho pode ser bloqueada; não há garantia de convergência em todo agrupamento possível.
- Janela pequena reduz a tipografia; hardware/driver diferente e ghosting de teclado exigem ensaio local.
- Não foram realizados três jogos consecutivos em tempo real nem sessão prolongada com dois humanos físicos. Houve nove jogos simulados e uma partida real de 180 s sem movimento humano após iniciar.
- Pausa de gol de dois segundos foi validada por tempo simulado e eventos GLFW, não cronometrada manualmente numa partida com gol real.
- Não foram testados Windows/macOS, instalação em sistema recém-formatado, todos os drivers ou todos os estados possíveis.

## 16. Estado do Git

O repositório estava limpo no início. Histórico recente de dez commits foi inspecionado; a base é `15b4b46`. Nenhum executável, build, objeto ou harness descartável está versionado. Há configurações `.idea/` versionadas, preservadas por serem arquivos existentes do projeto. O diretório local `cmake-build-debug` usa seu próprio `.gitignore`; `assets/` não contém recursos.

Alterações desta auditoria: README.md e Ball.cpp modificados; APRESENTACAO.md e AUDITORIA.md novos. `git diff --check` aprovado. Os testes ficaram fora do repositório. Nenhum reset, remoção destrutiva, commit, push ou tag foi executado.

## 17. Checklist final

| Estado | Item |
|---|---|
| APROVADO | Builds limpos Debug/Release em C++14, sem warnings |
| APROVADO | ASan/UBSan nos sete harnesses e no estresse |
| APROVADO | Controles, ações, alcance, seleção e setas nos cenários executados |
| APROVADO | Recuperação nos quatro lados/cantos para ambos os times |
| APROVADO | IA, histerese e limites nos testes determinísticos |
| APROVADO | Menu/Playing/Goal/Finished, cronômetro e resets |
| APROVADO | Partida real concluída e ESC com retorno 0 |
| APROVADO | Documentação e revisão de artefatos/Git |
| APROVADO COM RESSALVAS | Física em aglomerações: separação válida, mas pode haver reposicionamento perceptível |
| APROVADO COM RESSALVAS | Visual nas três resoluções; legibilidade dependente do tamanho efetivo da janela |
| NÃO TESTADO | Origem do SIGTERM anterior, sem evidências suficientes para reproduzir |
| NÃO TESTADO | Três partidas reais consecutivas e ensaio físico com dois humanos no equipamento de apresentação |
| REPROVADO na base; APROVADO após correção | Continuidade da posição da bola nas laterais internas dos gols |

## 18. Recomendação

**B — PRONTO COM RESSALVAS**, para apresentação acadêmica com as limitações explicitadas. Recomenda-se ensaiar teclado/monitor/driver no local e explicar a natureza arcade/discreta da física. A correção do salto nos gols é necessária e está pronta para revisão; a mudança do mecanismo de aprisionamento não foi embutida nesta entrega como uma nova física. Nenhuma liberação/tag 1.0 foi criada: a decisão de entrega permanece com o usuário após revisão.
