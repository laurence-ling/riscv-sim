#include "syscall.h"
#include "loader.h"
#include <sys/time.h>

extern int verbose;
extern Segment segments[S_MAX];
extern int seg_N;
extern char *get_paddr(long vaddr, int read);

long sys_open(char *filename, int flag, int mode)
{
    int r = open(filename, flag, mode);
    if (r == -1)
        printf("open file %s fail\n", filename);
    return (long)r;
}

long sys_close(int fd)
{
    if (fd == 1) {
	if (verbose)
        	printf("atempting closing the stdout\n");
        return 0;
    }
    int r = close(fd);
    if (r == -1)
        printf("close fail\n");
    return (long)r;
}

long sys_read(int fd, char *buf, int n)
{
    fflush(stdout);
    int r = read(fd, buf, n);
    if (r == -1)
        printf("read fail\n");
    return (long)r;
}

long sys_write(int fd, char *buf, int n)
{
    fflush(stdout);
    int r = write(fd, buf, n);
    if (r == -1)
        printf("write fail\n");
    return (long)r;
}

long sys_exit(int code)
{
    return 0;
}

long sys_brk(long addr)
{
    long end_vaddr = segments[seg_N - 2].vaddr + segments[seg_N - 2].msize;
    if (end_vaddr < addr) { /* grow the heap */
        if (verbose)
	    printf("grow the heap\n");
        segments[seg_N] = segments[seg_N - 1];
        int new_size = addr - end_vaddr;
        char *ptr = (char *)calloc(new_size, 1);	
        segments[seg_N - 1].vaddr = end_vaddr + 1;
        segments[seg_N - 1].msize = new_size;
        segments[seg_N - 1].paddr = ptr;
        seg_N += 1;
    }
    else { /* shrink the heap */
        if (verbose)	
            printf("shrink the heap\n");
    }
    return addr;
}

long sys_gettimeofday(long vaddr) {
    struct timeval t;
    gettimeofday(&t, NULL);
  
    long *ptr = (long *)get_paddr(vaddr, 0);
    *ptr = t.tv_sec;
    *(ptr + 1) = t.tv_usec;
    if (verbose)
        printf("gettime: sec = %ld  usec = %ld\n", t.tv_sec, t.tv_usec);
    return 0;
}

long do_syscall(long a0, long a1, long a2, long a3, long a4, 
        long a5, long a6, unsigned long n)
{
    switch (n) {
    case SYS_open:
	if (verbose)
        	printf("sys_open\n");
        return sys_open(get_paddr(a0, 1), (int)a1, (int)a2);
        break;
    case SYS_close:
	if (verbose)
       		printf("sys_close\n");
        return sys_close((int)a0);
        break;
    case SYS_read:
	if (verbose)
        	printf("sys_read\n");
        return sys_read((int)a0, get_paddr(a1, 1), (int)a2);
        break;
    case SYS_write:
	if (verbose)
        	printf("sys_write\n");
        return sys_write((int)a0, get_paddr(a1, 1), (int)a2);
        break;
    case SYS_exit:
	if (verbose)
        	printf("sys_exit\n");
        return sys_exit((int)a0);
        break;
    case SYS_fstat:
	if (verbose)
        	printf("sys_fstat\n");
        break;
    case SYS_brk:
	if (verbose)
        	printf("sys_brk\n");
        return sys_brk(a0);
        break;
    case SYS_lseek:
	if (verbose)
        	printf("sys_lseek\n");
        break;
    case SYS_gettimeofday:
        if (verbose)
		printf("sys_gettimeofday\n");
	sys_gettimeofday(a0);
        break;	
    default:
        printf("Unsupported syscall %ld!\n", n);
    }
    return 0;
}
