CC = riscv64-unknown-elf-gcc
DUMP = riscv64-unknown-elf-objdump
PROC = int1 hello float ackerman qsort matrix
FLAGS = -static -std=c99

all: $(PROC)

int1: int1.c
	$(CC) $(FLAGS) $^ -o $@
	$(DUMP) -d $@ > int1.txt

hello: hello.c
	$(CC) $(FLAGS) $^ -o $@ 
	$(DUMP) -d $@ > hello.txt

float: float.c
	$(CC) $(FLAGS) $^ -o $@
	$(DUMP) -d $@ > float.txt

ackerman: Ackermann.c
	$(CC) $(FLAGS) $^ -o $@

qsort: quickSort.c
	$(CC) $(FLAGS) $^ -o $@

matrix: matrix.c
	$(CC) $(FLAGS) $^ -o $@

clean:
	-rm $(PROC) *.o *.s *.i -f
