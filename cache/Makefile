CC=g++
CFLAGS=

all: sim

sim: main.o cache.o memory.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: cache.h

cache.o: cache.h def.h cache.cc
	$(CC) $(CFLAGS) -c $^

memory.o: memory.h

.PHONY: clean

clean:
	rm -rf sim *.o
