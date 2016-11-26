CC = g++
FLAGS = -o2
PROC = sim

all: $(PROC)

sim: sim.c syscall.h syscall.c loader.h loader.c decoder.h \
	./cache/cache.o ./cache/memory.o
	make -C ./cache
	$(CC) $(FLAGS) -o $@ $^


clean:
	-rm -f $(PROC) *.o
	make -C ./cache clean
