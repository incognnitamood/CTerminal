CC=gcc
CFLAGS=-Wall -Wextra
SRC=backend/main.c backend/utils.c backend/filesystem.c backend/history.c \
    backend/stack.c backend/hashmap.c backend/parser.c backend/trie.c \
    backend/logger.c backend/commands.c

all: terminal

terminal: $(SRC)
	$(CC) $(CFLAGS) -o terminal $(SRC)

clean:
	rm -f terminal

