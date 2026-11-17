CC = gcc

ifeq ($(OS),Windows_NT)
    TARGET = jogo.exe
    LIBS_SISTEMA = -lopengl32 -lgdi32 -lwinmm
else ifeq ($(shell uname -s),Darwin)
    TARGET = jogo
    LIBS_SISTEMA = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
else
    TARGET = jogo
    LIBS_SISTEMA = -lGL -lpthread -ldl -lrt -lX11
endif

RAYLIB = $(shell pkg-config --cflags --libs raylib)
CFLAGS = -O2 -std=c99 -Wall -Wextra -Iinclude

.PHONY: all clean

all: $(TARGET)

SOURCES = src/main.c src/colisao.c src/jogador.c src/som.c src/menu.c src/barra_itens.c src/lanterna.c src/inimigo.c src/ranking.c src/ordem_desenho.c
HEADERS = include/main.h include/colisao.h include/jogador.h include/som.h include/menu.h include/barra_itens.h include/lanterna.h include/inimigo.h include/ranking.h include/ordem_desenho.h

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(RAYLIB) $(LIBS_SISTEMA) -lm

clean:
	$(RM) $(TARGET)