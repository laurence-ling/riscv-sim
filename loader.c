#include "loader.h"

int load_elf(char *filename, Segment *segments, long *PC)
{
    Elf64_Ehdr elf_hdr;
    Elf64_Phdr ph_hdr;
    /* open elf file */
    int fd = open(filename, O_RDONLY, 0);
    read(fd, (void *)(&elf_hdr), sizeof(Elf64_Ehdr));
    lseek(fd, elf_hdr.e_phoff, 0);

    *PC = elf_hdr.e_entry; /* set PC to entry point */
    if (elf_hdr.e_phnum >= S_MAX) {
        printf("Program segments number exceeded S_MAX\n");
        return 0;
    }

    /* read program header and allocate memory */
    for (int i = 0; i < elf_hdr.e_phnum; ++i) {
        read(fd, (void *)(&ph_hdr), sizeof(Elf64_Phdr));
        segments[i].vaddr = ph_hdr.p_vaddr;
        segments[i].msize = ph_hdr.p_memsz;
        segments[i].paddr = (char *)calloc(ph_hdr.p_memsz, 1);

        void *ptr = mmap(NULL, ph_hdr.p_filesz, ph_hdr.p_flags, 
                MAP_PRIVATE, fd, ph_hdr.p_offset);
        memcpy((void *)segments[i].paddr, ptr, ph_hdr.p_filesz);
        munmap(ptr, ph_hdr.p_filesz);
        
        /*printf("----load segment %d start with %x\n", i, 
                *(int *)segments[i].paddr);*/
    }
    close(fd);
    return elf_hdr.e_phnum;
}
