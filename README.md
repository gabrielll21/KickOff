# KickOff

Jogo de futebol arcade 2D, com visão superior e multiplayer local para **dois jogadores humanos**, desenvolvido como projeto acadêmico de **Computação Gráfica**. Cada pessoa controla um time de três jogadores: um recebe comandos diretos e os outros dois ajustam seu posicionamento automaticamente.

O objetivo é marcar mais gols durante **três minutos de tempo jogável**. O projeto é um candidato à versão 1.0 para apresentação; resultados, ressalvas e evidências estão em [AUDITORIA.md](AUDITORIA.md). O roteiro técnico está em [APRESENTACAO.md](APRESENTACAO.md).

## Tecnologias e requisitos

- Compilador com suporte a **C++14** (GCC ou equivalente).
- **CMake 3.16** ou superior.
- **GLFW 3.3** ou superior, incluindo arquivos de desenvolvimento.
- OpenGL e um driver/contexto compatível com as funções clássicas utilizadas: `glBegin`, `glOrtho`, `glPushAttrib` e primitivas imediatas.
- Ambiente gráfico local para abrir a janela. Linux foi a plataforma validada nesta auditoria; Windows e macOS não foram testados.

Não há fontes, imagens, texturas, sons ou outros assets externos necessários à execução. O jogo desenha seus elementos com OpenGL. A pasta `assets/` não é carregada pelo código atual.

Em Debian/Ubuntu, as dependências podem ser instaladas com:

```sh
sudo apt update
sudo apt install build-essential cmake libglfw3-dev libgl1-mesa-dev
```

## Compilar e executar

A partir da raiz do repositório:

```sh
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic"
cmake --build build/debug --parallel
./build/debug/KickOff
```

Para Release, use um diretório diferente:

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic"
cmake --build build/release --parallel
./build/release/KickOff
```

Esses diretórios são saídas locais do CMake, não arquivos para entrega/versionamento. A execução não depende do diretório de trabalho nem dos harnesses temporários usados na auditoria. Em caso de falha ao encontrar OpenGL ou GLFW, instale os pacotes de desenvolvimento; não apenas suas bibliotecas de execução.

## Controles

| Ação | Blue — humano 1 | Red — humano 2 |
|---|---|---|
| Movimento | W, A, S, D | Setas direcionais |
| Selecionar jogador mais próximo da bola | TAB | ENTER |
| Chutar | SPACE | SHIFT direito |
| Passar | Q | CTRL direito |

Movimento diagonal é normalizado. Seleção, chute e passe ocorrem uma vez por pressionamento: solte a tecla para tentar novamente. Um passe válido seleciona imediatamente seu destinatário, escolhido como o companheiro mais próximo do passador. Não há recepção automática.

**Direção do chute:** segue as teclas de movimento pressionadas. Sem direção pressionada, Blue chuta para a direita (+X) e Red para a esquerda (−X). Não há memória da última direção. A seta usa exatamente esse cálculo; fica mais intensa quando a bola está ao alcance. Passes são direcionados ao companheiro escolhido, independentemente das teclas de movimento.

| Navegação | Tecla |
|---|---|
| Iniciar no menu | ENTER |
| Reiniciar diretamente após o resultado | R |
| Voltar ao menu após o resultado | M |
| Sair | ESC ou fechar a janela |

R e M não atuam durante uma jogada ou a pausa de gol. O ENTER usado para iniciar não seleciona acidentalmente um jogador Red.

## Mecânicas e regras

- Dois times com três jogadores. Apenas o selecionado recebe movimento humano.
- Companheiros oferecem apoio ofensivo e marcação defensiva por zona, usando estimativa de posse e histerese. Não chutam nem passam automaticamente.
- Todos os jogadores participam de colisões circulares entre si e com a bola.
- A bola possui movimento independente, desaceleração de 5 unidades/s² e parada completa. Contatos suaves permitem condução por toques, sem posse obrigatória ou magnetismo.
- Velocidades iniciais: chute 35, passe 25 e contato físico entre 6 e 20 conforme intensidade. A bola começa com componentes de velocidade `(14, 9)`.
- Jogadores podem utilizar uma faixa externa para contornar a bola. O centro do jogador fica limitado a X ±55 e Y ±36,5, com restrições locais junto às caixas dos gols. A bola continua contida no campo, exceto pela abertura dos gols.
- As linhas laterais funcionam como paredes para a bola: não há lateral, escanteio ou tiro de meta.
- O gol exige que a bola caiba entre as traves e atravesse completamente a linha de fundo. A abertura é de 10,248 unidades e a profundidade é 2,44.
- O cronômetro diminui somente em `Playing`. Um gol atualiza o placar, congela a partida por dois segundos e restaura a jogada, preservando placar e tempo restante.
- Um gol válido no último intervalo conta. Após a pausa, a partida termina se o tempo acabou.
- Ao terminar, a partida fica congelada e exibe o vencedor ou empate. R restaura todos os estados para uma nova partida; M prepara o menu novamente.

Não há adversário controlado pela máquina, menu de configurações, pausa manual, stamina, impedimento, faltas ou disputa de pênaltis.

## Estrutura

```text
CMakeLists.txt           Configuração de compilação e dependências
src/main.cpp            Janela, input, estados, IA, colisões e renderização
src/entities/Player.*   Posição, movimento, raio e identificação do jogador
src/entities/Ball.*     Posição, velocidade, desaceleração e limites da bola
src/entities/Team.*     Identificação, nome, placar e referências aos jogadores
src/main.h              Cabeçalho histórico, sem uso no fluxo atual
README.md               Instalação e uso
APRESENTACAO.md         Roteiro técnico e demonstração
AUDITORIA.md            Evidências e limitações da auditoria final
```

`Team` guarda referências não proprietárias a jogadores criados no escopo da partida. Não realiza renderização. O estado tático, input e coordenação permanecem na lógica principal. Não há bibliotecas de UI, carregadores de modelos ou shaders próprios.

## Validação e limites

Consulte [AUDITORIA.md](AUDITORIA.md) para distinguir simulações, eventos GLFW, inspeção visual e execução real. Compilação e sanitizadores não substituem ensaio no computador da apresentação. A física é arcade e discreta; aglomerações podem exigir correções posicionais perceptíveis. O arquivo principal é extenso, mas não foi refatorado nesta entrega para evitar regressões.
