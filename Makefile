
CC = gcc

CFLAGS = -Wall -Wextra

LIBS = -lSDL2 -lSDL2_ttf

SRC = main.c

TARGET = flashcard

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)


run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
