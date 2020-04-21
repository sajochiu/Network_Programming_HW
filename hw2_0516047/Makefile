CC := g++
CPPFLAGS := -stdc++=17 -ggdb -fPIC -Wall -Wno-unused-variable -Wno-write-strings

all: server

server:
	$(CC) -L/usr/include $(CPP_FLAGS) server.cpp -o server -lsqlite3

run:
	./server 7777

clean:
	rm -rf server
