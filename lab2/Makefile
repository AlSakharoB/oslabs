# Makefile

CC      = gcc
CFLAGS  = -std=gnu11
LDFLAGS =

TARGETS = server client1 client2

all: $(TARGETS)

server: server.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

client1: client1.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

client2: client2.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# по требованию задания – общий таргет "client"
client: client1 client2

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test:
	sh ./run_tests.sh

clean:
	rm -f $(TARGETS) *.o *_out.txt

