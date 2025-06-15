CC = gcc
CFLAGS = -Wall -g -I.. -lpthread
SRCS = main.c server.c logic.c deck.c player.c utils.c
TARGET = server

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
