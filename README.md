# Dark Sewer

Jogo de terror em C com [raylib](https://www.raylib.com/). Você está preso em um esgoto escuro, com uma lanterna que gasta bateria e um palhaço que anda pelos túneis. Para escapar é preciso achar três chaves, abrir a porta da sala do bueiro e sair por ele.

## Objetivo

1. Explore o esgoto e encontre as **3 chaves**.
2. Leve as chaves até a **porta com grade**, ao sul do corredor central. Ela abre sozinha quando você chega com as três.
3. Pise no **bueiro** dentro da sala para escapar.

Se o palhaço encostar em você, a partida acaba.

O palhaço enxerga mais longe quando a sua lanterna está acesa. Apagar a lanterna serve para se esconder.

## Controles

| Tecla | Ação |
|-------|------|
| Enter, Espaço ou clique em JOGAR | Começar a partida |
| R (na tela inicial) | Ver o ranking |
| W A S D ou setas | Andar |
| F | Ligar e desligar a lanterna |
| Q e E, ou a roda do mouse | Trocar o item selecionado |
| Espaço ou clique esquerdo | Usar o item selecionado |
| ESC ou P | Pausar e continuar |
| M (na pausa e nas telas de fim) | Voltar ao menu |
| Q (na pausa) | Sair do jogo |

## Itens

| Item | O que faz |
|------|-----------|
| Bateria | Recarrega a lanterna. A carga cheia dura 90 segundos com a lanterna acesa. |
| Chave | São três. Abrem a porta da sala do bueiro. |
| Isca | Largada no chão, atrai o palhaço por 8 segundos, mesmo que ele esteja perseguindo você. |
| Bússola | Enquanto está selecionada, mostra uma seta para a chave mais próxima. Depois aponta para a porta e para o bueiro. |

## Ranking

Quem escapa digita um nome e entra no ranking dos 10 melhores tempos. O ranking fica salvo no arquivo `ranking.txt`, ao lado do executável.

## Como compilar

O jogo precisa de um compilador C, do `make`, do `pkg-config` e da **raylib 5.5 ou mais nova**.

### Windows (MSYS2)

No terminal **MSYS2 UCRT64** (o MSYS2 só publica a raylib para esse ambiente):

```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib mingw-w64-ucrt-x86_64-pkgconf make
make
./jogo.exe
```

### Linux (Debian e Ubuntu)

```bash
sudo apt-get install build-essential cmake pkg-config libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git
cmake -S raylib -B raylib/build -DBUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build raylib/build
sudo cmake --install raylib/build
make
./jogo
```

### macOS

```bash
brew install raylib pkg-config
make
./jogo
```

## Estrutura do projeto

| Pasta ou arquivo | Conteúdo |
|------------------|----------|
| `src/` | Código-fonte |
| `include/` | Cabeçalhos |
| `assets/` | Imagens: mapa, máscara de paredes, personagens e itens |
| `sons/` | Músicas e efeitos sonoros |
| `makefile` | Compila o jogo no Windows, no Linux e no macOS |

Módulos principais em `src/`:

| Arquivo | Responsabilidade |
|---------|------------------|
| `main.c` | Laço do jogo, estados (menu, jogando, pausado, fim, vitória, ranking) e a partida |
| `colisao.c` | Máscara de paredes, colisão, grade de caminhada e busca de caminho |
| `inimigo.c` | Comportamento e desenho do palhaço |
| `barra_itens.c` | Barra de itens e desenho dos itens |
| `lanterna.c` | Carga e raio da lanterna |
| `ranking.c` | Ranking em arquivo |
| `ordem_desenho.c` | Ordem em que os elementos do chão são desenhados |
| `menu.c` | Tela inicial e telas de pausa, fim e vitória |
| `som.c` | Músicas e efeitos |
| `jogador.c` | Desenho dos personagens |

## Estruturas de dados e algoritmos

| Onde | O que é usado |
|------|---------------|
| Barra de itens (`barra_itens.c`) | Lista duplamente encadeada circular: cada slot aponta para o anterior e o próximo, e do último se volta ao primeiro |
| Ranking (`ranking.c`) | Lista encadeada ordenada: cada tempo novo é inserido já na posição certa |
| Ordem de desenho (`ordem_desenho.c`) | Insertion sort: os elementos do chão são ordenados pela posição vertical a cada quadro |
| Caminho do palhaço (`colisao.c`) | Busca em largura com fila sobre uma grade: gera um mapa de distâncias até o alvo |
| Sorteio de itens (`colisao.c`) | Busca em largura com fila: só sorteia posições que o jogador consegue alcançar |

## Créditos

- Arte do mapa: GØDXMODE.
- Biblioteca gráfica e de áudio: [raylib](https://www.raylib.com/).
