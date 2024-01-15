all: client server

client: client.c
	gcc $^ -o $@

server: server.c
	gcc $^ -o $@ -lsodium -lpthread

clean:
	rm -r server client