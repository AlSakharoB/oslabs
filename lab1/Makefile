# Makefile

CC      = gcc
CFLAGS  = -std=gnu11
LDFLAGS =

TARGETS = server client

all: $(TARGETS)

server: server.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

client: client.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test:
	sh ./run_tests.sh

clean:
	rm -f $(TARGETS) *.o
	rm -rf testdir_*