CC = gcc
CFLAGS = -Wall -I./headers -pthread

SERVER_SRCS = src/main.c src/resolver.c src/auth.c src/cache.c src/records.c src/logger.c src/upstream.c
CLIENT_SRCS = src/client.c

all: server client

server: $(SERVER_SRCS)
	$(CC) $(CFLAGS) $(SERVER_SRCS) -o server

client: $(CLIENT_SRCS)
	$(CC) $(CFLAGS) $(CLIENT_SRCS) -o client

clean:
	rm -f server client
