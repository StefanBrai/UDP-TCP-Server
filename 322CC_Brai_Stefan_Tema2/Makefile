CC=g++
CFLAGS=-std=c++14 -Wall -Wextra

all: server subscriber

server: utilitare.o server.o
	$(CC) $(CFLAGS) $^ -o $@

server.o: server.cpp
	$(CC) $(CFLAGS) -c $< -o $@

utilitare.o: utilitare.cpp
	$(CC) $(CFLAGS) -c $< -o $@

subscriber: subscriber.o utilitare.o
	$(CC) $(CFLAGS) $^ -o $@

subscriber.o: subscriber.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o server subscriber
