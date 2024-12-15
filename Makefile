CC=cc 
CFLAGS=-Wall -Wextra  -Wpedantic -std=c99 -ggdb
CLIBS=-lm -lraylib -lpthread -ldl -lX11

all: 
	$(CC) $(CFLAGS) -o main main.c $(CLIBS)
