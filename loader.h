#ifndef __LOADER_H
#define __LOADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define S_MAX 20

typedef struct {
	unsigned char e_ident[16];/* Magic number and other info */
	unsigned short e_type;		/* Object file type */
  	unsigned short e_machine;	/* Architecture */
  	unsigned int e_version;		/* Object file version */
	unsigned long e_entry;	/* Entry point virtual address */
  	unsigned long e_phoff;	/* Program header table file offset */
	unsigned long e_shoff;	/* Section header table file offset */
	unsigned int e_flags;		/* Processor-specific flags */
	unsigned short e_ehsize;	/* ELF header size in bytes */
  	unsigned short e_phentsize;	/* Program header table entry size */
  	unsigned short e_phnum;		/* Program header table entry count */
	unsigned short e_shentsize;	/* Section header table entry size */
	unsigned short e_shnum;		/* Section header table entry count */
	unsigned short e_shstrndx;	/* Section header string table index */
}Elf64_Ehdr; 

typedef struct { 
	unsigned int p_type;
	unsigned int p_flags;
	unsigned long p_offset;
	unsigned long p_vaddr;
	unsigned long p_paddr;
	unsigned long p_filesz;
	unsigned long p_memsz;
	unsigned long p_align;
}Elf64_Phdr;

typedef struct {
	long vaddr;
	long msize;
	char *paddr;
}Segment;

int load_elf(char *filename, Segment *segments, long *PC);

#endif
