CC = gcc
CFLAGS = -I./include -Wall
LDFLAGS = 

SOURCES = main.c songs.c albums.c utils.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = music_player

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run