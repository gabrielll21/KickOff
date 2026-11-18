# KickOff — guia técnico de apresentação

## 1. Visão geral

Projeto acadêmico de Computação Gráfica: futebol arcade 2D top-down para dois humanos no mesmo computador. Cada time tem três jogadores; apenas um recebe input humano. O objetivo é conectar renderização geométrica, atualização temporal e colisões em uma partida completa, do menu ao resultado.

## 2. Tecnologias e estrutura

C++14, CMake, OpenGL clássico e GLFW. O código não usa sprites, texturas, modelos, áudio, shaders próprios ou biblioteca de UI.

- `src/main.cpp`: funções locais e estruturas simples para input, estados, IA, ações, colisões, janela e desenho.
- `Player`: coordenadas, velocidade de movimento, raio e time. `move` aplica direção × velocidade × intervalo.
- `Ball`: coordenadas, componentes da velocidade, raio, integração do atrito e ricochetes.
- `Team`: nome, identificação, placar e até três ponteiros para jogadores. Os jogadores pertencem aos arrays criados no `main`, não ao `Team`.
- `src/main.h`: cabeçalho histórico sem papel ativo. Não apresentá-lo como um controlador de jogo.

## 3. Renderização

A janela é criada pelo GLFW. O OpenGL usa projeção ortográfica (`glOrtho`) e coordenadas de mundo. `configureProjection` calcula um viewport proporcional e centralizado; barras externas preservam as proporções ao redimensionar.

Campo e letras são construídos com retângulos, linhas, arcos e triângulos. Os discos de bola/jogadores são aproximações poligonais com `GL_TRIANGLE_FAN`; contornos usam linhas. Placar e relógio reutilizam dígitos de sete segmentos. Menu e resultado usam glifos geométricos 5×7.

As setas usam uma base ortonormal 2D: direção de chute e vetor perpendicular. Os vértices da haste/ponta são calculados nessa base e transladados para a posição do jogador. Não há rotação de sprite. A bola é desenhada depois da seta para continuar legível. `currentKickDirection` é compartilhada pelo chute e pelo desenho; `canKick` é compartilhada pelo alcance real e pelo destaque.

## 4. Loop e atualização temporal

O loop lê `glfwGetTime`, calcula `deltaTime`, chama `updateMatch`, limpa a imagem, desenha, troca os buffers e processa os eventos GLFW. `glfwSwapInterval(1)` solicita sincronização vertical; o jogo não presume um número fixo de frames por segundo.

Durante Playing, `updatePlaying` divide o intervalo em subpassos. A quantidade considera a velocidade da bola, a velocidade humana e o raio da bola, limitando o deslocamento por passo. É uma simulação de passo variável subdividido, não um acumulador com passo fixo.

Por subpasso:

1. Guardar as posições anteriores e processar seleção/movimento humano.
2. Estimar posse e atualizar marcação defensiva.
3. Calcular os alvos dos dois times antes de mover os companheiros.
4. Movimentar companheiros e resolver colisões entre jogadores.
5. Capturar as tentativas de chute/passe antes dos contatos com a bola.
6. Integrar a bola, arbitrar ações e executar a ação manual válida.
7. Resolver impulso físico e separações, respeitando a proteção inicial.
8. Atualizar o cronômetro pelo intervalo processado e verificar gol.

Uma pressão gera uma tentativa por frame, não por subpasso. O último intervalo é limitado ao tempo restante. A detecção de gol tem precedência sobre a expiração daquele intervalo válido.

## 5. Colisões e limites

Jogadores e bola usam círculos. Primeiro compara-se distância ao quadrado com a soma dos raios ao quadrado. Apenas num contato calcula-se a normal de separação. Centros coincidentes têm uma direção determinística de fallback.

Jogadores que se moveram no passo compartilham correção; jogadores parados funcionam como obstáculos. As correções são acumuladas a partir de uma posição comum, em até oito passagens. Um agrupamento bloqueado pode retornar às posições anteriores válidas.

Na bola, aproximação é avaliada pela velocidade relativa projetada na normal. Um único contato recebe impulso por intervalo, selecionado pela maior aproximação; empates não escolhem arbitrariamente um time. Contatos leves usam resposta 6, com transição até 20 para impactos fortes. As outras passagens corrigem posição. A recuperação de aprisionamento procura uma saída livre em 16 direções; essa aproximação pode produzir reposicionamento perceptível em aglomerações.

O gramado mede 105×68. Centros da bola ficam normalmente em X ±51,5/Y ±33. Centros dos jogadores podem ir a X ±55/Y ±36,5. A faixa foi calculada pelos raios e por 0,5 de folga, permitindo contornar a bola sem ampliar sua área válida. Caixas dos gols têm restrição local para os jogadores.

## 6. Física da bola

A velocidade é um vetor `(vx, vy)`. A resistência de rolamento retira 5 unidades/s² da magnitude sem alterar sua direção livre. O deslocamento usa a velocidade média no tempo efetivamente percorrido; se a bola parar antes do fim do passo, o restante não produz movimento.

A velocidade inicial `(14,9)` tem magnitude aproximada 16,64. Sem contatos, a bola para em cerca de 3,33 s após percorrer 27,7 unidades. Ricochetes refletem componentes e preservam a distância excedente dentro dos subpassos curtos. Dentro da cavidade do gol, suas laterais limitam Y; o limite X não deve voltar abruptamente ao gramado.

## 7. Chutes, passes e seleção

- Blue: WASD, TAB, SPACE e Q.
- Red: setas, ENTER, SHIFT direito e CTRL direito.
- O chute usa input normalizado e começa em 35. Sem movimento pressionado: Blue +X, Red −X. Não existe memória da última direção.
- O passe exige o mesmo alcance do chute e começa em 25. Escolhe o companheiro mais próximo do passador, com desempate pelo menor índice, e aponta da bola para esse companheiro.
- Após o passe, o destinatário é selecionado imediatamente. O passador recebe IA; não há recepção ou correção automática de trajetória.
- As tentativas dos times são comparadas pela distância à bola; empate exato cancela ambas. Chute tem prioridade sobre passe do mesmo time.
- A proteção é vinculada ao jogador de origem, não ao recebedor. Separação continua ativa. A proteção termina por afastamento, ricochete ou contato legítimo posterior.
- TAB/ENTER escolhem o jogador mais próximo da bola; em empate, o atual é preservado quando também está entre os mais próximos.

## 8. IA de posicionamento

A IA atua apenas nos dois não selecionados de cada time. Não é um adversário autônomo nem executa ações com a bola.

A posse é uma estimativa por distância mínima dos seis jogadores, com margem linear de 3 para evitar trocas frequentes. Neutral mantém referência inicial + 15% da posição da bola. Attacking avança a referência 6 no sentido ofensivo; Defending recua 5.

A marcação compara seis atribuições distintas dos dois defensores aos três adversários, mantém atribuição anterior até melhorar mais de 2 e combina 40% do alvo defensivo com 60% de um ponto até 5 unidades na direção do próprio gol.

O apoio ofensivo gera até nove candidatos por companheiro e compara até 81 pares. Usa distância ao segmento finito para penalizar linhas bloqueadas, penaliza agrupamentos e preserva a referência. Histerese estabiliza os índices dos alvos.

Todos os alvos respeitam as regiões ±12/±8 e o gramado. A velocidade automática é 7,2, com zona de parada 0,5. Um ex-selecionado na faixa externa retorna com esse mesmo movimento, sem teletransporte ou busca de caminho.

## 9. Estados da partida

| Estado | Comportamento |
|---|---|
| Menu | Campo estático, 0–0 e 03:00; ENTER inicia |
| Playing | Input, IA, física e cronômetro ativos |
| Goal | Dois segundos de congelamento; placar e tempo preservados |
| Finished | Resultado e 00:00; R reinicia, M volta ao menu |

ESC fecha normalmente. R/M só atuam em Finished. Estados das teclas são sincronizados nas transições, evitando seleção Red pelo ENTER inicial e ações repetidas de teclas mantidas. Reset após gol preserva placar/tempo; reset completo os restaura e limpa estados da IA.

## 10. Roteiro de demonstração ao vivo

1. Compile e abra previamente o executável no computador da apresentação; confirme as duas combinações de teclado.
2. Mostre o menu, os dois humanos e as instruções. ENTER inicia.
3. Movimente ambos simultaneamente; mostre normalização diagonal e os indicadores de seleção.
4. TAB/ENTER transferem o controle. Mostre setas nas oito direções e o retorno à direção padrão quando soltar o movimento.
5. Aproxime-se da bola sem chutar: demonstre os toques suaves e o destaque de alcance.
6. Chute com SPACE/SHIFT direito. Mostre desaceleração e bola independente.
7. Passe com Q/CTRL direito e explique a seleção imediata do recebedor.
8. Recupere uma bola junto à lateral contornando-a pela faixa externa. Não atravesse as caixas dos gols.
9. Marque um gol e mostre placar, GOAL!, pausa, reset e cronômetro preservado.
10. Use o tempo restante para explicar IA e desenho geométrico. A partida possui três minutos jogáveis: aguarde o encerramento real, acrescido das pausas de gol.
11. Mostre resultado e 00:00; R reinicia. Ao final de uma partida, M retorna ao menu. ESC encerra.

Não há tecla para antecipar o resultado ou alterar a duração. Se o tempo da apresentação não comportar uma partida inteira, explique previamente essa duração e use uma captura do resultado como apoio, identificando-a como captura, sem alegar execução ao vivo.

## 11. Pontos para explicar na disciplina

Projeção ortográfica; viewport e proporção; sistema de coordenadas; aproximação de curvas por segmentos; composição por primitivas; ordem de desenho; buffers; vetores normalizados e perpendiculares; distância ponto–segmento; normais de colisão; atualização por tempo; separação entre dados das entidades e coordenação.

Não atribuir ao jogo: renderização 3D, perspectiva, iluminação, malhas externas, shaders próprios, rede, física geral de corpos rígidos ou planejamento tático avançado.

## 12. Limitações e ensaio

A física é discreta e arcade. A recuperação de aprisionamento pode reposicionar a bola visivelmente quando vários jogadores a comprimem. Os gols usam limites geométricos simplificados, não traves cilíndricas. A IA não tem busca de caminho e pode ser bloqueada por obstáculos. A tipografia diminui com a janela; monitores/teclados podem limitar combinações simultâneas. O arquivo principal extenso é uma melhoria futura, não uma refatoração da versão de apresentação.

Consulte [AUDITORIA.md](AUDITORIA.md) para evidências, correção realizada, incidente SIGTERM não reproduzido e testes manuais ainda não realizados. Ensaiar com dois humanos no equipamento de apresentação continua recomendado.
