#ifndef __DECODER_H
#define __DECODER_H

#include <stdio.h>

typedef struct {
  int opcode; 
  int funct3, funct7;
  int rs1, rs2, rd;

  int shamt;
  int imm_I;
  int imm_S;
  int imm_SB;
  int imm_U;
  int imm_UJ;

}Data_Path;

typedef enum {
  LB, LH, LW, LBU, LHU, LWU, LD,
  SB, SH, SW, SD,
  
  ADD, ADDI, SUB, LUI, AUIPC,
  ADDW, ADDIW, SUBW,

  XOR, XORI, OR, ORI, AND, ANDI,

  SLL, SLLI, SRL, SRLI, SRA, SRAI,
  SLLW, SLLIW, SRLW, SRLIW, SRAW, SRAIW,

  SLT, SLTI, SLTU, SLTIU,
  BEQ, BNE, BLT, BGE, BLTU, BGEU,

  JAL, JALR,
  ECALL,

  MUL, MULW, MULH, DIV, DIVU, 
  DIVW, DIVUW, REM, REMU, REMW, REMUW,

  
  FLW, FSW, FADD_S, FSUB_S, FMUL_S, FDIV_S,
  FEQ_S, FLT_S, FLE_S, 
  FCVT_S_L, FCVT_S_W, FCVT_W_S, FCVT_L_S,
  
  FCVT_D_S, FMUL_D, FDIV_D, FCVT_S_D, FMV_X_D,
  FMV_D_X, FADD_D, FSUB_D, FLD, FSD, FEQ_D, 

  FLT_D, FLE_D, FCVT_D_W, FCVT_D_WU, FCVT_W_D,
  FCVT_WU_D, FMADD_D, FMV_D, FNEG_D,

  UNDEFINED
}Instr_Type;

char instr_Name[][10] = {
  "LB", "LH", "LW", "LBU", "LHU", "LWU", "LD",
  "SB", "SH", "SW", "SD",
  
  "ADD", "ADDI", "SUB", "LUI", "AUIPC",
  "ADDW", "ADDIW", "SUBW",

  "XOR", "XORI", "OR", "ORI", "AND", "ANDI",

  "SLL", "SLLI", "SRL", "SRLI", "SRA", "SRAI",
  "SLLW", "SLLIW", "SRLW", "SRLIW", "SRAW", "SRAIW",

  "SLT", "SLTI", "SLTU", "SLTIU",
  "BEQ", "BNE", "BLT", "BGE", "BLTU", "BGEU",

  "JAL", "JALR",
  "ECALL",

  "MUL", "MULW", "MULH", "DIV", "DIVU", 
  "DIVW", "DIVUW", "REM", "REMU", "REMW", "REMUW",

  "FLW", "FSW", "FADD.S", "FSUB.S", "FMUL.S", "FDIV.S",
  "FEQ.S", "FLT.S", "FLE.S", 
  "FCVT.S.L", "FCVT.S.W", "FCVT.W.S", "FCVT.L.S",

  "FCVT.D.S", "FMUL.D", "FDIV.D", "FCVT.S.D", "FMV.X.D",
  "FMV.D.X", "FADD.D", "FSUB.D", "FLD", "FSD", "FEQ.D",

  "FLT.D", "FLE.D", "FCVT.D.W", "FCVT.D.WU", "FCVT.W.D",
  "FCVT.WU.D", "FMADD.D", "FMV.D", "FNEG.D",
  
  "UNDEFINED"
};

#define _LOAD 0x0 /* 00000 */
#define _OP_IMM 0x4 /* 00100 */
#define _AUIPC 0x5 /* 00101 */
#define _OP_IMM_32 0x6 /* 00110 */

#define _STORE 0x8 /* 01000 */
#define _OP 0xc /* 01100 */
#define _LUI 0xd /* 01101 */
#define _OP_32 0xe /* 01110 */

#define _BRANCH 0x18 /* 11000 */
#define _JALR 0x019 /* 11001 */
#define _JAL 0x01b /* 11011 */

#define _LOAD_FP 0x1 /* 00001 */
#define _STORE_FP 0x9 /* 01001 */
#define _OP_FP 0x14 /* 10100 */

#define _SYSTEM 0x1c /* 11100 */
#define _MADD 0x10 /* 10000 */

Instr_Type decode(unsigned int instr, Data_Path *dp)
{
  int opcode = instr & 0x7f;
  opcode >>= 2;
  /* printf("opcode = %x\n", opcode); */
  int funct3 = (instr >> 12) & 0x7;
  int funct7 = (instr >> 25) & 0x7f;
  int funct5 = funct7 >> 2;

  dp->opcode = opcode;
  dp->funct3 = (instr >> 12) & 0x7;
  dp->funct7 = (instr >> 25) & 0x7f;

  dp->rd = (instr >> 7) & 0x1f;
  dp->rs1 = (instr >> 15) & 0x1f;
  dp->rs2 = (instr >> 20) & 0x1f;

  dp->imm_I = (instr >> 20) & 0xfff;
  dp->imm_S = (dp->funct7 << 5) | (dp->rd);
  int s1 = (funct7 >> 6 << 12) | ((dp->rd & 0x1) << 11);
  dp->imm_SB = s1 | ((funct7 & 0x3f) << 5) | (dp->rd >> 1 << 1);
  
  dp->imm_U = instr & (-1 << 12);
  int u1 = (funct7 >> 6 << 20) | ((dp->rs2 & 0x1) << 11);
  int u2 = (dp->rs1 << 3) | dp->funct3;
  int u3 = ((unsigned int)instr >> 21) & 0x3ff;
  dp->imm_UJ = u1 | (u2 << 12) | (u3 << 1);

  Instr_Type type;
  switch(opcode) {
  case _LOAD:
    switch(funct3) {
    case 0x0:
      type = LB;
      break;
    case 0x1:
      type = LH;
      break;
    case 0x2:
      type = LW;
      break;
    case 0x4:
      type = LBU;
      break;
    case 0x5:
      type = LHU;
      break;
    case 0x6:
      type = LWU;
      break;
    case 0x3:
      type = LD;
      break;
    }
    break;

  case _OP_IMM:
    switch(funct3) {
    case 0x0:
      type = ADDI;
      break;
    case 0x2:
      type = SLTI;
      break;
    case 0x3:
      type = SLTIU;
      break;
    case 0x4:
      type = XORI;
      break;
    case 0x6:
      type = ORI;
      break;  
    case 0x7:
      type = ANDI;
      break;
    case 0x1:
      type = SLLI;
      dp->shamt = ((funct7 & 0x1) << 5) | dp->rs2;
      break;
    case 0x5:
      dp->shamt = ((funct7 & 0x1) << 5) | dp->rs2;
      if ((instr & (1 << 30)) == 0)
        type = SRLI;
      else
        type = SRAI;
      break;
    }
    break;

  case _AUIPC:
    type = AUIPC;
    break;

  case _OP_IMM_32:
    switch(funct3) {
    case 0x0:
      type = ADDIW;
      break;
    case 0x1:
      type = SLLIW;
      dp->shamt = dp->rs2;
      break;
    case 0x5:
      dp->shamt = dp->rs2;
      if ((instr & (1 << 30)) == 0)
        type = SRLIW;
      else
        type = SRAIW;
      break;
    }
    break;

  case _STORE:
    switch(funct3) {
    case 0x0:
      type = SB;
      break;
    case 0x1:
      type = SH;
      break;
    case 0x2:
      type = SW;
      break;
    case 0x3:
      type = SD;
      break;
    }
    break;

  case _OP:
    if (funct7 == 1)
      switch(funct3) {
      case 0x0:
        type = MUL;
        break;
      case 0x1:
        type = MULH;
        break;
      /*case 0x2:
        type = MULHSU;
        break;
      case 0x3:
        type = MULHU;
        break;*/
      case 0x4:
        type = DIV;
        break;
      case 0x5:
        type = DIVU;
        break;
      case 0x6:
        type = REM;
        break;
      case 0x7:
        type = REMU;
        break;
      }
    
    else  
      switch(funct3) {
      case 0x0:
        if((instr & (1 << 30)) == 0)
          type = ADD;
        else
          type = SUB;
        break;
      case 0x1:
        type = SLL;
        break;
      case 0x2:
        type = SLT;
        break;
      case 0x3:
        type = SLTU;
        break;
      case 0x4:
        type = XOR;
        break;
      case 0x5:
        if((instr & (1 << 30)) == 0)
          type = SRL;
        else
          type = SRA;
        break;
      case 0x6:
        type = OR;
        break;
      case 0x7:
        type = AND;
        break;
    }
    break;
    
  case _LUI:
    type = LUI;
    break;
    
  case _OP_32:
    if (funct7 == 1)
      switch (funct3) {
      case 0x0:
        type = MULW;
        break;
      case 0x4:
        type = DIVW;
        break;
      case 0x5:
        type = DIVUW;
        break;
      case 0x6:
        type = REMW;
        break;
      case 0x7:
        type = REMUW;
        break;
      }

    else
      switch(funct3) {
      case 0x0:
      if((instr & (1 << 30)) == 0)
        type = ADDW;
      else
        type = SUBW;
      break;
      case 0x1:
      type = SLLW;
      break;
      case 0x5:
        if((instr & (1 << 30)) == 0)
          type = SRLW;
        else
          type = SRAW;
      break;
      }
    break;
    
  case _BRANCH:
    switch(funct3) {
      case 0x0:
        type = BEQ;
        break;
      case 0x1:
        type = BNE;
        break;
      case 0x4:
        type = BLT;
        break;
      case 0x5:
        type = BGE;
        break;
      case 0x6:
        type = BLTU;
        break;
      case 0x7:
        type = BGEU;
        break;
    }
    break;
    
  case _JALR:
    type = JALR;
    break;
    
  case _JAL:
    type = JAL;
    break;

   /* F-extension for dhry 2 */
  case _LOAD_FP:
    switch(funct3) {
    case 0x2:
      type = FLW;
      break;
    case 0x3:
      type = FLD;
      break;
    }
    break;

  case _STORE_FP:
    switch(funct3) {
    case 0x2:
      type = FSW;
      break;
    case 0x3:
      type = FSD;
      break;
    }
    break;
  /* end 2 */

  case _OP_FP:
    switch(funct7) {
    case 0x0:
      type = FADD_S;
      break;
    case 0x4:
      type = FSUB_S;
      break;
    case 0x8:
      type = FMUL_S;
      break;
    case 0xc:
      type = FDIV_S;
      break;
    case 0x50:
      switch(funct3) {
        case 0x0:
          type = FLE_S;
          break;
        case 0x1:
          type = FLT_S;
          break;
        case 0x2:
          type = FEQ_S;
          break;
      }

   /* F-extension for dhry */
    case 0x01:
      type = FADD_D;
      break;
    case 0x05:
      type = FSUB_D;
      break;
    case 0x9:
      type = FMUL_D;
      break;  
    case 0x0d:
      type = FDIV_D;
      break;

    case 0x68:
      switch(dp->rs2) {
        case 0x00:
          type = FCVT_S_W;
          break;
        case 0x02:
          type = FCVT_S_L;
          break;
      }
      break;
    case 0x60:
      switch(dp->rs2) {
        case 0x0:
          type = FCVT_W_S;
          break;
        case 0x2:
          type = FCVT_L_S;
          break;
      }
      break;

    case 0x21:
      type = FCVT_D_S;
      break;
    case 0x20:
      type = FCVT_S_D;
      break;
    case 0x71:
      type = FMV_X_D;
      break; 
    case 0x79:
      type = FMV_D_X;  
      break;
    case 0x51:
      switch(funct3) {
        case 0x0:
          type = FLE_D;
          break;
        case 0x1:
          type = FLT_D;
          break;
        case 0x2:
          type = FEQ_D;
          break;  
      }
      break;
    case 0x61:
      switch(dp->rs2) {
        case 0x0:
          type = FCVT_W_D;
          break;
        case 0x1:
          type = FCVT_WU_D;
          break;

      }
      break;
    case 0x69:
      switch(dp->rs2) {
        case 0x0:
          type = FCVT_D_W;
          break;
        case 0x1:
          type = FCVT_D_WU;
          break;  
      }  
      break;
    case 0x11:
      switch(funct3) {
        case 0x0:
          type = FMV_D;   // pseudoinstr, real instr fsgnj.d rx,ry, ry
          break;
        case 0x1:
          type = FNEG_D;  // pseudoinstr, real instr fsgnjn.d rx, ry, ry
          break;
      }
    break;
     /* F-extension for dhry end */

    default:
      type = UNDEFINED;
      printf("Undefined floating point instruction\n");
    }
    break;

  case _MADD:      // fmadd.d
    type = FMADD_D;
    break;

  case _SYSTEM:
    if(dp->imm_I == 0)
	    type = ECALL;
    break;
  default:
    type = UNDEFINED;
    printf("Undefined opcode\n");
  }
  return type;
}

#endif
