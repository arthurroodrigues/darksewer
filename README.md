# Dark Sewer

Jogo de terror em C com [raylib](https://www.raylib.com/). Você está preso em um esgoto escuro, com uma lanterna que gasta bateria e um palhaço que anda pelos túneis. Para escapar é preciso achar três chaves, abrir a grade que fecha o poço e sair por ele.

## Objetivo

1. Explore o esgoto e encontre as **3 chaves**.
2. Leve as chaves até a **grade de ferro** que fecha o poço, na sala central. Ela abre sozinha quando você chega com as três.
3. Pise no **poço** para escapar.

A partida acaba se o palhaço encostar em você ou se a bateria da lanterna chegar a zero (apagão seguido do susto). Pelo mapa há **3 baterias** e **1 bússola** para ajudar.

O palhaço enxerga mais longe quando a sua lanterna está acesa. Apagar a lanterna serve para se esconder. Você também pode se esconder dentro de **guarda-roupas** e **caixas** espalhados pelo esgoto (veja abaixo).

## Controles

| Tecla | Ação |
|-------|------|
| Enter, Espaço ou clique em JOGAR | Iniciar a introdução; Enter avança as falas e libera o movimento após a última |
| R (na tela inicial) | Ver o ranking |
| W A S D ou setas | Andar |
| F | Ligar e desligar a lanterna |
| H ou clique direito | Entrar no esconderijo (perto de um guarda-roupa ou caixa) e sair dele |
| Q e E, ou a roda do mouse | Trocar o item selecionado |
| Espaço ou clique esquerdo | Usar o item selecionado |
| ESC ou P | Pausar e continuar |
| M (na pausa e na tela de derrota) | Voltar ao menu |
| Q (na pausa) | Sair do jogo |
| Enter (na tela de derrota) | Tentar de novo |
| Enter (no ranking) | Começar uma nova partida |
| M ou ESC (no ranking) | Voltar ao menu |
| Letras, números e espaço (na vitória) | Digitar o nome, com até 10 caracteres; Backspace apaga e Enter confirma |

## Itens

| Item | O que faz |
|------|-----------|
| Bateria | Recarrega a lanterna quando usada (Espaço ou clique esquerdo), mas só se a carga estiver em até 90%. A carga cheia dura 75 segundos com a lanterna acesa. |
| Chave | São três. Ao chegar na grade com todas, elas abrem a grade e são consumidas. |
| Bússola | Enquanto está selecionada, mostra uma seta para a chave mais próxima. Depois aponta para a grade e para o poço. |

A barra de itens tem 5 slots, e cada slot guarda até 3 itens.

## Esconderijos

Há **4 guarda-roupas** e **5 caixas**, em posições sorteadas a cada partida, sempre perto das paredes e afastados do poço e da grade. Chegue perto de um e aperte **H** (ou clique com o botão direito) para entrar.

- Você fica escondido por no máximo **5 segundos**; aperte H de novo para sair antes, ou o jogo tira você sozinho.
- Enquanto está escondido, o palhaço não vê nem pega você. Se ele estava na sua cola, ele desiste e segue patrulhando para longe do esconderijo, sem ficar parado na porta.
- Depois de sair você tem **1,5 s** em que o palhaço não consegue te pegar, e só pode usar outro esconderijo depois de **8 s**.
- Escondido, você não anda, não coleta itens e não escapa pelo poço.

Os valores ficam no topo de `src/main.c` (`DURACAO_MAXIMA_ESCONDIDO`, `RECARGA_ESCONDERIJO`, `PROTECAO_SAIDA_ESCONDERIJO`) e as quantidades em `include/esconderijo.h`.

## Ranking

<<<<<<< HEAD
Quem escapa digita um nome e entra no ranking dos 10 melhores tempos. O ranking fica salvo no arquivo `ranking.txt`, ao lado do executável.
O comando `make clean` remove o executável e esse arquivo, reiniciando o ranking para a próxima execução.
=======
Ao escapar, uma imagem de fuga aparece e em seguida você digita um nome. Quem escapa entra no ranking dos 10 melhores tempos. O ranking fica salvo no arquivo `ranking.txt`, ao lado do executável.
>>>>>>> 03ba19f5773e8e0f24a2f00430393f9b7926942f

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
| `assets/` | Mapa e máscara de colisão, tela inicial, imagem de fuga, sprites do jogador e do palhaço (parado e andando), texturas dos esconderijos, bateria, GIF e áudio do jumpscare |
| `sons/` | Músicas e efeitos sonoros |
| `.github/workflows/build.yml` | Compilação automática no Linux, no macOS e no Windows |
| `makefile` | Compila o jogo no Windows, no Linux e no macOS |

Módulos principais em `src/`:

| Arquivo | Responsabilidade |
|---------|------------------|
| `main.c` | Laço do jogo, estados (menu, introdução, diálogo, jogando, pausado, apagão, jumpscare, tela de fuga, fim, vitória e ranking) e a partida |
| `colisao.c` | Máscara de paredes, colisão, grade de caminhada, busca de caminho e sorteio de posições alcançáveis |
| `inimigo.c` | Comportamento, animação de andar e desenho do palhaço |
| `barra_itens.c` | Barra de itens e desenho dos itens |
| `lanterna.c` | Carga e raio da lanterna |
| `ranking.c` | Ranking em arquivo |
| `esconderijo.c` | Texturas, desenho e proximidade dos guarda-roupas e caixas |
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
| Sorteio de itens e esconderijos (`colisao.c`) | Busca em largura com fila: só sorteia posições que o jogador consegue alcançar; os esconderijos ainda exigem uma parede por perto |

## Créditos

- Biblioteca gráfica e de áudio: [raylib](https://www.raylib.com/).
