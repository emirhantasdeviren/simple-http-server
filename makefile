CC = gcc
CFLAGS = -std=c17 -g -O0 -Wall

debug: src/tcp_listener.c src/tcp_stream.c src/main.c
	$(CC) $(CFLAGS) $^ -o target/http-server

run: debug
	./target/http-server
