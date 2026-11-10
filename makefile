CC = gcc
TARGET = jogo.exe
RAYLIB = $(shell pkg-config --cflags --libs raylib)
CFLAGS = -O2 -std=c99 -Wall -Wextra -Iinclude

.PHONY: all clean

all: $(TARGET)

SOURCES = src/main.c src/colisao.c src/jogador.c src/som.c src/menu.c src/barra_itens.c
HEADERS = include/main.h include/colisao.h include/jogador.h include/som.h include/menu.h include/barra_itens.h

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(RAYLIB) -lopengl32 -lgdi32 -lwinmm -lm

clean:
	$(RM) $(TARGET)