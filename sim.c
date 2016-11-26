#include "loader.h"
#include "decoder.h"
#include "syscall.h"
#include "cache/cache.h"
#include "cache/memory.h"
#include <math.h>

long PC;
unsigned int instr; /* instruction */

long RegFile[32]; /* integer register file */
float FP_Reg[32]; /* floating point register file */
double DP_Reg[32]; /* double precision floating point */

Segment segments[S_MAX];	/*memory segments */
int seg_N;  /* segments number */

Cache cache, L2, LLC; /* cache simulator */
Memory m;
int memory_cycle; /* cycles spend on memory*/

Instr_Type type ; /* instruction type from decoding */
Data_Path dpath; /* contain all signals from decoding */

long breakpoint;
int verbose; /* verbose mode flag */
int single;

void set_stack(void);
void set_cache(void);
void fetch(void);
void execute(void);
void sign_extend(void);

char *get_paddr(long vaddr, int read);
void finish(void);
int wait_command_line(void);

void print_regfile(void);
void print_fpreg(void);
void print_dpreg(void);
void print_datapath(void);

int main(int argc, char *argv[])
{
	if (argc < 2 || argc > 3) {
		printf("Usage: <file_name>[--verbose|--debug]\n");
		exit(1);
	}

	seg_N = load_elf(argv[1], segments, &PC);
	if (seg_N == 0)
		exit(1);
	set_stack();
	set_cache();

	if (argc == 3) {
		if (strcmp(argv[2], "--debug") == 0){
			printf("Debug mode\n");
			breakpoint = PC;
		}
		if (strcmp(argv[2], "--verbose") == 0)
			verbose = 1;
	}
	int i;
	for (i = 0; ; ++i) {
		if (breakpoint == PC || single) {
			printf("breakpoint at 0x%lx\n", PC);
			single = 0;
			int quit = wait_command_line();
			if (quit != 0) {
				printf("quit...\n");
				break;
			}
		}
		if (verbose)
			printf("\nThe %d round...\n", i);
		
		long lastPC = PC;
		fetch();
		type = decode(instr, &dpath);
		execute();

		if (PC == lastPC){
            if (verbose)
                printf("----PC donnot change, exit...\n");
			break;
		}
		if (verbose)
			print_regfile();
		
	}
	finish();
    	/* print cache miss rate */
	cache.PrintMissRate();
	printf("Instruction count: %d, memory cycle: %d\n", i, memory_cycle);
	return 0;
}

int wait_command_line()
{
	char cmd[10];
	long addr;
	char *paddr;
	int flag;
	while (1) {
		flag = 1;
		printf(">");
		scanf("%s", cmd);
		fflush(stdin); /* eat '\n' */
		switch (cmd[0]) {
		case 'b':
			scanf("%lx", &addr);
			breakpoint = addr;
			break;
		case 'c':
			break;
		case 's':
			single = 1;
			break;
        case 'n':
            breakpoint = PC + 4;
            break;
		case 'p':
			print_regfile();
			flag = 0;
			break;
		case 'f':
			print_fpreg();
	    		flag = 0;
			break;
        case 'd':
            print_dpreg();
                flag = 0;
            break;
		case 'x':
			scanf("%lx", &addr);
			paddr = get_paddr(addr, 1);
			printf("%x    %x\n", *(int *)paddr, *(int *)(paddr + 4));
			flag = 0;
			break;
        case 'v':
            flag = 0;
            if (verbose) 
                verbose = 0;
            else
                verbose = 1;
            break;
		case 'q':
			return 1;
		default:
			flag = 0;
			printf("Undefined command\n");
		}
		if(flag != 0)
			break;
	}
	return 0;
}

void set_stack()
{
    RegFile[2] = 0x7fff0000; /*stack pointer sp */
    int stack_size = 0xf0000;
    segments[seg_N].msize = stack_size;

    /*stack segment start at low address, sp at the top*/
    segments[seg_N].vaddr = RegFile[2] - stack_size;
    segments[seg_N].paddr = (char *)calloc(stack_size, 1);
    if(verbose)
        printf("set stack bottom addr %lx\n", (long)segments[seg_N].paddr);
    seg_N += 1;
}

void finish()
{
    if (verbose)
        printf("-----Clearing memory segments\n");
    /* free segments pointer */
    for (int i = 0; i < seg_N; ++i) {
        free(segments[i].paddr);
    }  
}

char *get_paddr(long vaddr, int read)
{
    int hit, time;
    cache.HandleRequest(vaddr, 0, read, NULL, hit, time);
    memory_cycle += time;
    for (int i = 0; i < seg_N; ++i) {
        long saddr = segments[i].vaddr;
        if (vaddr >= saddr && vaddr <= saddr + segments[i].msize) {
            return segments[i].paddr + vaddr - saddr;
        }
    }
    printf("Cannot get virtual address %lx!  PC = %lx\n", vaddr, PC);
    finish();
    exit(1);
}

void fetch()
{
    char *paddr = get_paddr(PC, 1);
    instr = *(unsigned int *)paddr;
    if (verbose) {
        printf("PC = 0x%lx    ", PC);
        printf("instruction = 0x%x\n", instr);    
    }
    PC += 4; /* PC already plus 4 */
}

void sign_extend()
{
    if((instr >> 31) != 0) {
       dpath.imm_I |= (-1 << 12);
       dpath.imm_S |= (-1 << 12);
       dpath.imm_SB |= (-1 << 13);
       dpath.imm_UJ |= (-1 << 21);
    }
}

void print_regfile()
{
    printf("--------All register files--------\n");
    for (int i = 0; i < 32; ++i) {
        if (i % 4 == 0 && i != 0)
            printf("\n");
        printf("%lx    ", RegFile[i]);
    }
    printf("\nra(x1): %lx   sp(x2): %lx   gp(x3): %lx   s0(x8): %lx\n\n", 
        RegFile[1], RegFile[2], RegFile[3], RegFile[8]);
}

void print_fpreg(void)
{
    printf("--------FP register files--------\n");
    for (int i = 0; i < 32; ++i) {
        if (i % 4 == 0 && i != 0)
            printf("\n");
        printf("%f    ", FP_Reg[i]);
    }
    printf("\n");
}

void print_dpreg(void)
{
    printf("--------DP register files--------\n");
    for (int i = 0; i < 32; ++i) {
        if (i % 4 == 0 && i != 0)
            printf("\n");
        printf("%f    ", DP_Reg[i]);
    }
    printf("\n");
}

void print_datapath()
{
    printf("----------Signals from decoding-------\n");
    printf("rs1 = %x  rs2 = %x  rd = %x\n", dpath.rs1, dpath.rs2, dpath.rd);
    printf("shamt = 0x%x\n", dpath.shamt);
    printf("imm_I = 0x%x, %d\n", dpath.imm_I, dpath.imm_I);
    printf("imm_S = 0x%x, %d\n", dpath.imm_S, dpath.imm_S);
    printf("imm_SB = 0x%x, %d\n", dpath.imm_SB, dpath.imm_SB);
    printf("imm_U = 0x%x, %d\n", dpath.imm_U, dpath.imm_U);
    printf("imm_UJ = 0x%x, %d\n\n", dpath.imm_UJ, dpath.imm_UJ);
}

void execute()
{
    sign_extend();
    switch(type) {
    case UNDEFINED:
        printf("Undefined intruction PC = %lx\n", PC);
        break;

    case ECALL:
        if (verbose)
            printf("ecall\n\n");
        RegFile[10] = do_syscall(RegFile[10], RegFile[11], RegFile[12], 
        RegFile[13], RegFile[14], RegFile[15], RegFile[16], RegFile[17]);
	break;

    case AUIPC:
        if (verbose)
            printf("auipc x%d, %x\n\n", dpath.rd, dpath.imm_U);
        RegFile[dpath.rd] = PC - 4 + dpath.imm_U;
        break;

    case LUI:
        if (verbose)
            printf("lui x%d, %x\n\n", dpath.rd, dpath.imm_U);
        RegFile[dpath.rd] = (long)dpath.imm_U;
        break;

    /* OP_IMM */
    case ADDI:
        if (verbose)
            printf("addi x%d, x%d, %x\n\n", dpath.rd, dpath.rs1, dpath.imm_I);
        RegFile[dpath.rd] = RegFile[dpath.rs1] + dpath.imm_I;
        break;
    case ANDI:
        if (verbose)
            printf("andi x%d, x%d, %x\n\n", dpath.rd, dpath.rs1, dpath.imm_I);
        RegFile[dpath.rd] = RegFile[dpath.rs1] & dpath.imm_I;
        break;   
    case ORI:
        if (verbose)
            printf("ori x%d, x%d, %x\n\n", dpath.rd, dpath.rs1, dpath.imm_I);
        RegFile[dpath.rd] = RegFile[dpath.rs1] | dpath.imm_I;
        break; 
    case XORI:
        if (verbose)
            printf("xori x%d, x%d, %x\n\n", dpath.rd, dpath.rs1, dpath.imm_I);
        RegFile[dpath.rd] = RegFile[dpath.rs1] ^ dpath.imm_I;
        break; 
    case SLTI:
        if (verbose)
            printf("slti x%d, x%d, %x\n\n", dpath.rd, dpath.rs1, dpath.imm_I);
        if (RegFile[dpath.rs1] < (long)dpath.imm_I)
            RegFile[dpath.rd] = 1;
        else
            RegFile[dpath.rd] = 0;
        break; 
    case SLTIU:
        if (verbose)
            printf("sltiu x%d, x%d, %x\n\n", dpath.rd, dpath.rs1, dpath.imm_I);
        if ((unsigned long)RegFile[dpath.rs1] < (unsigned long)(long)dpath.imm_I)
            RegFile[dpath.rd] = 1;
        else
            RegFile[dpath.rd] = 0;
        break;
    case SLLI:
        if (verbose)
            printf("slli x%d, x%d, %x\n\n", dpath.rd, dpath.rs1, dpath.imm_I);
        RegFile[dpath.rd] = RegFile[dpath.rs1] << dpath.shamt;
        break;
    case SRAI:
        if (verbose)
            printf("srai x%d, x%d, %x\n\n", dpath.rd, dpath.rs1, dpath.imm_I);
        RegFile[dpath.rd] = RegFile[dpath.rs1] >> dpath.shamt;
        break;
    case SRLI:
        if (verbose)
            printf("srli x%d, x%d, %x\n\n", dpath.rd, dpath.rs1, dpath.imm_I);
        RegFile[dpath.rd] = (unsigned long)RegFile[dpath.rs1] >> dpath.shamt;
        break;

    /* OP_IMM_32 */
    case ADDIW:
        if (verbose)
            printf("addiw x%d, x%d, %x\n\n", dpath.rd, dpath.rs1, dpath.imm_I);
        RegFile[dpath.rd] = ((int)RegFile[dpath.rs1] + dpath.imm_I);
        break;
    case SLLIW:
        if (verbose)
            printf("slliw x%d, x%d, %x\n\n", dpath.rd, dpath.rs1, dpath.imm_I);
        RegFile[dpath.rd] = ((int)RegFile[dpath.rs1] << dpath.shamt);
        break;
    case SRAIW:
        if (verbose)
            printf("sraiw x%d, x%d, %x\n\n", dpath.rd, dpath.rs1, dpath.imm_I);
        RegFile[dpath.rd] = ((int)RegFile[dpath.rs1] >> dpath.shamt);
        break;
    case SRLIW:
        if (verbose)
            printf("srliw x%d, x%d, %x\n\n", dpath.rd, dpath.rs1, dpath.imm_I);
        RegFile[dpath.rd] = ((unsigned int)RegFile[dpath.rs1] >> dpath.shamt);
        break;

    /* OP */
    case ADD:
        if (verbose)
            printf("add x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = RegFile[dpath.rs1] + RegFile[dpath.rs2];
        break;
    case SUB:
        if (verbose)
            printf("sub x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = RegFile[dpath.rs1] - RegFile[dpath.rs2];
        break;
    case AND:
        if (verbose)
            printf("and x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = RegFile[dpath.rs1] & RegFile[dpath.rs2];
        break;
    case OR:
        if (verbose)
            printf("or x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = RegFile[dpath.rs1] | RegFile[dpath.rs2];
        break;
    case XOR:
        if (verbose)
            printf("xor x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = RegFile[dpath.rs1] ^ RegFile[dpath.rs2];
        break;
    case SLT:
        if (verbose)
            printf("slt x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        if (RegFile[dpath.rs1] < RegFile[dpath.rs2])
            RegFile[dpath.rd] = 1;
        else
            RegFile[dpath.rd] = 0;
        break;
     case SLTU:
        if (verbose)
            printf("sltu x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        if ((unsigned long)RegFile[dpath.rs1] < (unsigned long)RegFile[dpath.rs2])
            RegFile[dpath.rd] = 1;
        else
            RegFile[dpath.rd] = 0;
        break;
    case SLL:
        {
            if (verbose)
                printf("sll x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
            int shamt = RegFile[dpath.rs2] & 0x3f;
            RegFile[dpath.rd] = RegFile[dpath.rs1] << shamt;
        }
        break;
    case SRA:
        {
            if (verbose)
                printf("sra x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
            int shamt = RegFile[dpath.rs2] & 0x3f;
            RegFile[dpath.rd] = RegFile[dpath.rs1] >> shamt;
        }
        break;
    case SRL:
        {
            if (verbose)
                printf("srl x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
            int shamt = RegFile[dpath.rs2] & 0x3f;
            RegFile[dpath.rd] = (unsigned long)RegFile[dpath.rs1] >> shamt;
        }
        break;
    /* M-extension */
    case MUL:
        if (verbose)
            printf("mul x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = RegFile[dpath.rs1] * RegFile[dpath.rs2];
        break;
    case MULH:
        {
            if (verbose)
                printf("mulh x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
            long a_lo = (int)RegFile[dpath.rs1];
            long a_hi = RegFile[dpath.rs1] >> 32;
            long b_lo = (int)RegFile[dpath.rs2];
            long b_hi = RegFile[dpath.rs2] >> 32;
            long mul_hi = a_hi * b_hi;
            long mul_mid = a_hi * b_lo + a_lo * b_hi;
            
            RegFile[dpath.rd] = mul_hi + (mul_mid >> 32);
        }
        break;
    case DIV:
        if (verbose)
            printf("div x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = RegFile[dpath.rs1] / RegFile[dpath.rs2];
        break;
    case DIVU:
        if (verbose)
            printf("divu x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = (unsigned long)RegFile[dpath.rs1] 
                        / (unsigned long)RegFile[dpath.rs2];
        break;
    case REM:
        if (verbose)
            printf("rem x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = RegFile[dpath.rs1] % RegFile[dpath.rs2];
        break;
    case REMU:
        if (verbose)
            printf("remu x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = (unsigned long)RegFile[dpath.rs1] 
                        % (unsigned long)RegFile[dpath.rs2];
        break;

    /* OP_32 */
    case ADDW:
        if (verbose)
            printf("addw x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = ((int)RegFile[dpath.rs1] + (int)RegFile[dpath.rs2]);
        break;
    case SUBW:
        if (verbose)
            printf("subw x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = ((int)RegFile[dpath.rs1] - (int)RegFile[dpath.rs2]);
        break;
    case SLLW:
        {
            if (verbose)
                printf("sllw x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
            int shamt = RegFile[dpath.rs2] & 0x1f;
            RegFile[dpath.rd] = (((int)RegFile[dpath.rs1]) << shamt);
        }
        break;
    case SRAW:
        {
            if (verbose)
                printf("sraw x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
            int shamt = RegFile[dpath.rs2] & 0x1f;
            RegFile[dpath.rd] = (((int)RegFile[dpath.rs1]) >> shamt);
        }
        break;
    case SRLW:
        {
            if (verbose)
                printf("srlw x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
            int shamt = RegFile[dpath.rs2] & 0x1f;
            RegFile[dpath.rd] = (unsigned long)((unsigned int)RegFile[dpath.rs1] >> shamt);
        }
        break;
    /* M-extension */
    case MULW:
        if (verbose)
            printf("mulw x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = (int)RegFile[dpath.rs1] * (int)RegFile[dpath.rs2];
        break;
    case DIVW:
        if (verbose)
            printf("divw x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = (int)RegFile[dpath.rs1] / (int)RegFile[dpath.rs2];
        break;
    case DIVUW:
        if (verbose)
            printf("divuw x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = (unsigned int)RegFile[dpath.rs1] 
                            / (unsigned int)RegFile[dpath.rs2];
        break;
    case REMW:
        if (verbose)
        printf("remw x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = (int)RegFile[dpath.rs1] % (int)RegFile[dpath.rs2];
        break;
    case REMUW:
        if (verbose)
            printf("remuw x%d, x%d, x%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        RegFile[dpath.rd] = (unsigned int)RegFile[dpath.rs1] 
                            % (unsigned int)RegFile[dpath.rs2];
        break;


    /* STORE */
    case SD: 
        {
            if (verbose)
                printf("sd x%d, %d(x%d)\n\n", dpath.rs2, dpath.imm_S, dpath.rs1);
            long addr = RegFile[dpath.rs1] + dpath.imm_S; 
            char *ptr = get_paddr(addr, 0);
            *(long *)ptr = RegFile[dpath.rs2];
        }
        break;
    case SW:
        {
            if (verbose)
                printf("sw x%d, %d(x%d)\n\n", dpath.rs2, dpath.imm_S, dpath.rs1);
            long addr = RegFile[dpath.rs1] + dpath.imm_S; 
            char *ptr = get_paddr(addr, 0);
            *(int *)ptr = (int)RegFile[dpath.rs2];
        }
        break;
    case SH:
        {
            if (verbose)
                printf("sh x%d, %d(x%d)\n\n", dpath.rs2, dpath.imm_S, dpath.rs1);
            long addr = RegFile[dpath.rs1] + dpath.imm_S; 
            char *ptr = get_paddr(addr, 0);
            *(short *)ptr = (short)RegFile[dpath.rs2];
        }
        break;
    case SB:
        {
            if (verbose)
                printf("sb x%d, %d(x%d)\n\n", dpath.rs2, dpath.imm_S, dpath.rs1);
            long addr = RegFile[dpath.rs1] + dpath.imm_S; 
            char *ptr = get_paddr(addr, 0);
            *ptr = (char)RegFile[dpath.rs2];
        }
        break;

    /* LOAD */
    case LD:
        {
            if (verbose)
                printf("ld x%d, %d(x%d)\n\n", dpath.rd, dpath.imm_I, dpath.rs1);
            long addr = RegFile[dpath.rs1] + dpath.imm_I; 
            char *ptr = get_paddr(addr, 1);
            RegFile[dpath.rd] = *(long *)ptr;
        }
        break;
    case LW:
        {
            if (verbose)
                printf("lw x%d, %d(x%d)\n\n", dpath.rd, dpath.imm_I, dpath.rs1);
            long addr = RegFile[dpath.rs1] + dpath.imm_I; 
            char *ptr = get_paddr(addr, 1);
            RegFile[dpath.rd] = *(int *)ptr;
        }
        break;
    case LH:
        {
            if (verbose)
                printf("lh x%d, %d(x%d)\n\n", dpath.rd, dpath.imm_I, dpath.rs1);
            long addr = RegFile[dpath.rs1] + dpath.imm_I; 
            char *ptr = get_paddr(addr, 1);
            RegFile[dpath.rd] = *(short *)ptr;
        }
        break;
    case LB:
        {
            if (verbose)
                printf("lb x%d, %d(x%d)\n\n", dpath.rd, dpath.imm_I, dpath.rs1);
            long addr = RegFile[dpath.rs1] + dpath.imm_I; 
            char *ptr = get_paddr(addr, 1);
            RegFile[dpath.rd] = *ptr;
        }
        break;
    case LWU:
        {
            if (verbose)
                printf("lwu x%d, %d(x%d)\n\n", dpath.rd, dpath.imm_I, dpath.rs1);
            long addr = RegFile[dpath.rs1] + dpath.imm_I; 
            char *ptr = get_paddr(addr, 1);
            RegFile[dpath.rd] = (unsigned long)*(unsigned int *)ptr;
        }
        break;
    case LHU:
        {
            if (verbose)
                printf("lhu x%d, %d(x%d)\n\n", dpath.rd, dpath.imm_I, dpath.rs1);
            long addr = RegFile[dpath.rs1] + dpath.imm_I; 
            char *ptr = get_paddr(addr, 1);
            RegFile[dpath.rd] = (unsigned long)*(unsigned short *)ptr;
        }
        break;
    case LBU:
        {
            if (verbose)
                printf("lbu x%d, %d(x%d)\n\n", dpath.rd, dpath.imm_I, dpath.rs1);
            long addr = RegFile[dpath.rs1] + dpath.imm_I; 
            char *ptr = get_paddr(addr, 1);
            RegFile[dpath.rd] = (unsigned long)*(unsigned char *)ptr;
        }
        break;

    /* BRANCH */
    case BEQ:
        {
            long target = PC - 4 + dpath.imm_SB;
            if (verbose)
                printf("beq x%d, x%d, %x <0x%lx>\n\n", dpath.rs1, dpath.rs2, dpath.imm_SB, target);
            if (RegFile[dpath.rs1] == RegFile[dpath.rs2])
                PC = target;
        }
        break;
    case BNE:
        {
            long target = PC - 4 + dpath.imm_SB;
            if (verbose)
                printf("bne x%d, x%d, %x <0x%lx>\n\n", dpath.rs1, dpath.rs2, dpath.imm_SB, target);
            if (RegFile[dpath.rs1] != RegFile[dpath.rs2])
                PC = target;
        }
        break;
    case BLT:
        {
            long target = PC - 4 + dpath.imm_SB;
            if (verbose)
                printf("blt x%d, x%d, %x <0x%lx>\n\n", dpath.rs1, dpath.rs2, dpath.imm_SB, target);
            if (RegFile[dpath.rs1] < RegFile[dpath.rs2])
                PC = target;
        }
        break;
    case BLTU:
        {
            long target = PC - 4 + dpath.imm_SB;
            if (verbose)
                printf("bltu x%d, x%d, %x <0x%lx>\n\n", dpath.rs1, dpath.rs2, dpath.imm_SB, target);
            if ((unsigned long)RegFile[dpath.rs1] < (unsigned long)RegFile[dpath.rs2])
                PC = target;
        }
        break;
    case BGE:
        {
            long target = PC - 4 + dpath.imm_SB;
            if (verbose)
                printf("bge x%d, x%d, %x <0x%lx>\n\n", dpath.rs1, dpath.rs2, dpath.imm_SB, target);
            if (RegFile[dpath.rs1] >= RegFile[dpath.rs2])
                PC = target;
        }
        break;
    case BGEU:
        {
            long target = PC - 4 + dpath.imm_SB;
            if (verbose)
                printf("bgeu x%d, x%d, %x <0x%lx>\n\n", dpath.rs1, dpath.rs2, dpath.imm_SB, target);
            if ((unsigned long)RegFile[dpath.rs1] >= (unsigned long)RegFile[dpath.rs2])
                PC = target;
        }
        break;

    case JAL:
        {
            long target = PC - 4 + dpath.imm_UJ;
            if (verbose)
                printf("jal x%d, %x <0x%lx>\n\n", dpath.rd, dpath.imm_UJ, target);
            if (dpath.rd != 0) 
                RegFile[dpath.rd] = PC; /* store PC */
            PC = target;
        }
        break;

    case JALR:
        {
            long target = RegFile[dpath.rs1] + dpath.imm_I;
            target = target >> 1 << 1; /* set the lowest bit to 0 */
            if (verbose)
                printf("jalr x%d, %x(x%d) <0x%lx>\n\n", dpath.rd, dpath.imm_I, dpath.rs1, target);
            if (dpath.rd != 0)
                RegFile[dpath.rd] = PC; /* store PC */
            PC = target;
        }
        break;

    /* F-extension */
    case FLW:
        {
            long addr = RegFile[dpath.rs1] + dpath.imm_I;
            if (verbose)
                printf("flw f%d, %d(x%d)\n\n", dpath.rd, dpath.imm_I, dpath.rs1);
            char *ptr = get_paddr(addr, 1);
            FP_Reg[dpath.rd] = *(float *)ptr;
        }
        break;
    case FSW:
        {
            long addr = RegFile[dpath.rs1] + dpath.imm_S;
            if (verbose)
                printf("fsw f%d, %d(x%d)\n\n", dpath.rs2, dpath.imm_I, dpath.rs1);
            char *ptr = get_paddr(addr, 0);
            *(float *)ptr = FP_Reg[dpath.rs2];
        }
        break;
    case FADD_S:
        if (verbose)
            printf("fadd.s f%d, f%d, f%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        FP_Reg[dpath.rd] = FP_Reg[dpath.rs1] + FP_Reg[dpath.rs2];
        break;
    case FSUB_S:
        if (verbose)
            printf("fsub.s f%d, f%d, f%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        FP_Reg[dpath.rd] = FP_Reg[dpath.rs1] - FP_Reg[dpath.rs2];
        break;
    case FMUL_S:
        if (verbose)
            printf("fmul.s f%d, f%d, f%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        FP_Reg[dpath.rd] = FP_Reg[dpath.rs1] * FP_Reg[dpath.rs2];
        break;
    case FDIV_S:
        if (verbose)
            printf("fdiv.s f%d, f%d, f%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        FP_Reg[dpath.rd] = FP_Reg[dpath.rs1] / FP_Reg[dpath.rs2];
        break;
    case FEQ_S:
        if (verbose)
            printf("feq.s f%d, f%d, f%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        if (FP_Reg[dpath.rs1] == FP_Reg[dpath.rs2])
            RegFile[dpath.rd] = 1;
        else
            RegFile[dpath.rd] = 0;
        break;
    case FLT_S:
        if (verbose)
            printf("flt.s f%d, f%d, f%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        if (FP_Reg[dpath.rs1] < FP_Reg[dpath.rs2])
            RegFile[dpath.rd] = 1;
        else
            RegFile[dpath.rd] = 0;
        break;
    case FLE_S:
        if (verbose)
            printf("fle.s f%d, f%d, f%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        if (FP_Reg[dpath.rs1] <= FP_Reg[dpath.rs2])
            RegFile[dpath.rd] = 1;
        else
            RegFile[dpath.rd] = 0;
        break;

    /* F-extension for dhry */
    case FCVT_S_W:          // 32-bit signed integer to float
        if(verbose)
            printf("fcvt.s.w f%d, x%d\n\n",dpath.rd,dpath.rs1);
        FP_Reg[dpath.rd] = (float)(RegFile[dpath.rs1] & 0x0ffffffff);
        break;
    case FCVT_S_L:          // 64-bit signed integer to double
        if(verbose)
            printf("fcvt.s.l f%d, x%d\n\n", dpath.rd,dpath.rs1);
        FP_Reg[dpath.rd] = (float)RegFile[dpath.rs1];
        break;
    case FCVT_W_S:
        if(verbose)
            printf("fcvt.w.s x%d, f%d\n\n", dpath.rd, dpath.rs1);
        RegFile[dpath.rd] = (int)FP_Reg[dpath.rs1];
        break;
    case FCVT_L_S:
        if(verbose)
            printf("fcvt.l.s x%d, f%d\n\n", dpath.rd, dpath.rs1);
        RegFile[dpath.rd] = (long)FP_Reg[dpath.rs1];
        break;
    case FCVT_D_S:          // float to double
        if(verbose)
            printf("fcvt.d.s f%d, f%d\n\n",dpath.rd,dpath.rs1);
        DP_Reg[dpath.rd] = (double)FP_Reg[dpath.rs1];
        break;


    /* D-extension for dhry*/
    case FMUL_D:            // double *
        if(verbose)
            printf("fmul.d f%d, f%d, f%d\n\n", dpath.rd,dpath.rs1,dpath.rs2);
        DP_Reg[dpath.rd] = DP_Reg[dpath.rs1] * DP_Reg[dpath.rs2];
        break;
    case FDIV_D:            // double /
        if(verbose)
            printf("fdiv.d f%d, f%d, f%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        DP_Reg[dpath.rd] = DP_Reg[dpath.rs1] / DP_Reg[dpath.rs2];
        break;
    case FCVT_S_D:          // double -> float
        if(verbose)
            printf("fcvt.s.d f%d, f%d\n\n",dpath.rd,dpath.rs1);
        FP_Reg[dpath.rd] = (float)DP_Reg[dpath.rs1];
        break;
    case FMV_X_D:           // double to integer & keep binary code not change
        if(verbose)
            printf("fmv.x.d x%d, f%d\n\n", dpath.rd, dpath.rs1);
        RegFile[dpath.rd] = *(long *)(&DP_Reg[dpath.rs1]);
        break;
    case FMV_D_X:           // integer to double & binary code not change
        if(verbose)
            printf("fmv.d.x f%d, x%d\n\n",dpath.rd, dpath.rs1);
        DP_Reg[dpath.rd] = *(double*)(&RegFile[dpath.rs1]);
        break;
    case FADD_D:            // double +
        if(verbose)
            printf("fadd.d f%d, f%d,f%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
        DP_Reg[dpath.rd] = DP_Reg[dpath.rs1] + DP_Reg[dpath.rs2];
        break;
    case FSUB_D:            // double -
        if (verbose)
            printf("fsub.d f%d, f%d, f%d\n\n",dpath.rd,dpath.rs1,dpath.rs2 );
        DP_Reg[dpath.rd] = DP_Reg[dpath.rs1] - DP_Reg[dpath.rs2];
        break;
    case FLD:               // load double from memory
        {
            long addr = RegFile[dpath.rs1] + dpath.imm_I;
            if(verbose)
                printf("fld f%d, %d(x%d)\n\n", dpath.rd, dpath.imm_I, dpath.rs1);
            char *ptr = get_paddr(addr, 1);
            DP_Reg[dpath.rd] = *(double *)ptr;
            break;
        }
    case FSD:               // store double to memory
        {
            long addr = RegFile[dpath.rs1] + dpath.imm_S;
            if(verbose)
                printf("fsd f%d, %d(x%d)\n\n",dpath.rs2,dpath.imm_I,dpath.rs1);
            char *ptr = get_paddr(addr, 0);
            *(double *)ptr = DP_Reg[dpath.rs2];
            break;
        }
    case FEQ_D:             // f.rs1 == f.rs2 ,x.rd = 1
        if(verbose)
            printf("feq.d x%d, f%d, f%d\n\n",dpath.rd,dpath.rs1,dpath.rs2 );
        if(DP_Reg[dpath.rs1] == DP_Reg[dpath.rs2])
            RegFile[dpath.rd] = 1;
        else
            RegFile[dpath.rd] = 0;
        break;
    case FLT_D:             // f.rs1 < f.rs2
        if(verbose)
            printf("flt.d x%d, f%d, f%d\n\n",dpath.rd, dpath.rs1, dpath.rs2 );
        if(DP_Reg[dpath.rs1] < DP_Reg[dpath.rs2])
            RegFile[dpath.rd] = 1;
        else
            RegFile[dpath.rd] = 0;
        break;
    case FLE_D:             // f.rs1 <= f.rs2
        if(verbose)
            printf("fle.d x%d, f%d, f%d\n\n",dpath.rd, dpath.rs1,dpath.rs2 );
        if(DP_Reg[dpath.rs1] <= DP_Reg[dpath.rs2])
            RegFile[dpath.rd] = 1;
        else
            RegFile[dpath.rd] = 0;
        break;
    case FCVT_D_W:          // 32-bit signed int -> double (Here : low 32 bit)
        {
            if(verbose)
                printf("fcvt.d.w f%d, x%d\n\n",dpath.rd, dpath.rs1 );
            DP_Reg[dpath.rd] = (double)(RegFile[dpath.rs1] & 0x0ffffffff);
        }
        break;
    case FCVT_D_WU:         // 32 bit unsigned int -> double (Here : low 32bit)
        {
            if(verbose)
                printf("fcvt.d.wu f%d, x%d\n\n",dpath.rd, dpath.rs1 );
            unsigned int res = (unsigned int)(RegFile[dpath.rs1] & 0x0ffffffff);
            DP_Reg[dpath.rd] = (double)res;
        }
        break;
    case FCVT_W_D:          // double -> int (int->long:sign extend)
        if(verbose)
            printf("fcvt.w.d x%d, f%d\n\n",dpath.rd,dpath.rs1);
        RegFile[dpath.rd] = (int)DP_Reg[dpath.rs1];
        break;
    case FCVT_WU_D:         // double -> unsigned int
        if(verbose)
            printf("fcvt.wu.d x%d, f%d\n\n",dpath.rd,dpath.rs1 );
        RegFile[dpath.rd] = (unsigned long)(unsigned int)DP_Reg[dpath.rs1];
        break; 
    case FMADD_D:           // rd = rs1 * rs2 + rs3
        {
            int rs3 = dpath.funct7 >> 2;
            if(verbose)
                printf("fmadd.d f%d, f%d, f%d, f%d\n\n",dpath.rd, dpath.rs1, dpath.rs2, rs3);
            DP_Reg[dpath.rd] = DP_Reg[dpath.rs1] * DP_Reg[dpath.rs2] + DP_Reg[rs3];
        }
        break;
    case FMV_D:             // rd = rs
        if(verbose)
            printf("fmv.d f%d, f%d\n\n",dpath.rd,dpath.rs1);
        DP_Reg[dpath.rd] = DP_Reg[dpath.rs1];
        break;
    case FNEG_D:            // rd = -rs
        if(verbose)
            printf("fneg.d f%d, f%d\n\n",dpath.rd,dpath.rs1 );
        DP_Reg[dpath.rd] = -DP_Reg[dpath.rs1];
        break;
    /*case FSGNJ_D:
        {
            if (verbose)
                printf("fsgnj.d f%d, f%d, f%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
            FP_Reg[dpath.rd] = fabs(FP_Reg[dpath.rs1]);
            if (FP_Reg[dpath.rs2] < 0)
                FP_Reg[dpath.rd] = -FP_Reg[dpath.rd];  
        }
        break;
    case FSGNJN_D:
        {
            if (verbose)
                printf("fsgnj.d f%d, f%d, f%d\n\n", dpath.rd, dpath.rs1, dpath.rs2);
            FP_Reg[dpath.rd] = fabs(FP_Reg[dpath.rs1]);
            if (FP_Reg[dpath.rs2] > 0)
                FP_Reg[dpath.rd] = -FP_Reg[dpath.rd];
        }
        break;*/
        
    /* D-extension for dhry end */  

    default:
        if (verbose)
            printf("Undefined instruction\n");
    }
}

void set_cache()
{
	cache.SetLower(&L2);
	L2.SetLower(&LLC);
	LLC.SetLower(&m);

	StorageLatency lt;
	lt.hit_latency = 10;
	cache.SetLatency(lt);
	L2.SetLatency(lt);
	LLC.SetLatency(lt);

	lt.hit_latency = 100;
	m.SetLatency(lt);

	CacheConfig cc;
	cc.write_through = 0;
	cc.write_allocate = 1;
	cc.associativity = 8;
	cc.block_size = 64;
	cc.set_num = 32*1024/8/64;
	cache.SetConfig(cc);

	cc.set_num = 256*1024/8/64;
	L2.SetConfig(cc);
	cc.set_num = 8*1024*1024/8/64;
	LLC.SetConfig(cc);

}
