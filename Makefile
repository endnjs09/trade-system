CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude -pthread
SERVER_SRC = src/server/server.c \
             src/server/client_manager.c \
             src/server/handler.c \
             src/server/market.c \
             src/server/orderbook.c \
             src/server/multicast.c
CLIENT_SRC = src/client/client.c

SERVER_OUT = server
CLIENT_OUT = client

all: $(SERVER_OUT) $(CLIENT_OUT)

$(SERVER_OUT): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_OUT) $(SERVER_SRC)

$(CLIENT_OUT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_OUT) $(CLIENT_SRC)

clean:
	rm -f $(SERVER_OUT) $(CLIENT_OUT)