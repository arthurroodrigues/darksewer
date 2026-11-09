CC = gcc
TARGET = jogo.exe
RAYLIB = $(shell pkg-config --cflags --libs raylib)
CFLAGS = -O2 -std=c99 -Wall -Wextra

.PHONY: all clean

all: $(TARGET)

$(TARGET): main.c main.h
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(RAYLIB) -lopengl32 -lgdi32 -lwinmm -lm

clean:
	$(RM) $(TARGET)