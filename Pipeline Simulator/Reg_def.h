typedef unsigned long long REG;

/* Instruction and Control Signals */
struct Instruction_Related{
    /* Instruction related */
    unsigned int inst;
    int PC; // points to next instruction
    int tmp_PC;  // PC of this inst
    unsigned long long imm;
    int rs1;
    int rs2;
    int rd;
    
    /* Control Signal */
    char MemRead;
    char MemWrite;
    char MemtoReg;
    char RegWrite;
    char ALUSrcA;
    char ALUSrcB;
    char ALUOp;
    char PCSrc;
    char ExtOp;
    
    /* inner register */
    unsigned long long alu_rst;  // might also be data_to_reg
    unsigned long long data_from_memory;  // data read out from M[alu_rst]
    
    /* data type */
    char data_type; // related to M read/write
    
    /* whether it is a bubble */
    char is_bubble;
}info[5];

#define NO 0
#define YES 1

// ALU_OP
#define ADD 0
#define SUB 1
#define MUL 2
#define DIV 3
#define SLL 4
#define XOR 5
#define SRA 6
#define SRL 7
#define OR 8
#define AND 9
#define REM 10  // %
#define EQ 11
#define NEQ 12
#define GT 13  // greater than
#define LT 14  // less than
#define GE 15  // greater equal
#define MULH 16  // get higher 64 bits
#define SLT 17  // reg[rd] = ((long long)reg[rs1] < (long long)reg[rs2]) ? 1: 0;

// ALU_SrcA
#define R_RS1 0
#define PROGRAM_COUNTER 1
#define ZERO 2

// ALU_SrcB
#define R_RS2 0
#define IMM 1
#define IMM_05 2
#define IMM_SLL_12 3
#define FOUR 4

// PC_Src
#define NORMAL 0  // plus 4
#define R_RS1_IMM  1 // R[rs1] + imm
#define PC_IMM 2 // PC + imm

// data_type
#define BYTE 0
#define HALF 1
#define WORD 2
#define DOUBLEWORD 3
