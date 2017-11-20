/*
 1）就很纳闷。decode之后，那些信号量到底变不变？？？看ppt
 2）何时判断是否要插入nop
 3）运算结果存在哪里？
     多加几个寄存器吧，又不要钱
 4）如果知道当前这个指令是什么了，那么岂不是后面环节的行为都是知道的？？？
 5）如何知道是不是预测错了？
     就是在肯定知道结果的那个环节之后检查一下后面那个的pc是不是预测的那个咯。是的话全给冲掉
 6）jalr执行一个周期还是两个周期？
 7）每个info里面的pc还得是下一个周期的pc，不然没法取下一条的指令啊宝贝
     可是 可是你想啊，之前单周期的时候，那些pc+imm也是用的自己的pc啊
 8）感觉有一些信号量是不必要的啊
 9）EXT_OP
     这个信号与数据类型有关。是不是还得添加一个数据类型位？？？
 10）什么时候写进去pc？？
     既然默认不转移了，那么其实无所谓了吧
 11）在访问memory之前，alu_rst需要先变成int类型！在memory对应的环节再做吧
     呃原来写的时候也不知道是怎么想的……其实并不需要
     已经改了，用long long就行
 12）slti需要转换成有符号数比较大小咋办呢？？？【重要】
     发现slt也是这样。当成规定动作呗
 13）对于ori好像不能对imm做符号扩展嘿？
 14）发现之前srai写错了 gg
 15）符号扩展从哪里开始，是需要特判的。如果是sari那么情况会特殊！都是63 - int(ALUSrcB)
 16）加了一个data type之后很麻烦。怎么知道是哪个周期需要的啊？memory还是execute的结果？
     发现凡是有memory的，利用的地址都是int类型，所以datatype是针对read或者write的类型的
     其他时候针对的是execute的结果
 17) 对于jalr，跳转肯定会发生啊宝贝！
 18）什么时候需要把写入内存的数据准备好呢？
 19）在执行过程中，把pc准备好.
     从执行过程往后，info[index].PC中存储的就是下一条指令的地址了
 20）slli的func7好像没有设置过哦。因为那个地方也是imm的位置。
 21）然后fill_out_control里面的func3啥的最好换成info[index].func3，为流水线做准备
 22）很奇怪。跑6.cpp的时候发现全局变量int result=15;被放在.sdata段里面，所以没有被load进来
     多load进来一点好了！
 */

#include "Simulation.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

extern void read_elf(const char*);
extern unsigned int dadr;
extern unsigned int dsize;
extern unsigned int cadr;
extern unsigned int csize;
extern unsigned int vadr;
extern unsigned int vdadr;
extern unsigned long long gp;  // read out from symbol table.
extern unsigned int madr;
extern unsigned int endPC;
extern unsigned int entry;
extern bool single_step;
extern bool run_til;
extern unsigned int pause_addr;
extern FILE *file;
extern FILE *ftmp;

//#define DEBUG

/* load data and instructions */
void load_memory()
{
    /* load .text */
    fseek(file,cadr,SEEK_SET);
    fread(&memory[vadr],1,csize,file);

    /* load .data */
    fseek(file, dadr, SEEK_SET);
    fread(&memory[vdadr], 1, dsize, file);
}

void fetch_instruction(int index){
#ifdef DEBUG
    cout << "entered fetch-instruction" << endl;
#endif
    
    /* put instruction in info[index] */
    info[index].inst = read_memory_word(info[index].PC);
}

/* decode and execute */
void decode(int index){
#ifdef DEBUG
    cout << "entered decode" << endl;
#endif
    
    unsigned int inst = info[index].inst;
    unsigned int tmp;
    
    opcode = getbit(inst, 0, 6);
    
    switch (opcode){
            /* R */
        case 0x33:
            rd = getbit(inst, 7, 11);
            func3 = getbit(inst, 12, 14);
            rs1 = getbit(inst, 15, 19);
            rs2 = getbit(inst, 20, 24);
            func7 = getbit(inst, 25, 31);
            
            fill_control_R(index);
            break;
            
            /* I */
        case 0x03:
        case 0x13:
        case 0x1B:
        case 0x67:
        case 0x73:
            rd = getbit(inst, 7, 11);
            func3 = getbit(inst, 12, 14);
            rs1 = getbit(inst, 15, 19);
            func7 = getbit(inst, 25, 31);
            imm12 = getbit(inst, 20, 31);
            imm = ext_signed((unsigned int)imm12, 11);
            
            fill_control_I(index);
            break;
            
            /* S (store) */
        case 0x23:
            imm5 = getbit(inst, 7, 11);
            func3 = getbit(inst, 12, 14);
            rs1 = getbit(inst, 15, 19);
            rs2 = getbit(inst, 20, 24);
            func7 = getbit(inst, 25, 31);
            imm7 = getbit(inst, 25, 31);
            tmp = (imm7 << 5) | imm5;
            imm = ext_signed(tmp, 11);
            
            fill_control_S(index);
            break;
            
            /* SB (switch branch) */
        case 0x63:
            func3 = getbit(inst, 12, 14);
            rs1 = getbit(inst, 15, 19);
            rs2 = getbit(inst, 20, 24);
            func7 = getbit(inst, 25, 31);
            tmp = (getbit(inst, 8, 11) << 1) | (getbit(inst, 25, 30) << 5) | (getbit(inst, 7, 7) << 11) | (getbit(inst, 31, 31) << 12);
            imm = ext_signed(tmp, 12);
            
            fill_control_SB(index);
            break;
            
            /* U */
        case 0x17:
        case 0x37:
            rd = getbit(inst, 7, 11);
            imm20 = getbit(inst, 12, 31);
            func7 = getbit(inst, 25, 31);
            tmp = imm20;// << 12;
            imm = ext_signed(tmp, 31);
            
            fill_control_U(index);
            break;
            
            /* UJ */
        case 0x6f:
            rd = getbit(inst, 7, 11);
            func7 = getbit(inst, 25, 31);
            tmp = (getbit(inst, 12, 19) << 12) | (getbit(inst, 20, 20) << 11) | (getbit(inst, 21, 30) << 1) | (getbit(inst, 31, 31) << 20);
            imm = ext_signed(tmp, 20);
            
            fill_control_UJ(index);
            break;
            
            /* extended */
        case 0x3b:
            rd = getbit(inst, 7, 11);
            func3 = getbit(inst, 12, 14);
            rs1 = getbit(inst, 15, 19);
            rs2 = getbit(inst, 20, 24);
            func7 = getbit(inst, 25, 31);
            
            fill_control_R(index);
            break;
    }
    
    /* store related Control Info in info[index] */
    write_inst_info(index);
}

void fill_control_R(int index){
#ifdef DEBUG
    cout << "entered fill_control_R()" << endl;
#endif
    
    ALUSrcA = R_RS1;
    ALUSrcB = R_RS2;
    MemRead = NO;
    MemWrite = NO;
    MemtoReg = NO;
    RegWrite = YES;
    PCSrc = NORMAL;
    ExtOp = NO;
    
    switch(opcode){
        /* regular R-type */
        case 0x33:
            data_type = DOUBLEWORD;
            
            switch (func3){
                case 0x0:
                    switch (func7){
                        case 0x00: // add rd, rs1, rs2
                            ALUOp = ADD;
                            break;
                        case 0x01: // mul rd, rs1, rs2
                            ALUOp = MUL;
                            break;
                        case 0x20: // sub rd, rs1, rs2
                            ALUOp = SUB;
                            break;
                    }
                    break;
                    
                case 0x1:
                    switch (func7){
                        case 0x00: // sll rd, rs1, rs2
                            ALUOp = SLL;
                            break;
                        case 0x01: // mulh rd, rs1, rs2
                            ALUOp = MULH;
                            break;
                    }
                    break;
                    
                case 0x2:
                    if(func7 == 0x00){ // slt rd, rs1, rs2
                        ALUOp = SLT;
                    }
                    break;
                    
                case 0x4:
                    switch (func7){
                        case 0x00: // xor rd, rs1, rs2
                            ALUOp = XOR;
                            break;
                        case 0x01: // div rd, rs1, rs2
                            ALUOp = DIV;
                            break;
                    }
                    break;
                    
                case 0x5:
                    switch (func7){
                        case 0x00: // srl rd, rs1, rs2
                            ALUOp = SRL;
                            break;
                        case 0x20: // sra rd, rs1, rs2
                            ALUOp = SRA;
                            ExtOp = YES;
                            break;
                    }
                    break;
                    
                case 0x6:
                    switch (func7){
                        case 0x00: // or rd, rs1, rs2
                            ALUOp = OR;
                            break;
                        case 0x01: // rem rd, rs1, rs2
                            ALUOp = REM;
                            break;
                    }
                    break;
                    
                case 0x7:
                    if(func7 == 0x00){ // and rd, rs1, rs2
                        ALUOp = AND;
                    }
                    break;
            }
            break;
        
        /* extended */
        case 0x3b:
            data_type = WORD;
            
            switch (func3){
                case 0x0:
                    if(func7 == 0x00){ // addw rd, rs1, rs2
                        ALUOp = ADD;
                    }
                    else if(func7 == 0x20){ // subw rd, rs1, rs2
                        ALUOp = SUB;
                    }
                    else if(func7 == 0x01){ // mulw rd, rs1, rs2
                        ALUOp = MUL;
                    }
                    break;
                    
                case 0x1: // sllw
                    ALUOp = SLL;
                    break;
                    
                case 0x4: // divw
                    ALUOp = DIV;
                    break;
                    
                case 0x5:
                    if(func7 == 0x00){ // srlw rd, rs1, rs2
                        ALUOp = SRL;
                    }
                    else{ // sraw rd, rs1, rs2
                        ALUOp = SRA;
                        ExtOp = YES;
                    }
                    break;
            }
    }
    
    /* Store control info */
    write_control_signal(index);
}

void fill_control_I(int index){
    unsigned int tmp;
    switch (opcode) {
        case 0x03:
            ALUSrcA = R_RS1;
            ALUSrcB = IMM;
            ALUOp = ADD;
            MemRead = YES;
            MemWrite = NO;
            MemtoReg = YES;
            RegWrite = YES;
            PCSrc = NORMAL;
            ExtOp = YES;
            
            switch (func3){
                case 0x0: // lb rd, offset(rs1)
                    data_type = BYTE;
                    break;
                    
                case 0x1: // lh rd, offset(rs1)
                    data_type = HALF;
                    break;
                    
                case 0x2: // lw rd, offset(rs1)
                    data_type = WORD;
                    break;
                    
                case 0x3: // ld rd, offset(rs1)
                    data_type = DOUBLEWORD;
                    break;
            }
            break;
            
        case 0x13:
            ALUSrcA = R_RS1;
            ALUSrcB = IMM;
            MemRead = NO;
            MemWrite = NO;
            MemtoReg = NO;
            RegWrite = YES;
            PCSrc = NORMAL;
            ExtOp = NO;
            data_type = DOUBLEWORD;
            
            switch (func3){
                case 0x0: // addi rd, rs1, imm
                    ALUOp = ADD;
                    break;
                    
                case 0x1: // slli rd, rs1, imm
                    if (func7 == 0x00){
                        ALUOp = SLL;
                        ALUSrcB = IMM_05;
                    }
                    break;
                    
                case 0x2: // slti rd, rs1, imm
                    ALUOp = SLT;
                    break;
                    
                case 0x4: // xori rd, rs1, imm
                    ALUOp = XOR;
                    break;
                    
                case 0x5: // srli rd, rs1, imm
                    if (func7 == 0x00){
                        ALUOp = SRL;
                        ALUSrcB = IMM_05;
                    }
                    else if (func7 == 0x10){ // srai rd, rs1, imm
                        ALUSrcB = IMM_05;
                        ALUOp = SRA;
                        ExtOp = YES;
                    }
                    break;
                    
                case 0x6: // ori rd, rs1, imm
                    ALUOp = OR;
                    break;
                    
                case 0x7: // andi rd, rs1, imm
                    ALUOp = AND;
                    break;
            }
            break;
            
        case 0x1B:
            ALUSrcA = R_RS1;
            ALUSrcB = IMM;
            MemRead = NO;
            MemWrite = NO;
            MemtoReg = NO;
            RegWrite = YES;
            PCSrc = NORMAL;
            ExtOp = YES;
            data_type = WORD;
            
            switch (func3){
                case 0x0: // addiw rd, rs1, imm
                    ALUOp = ADD;
                    break;
                case 0x1: // slliw rd, rs1, imm
                    ALUOp = SLL;
                    break;
                case 0x5:
                    // srliw rd, rs1, imm
                    if (getbit(info[index].inst, 30, 30) == 0){
                        ALUOp = SRL;
                        ALUSrcB = IMM_05;
                    }
                    // sraiw rd, rs1, imm
                    else{
                        ALUOp = SRA;
                        ALUSrcB = IMM_05;
                    }
                    break;
            }
            break;
            
        case 0x67:
            switch (func3){
                case 0x0: // Jalr rd, rs1, imm
                    ALUSrcA = PROGRAM_COUNTER;
                    ALUSrcB = FOUR;
                    ALUOp = ADD;
                    MemRead = NO;
                    MemWrite = NO;
                    MemtoReg = NO;
                    RegWrite = YES;
                    PCSrc = R_RS1_IMM;  // R[rs1] + imm
                    ExtOp = NO;
                    data_type = DOUBLEWORD;
                    break;
            }
            break;
            
        case 0x73:
            switch (func3){
                case 0x0: // ecall
                    if(func7 == 0x000){ // (Transfers control to operating system)
                        exit_flag = 1;
                    }
                    break;
            }
            break;
    }
    
    /* Store control info */
    write_control_signal(index);
}

void fill_control_S(int index){
#ifdef DEBUG
    cout << "entered fill_control_S()" << endl;
#endif
    
    switch (opcode){
        case 0x23:
            ALUSrcA = R_RS1;
            ALUSrcB = IMM;
            ALUOp = ADD;
            MemRead = NO;
            MemWrite = YES;
            MemtoReg = NO;
            RegWrite = NO;
            PCSrc = NORMAL;
            ExtOp = NO;
            
            if(func3 == 0x0){ // sb rs2, offset(rs1)
                data_type = BYTE;
            }
            else if(func3 == 0x1){ // sh rs2, offset(rs1)
                data_type = HALF;
            }
            else if(func3 == 0x2){ // sw rs2, offset(rs1)
                data_type = WORD;
            }
            else if(func3 == 0x3){ // sd rs2, offset(rs1)
                data_type = DOUBLEWORD;
            }
            break;
    }
    
    /* Store control info */
    write_control_signal(index);
}

void fill_control_SB(int index){
    switch (opcode){
        case 0x63:
            ALUSrcA = R_RS1;
            ALUSrcB = R_RS2;
            MemRead = NO;
            MemWrite = NO;
            MemtoReg = NO;
            RegWrite = NO;
            PCSrc = PC_IMM;
            ExtOp = NO;
            data_type = DOUBLEWORD;
            
            if(func3 == 0x0){ // beq rs1, rs2, offset
                ALUOp = EQ;
            }
            else if(func3 == 0x1){ // bne rs1, rs2, offset
                ALUOp = NEQ;
            }
            else if(func3 == 0x4){ // blt rs1, rs2, offset
                ALUOp = LT;
            }
            else if(func3 == 0x5){ // bge rs1, rs2, offset
                ALUOp = GE;
            }
            break;
    }
    
    /* Store control info */
    write_control_signal(index);
}

void fill_control_U(int index){
#ifdef DEBUG
    cout << "entered fill_control_U()" << endl;
#endif
    
    switch (opcode){
        case 0x17: // auipc rd, offset
            ALUSrcA = PROGRAM_COUNTER;
            ALUSrcB = IMM_SLL_12;
            ALUOp = ADD;
            MemRead = NO;
            MemWrite = NO;
            MemtoReg = NO;
            RegWrite = YES;
            PCSrc = NORMAL;
            ExtOp = NO;
            data_type = DOUBLEWORD;
            break;
            
        case 0x37: // lui rd, offset
            ALUSrcA = ZERO;
            ALUSrcB = IMM_SLL_12;
            ALUOp = ADD;
            MemRead = NO;
            MemWrite = NO;
            MemtoReg = NO;
            RegWrite = YES;
            PCSrc = NORMAL;
            ExtOp = NO;
            data_type = DOUBLEWORD;
            break;
    }
    
    /* Store control info */
    write_control_signal(index);
}

void fill_control_UJ(int index){
#ifdef DEBUG
    cout << "entered fill_control_UJ()" << endl;
#endif
    
    switch (opcode){
        case 0x6f: // jal rd, imm
//            reg[rd] = PC + 4;
//            PC = PC + imm;
//            update_PC = true;
            ALUSrcA = PROGRAM_COUNTER;
            ALUSrcB = FOUR;
            data_type = ADD;
            MemRead = NO;
            MemWrite = NO;
            MemtoReg = NO;
            RegWrite = YES;
            PCSrc = PC_IMM;
            ExtOp = NO;
            data_type = DOUBLEWORD;
            break;
    }
    
    /* Store control info */
    write_control_signal(index);
}

void execute(int index){
#ifdef DEBUG
    cout << "entered execute()" << endl;
#endif
    // print_control_info(index);
    
    unsigned long long op1;
    unsigned long long op2;
    int ext_pos;  // the index of sign bit

    /* find the sign bit */
    switch(info[index].data_type){
        case BYTE:
            ext_pos = 7;
            break;
            
        case HALF:
            ext_pos = 15;
            break;
            
        case WORD:
            ext_pos = 31;
            break;
        
        default:
            ext_pos = 63;
    }
#ifdef DEBUG
    cout << "entered data_type switch" << endl;
#endif
    
    /* the first operand */
    switch(info[index].ALUSrcA){
        case R_RS1:
            op1 = reg[info[index].rs1];
            break;
            
        case PROGRAM_COUNTER:
            op1 = info[index].PC;
            break;
            
        case ZERO:
            op1 = 0;
            break;
    }
#ifdef DEBUG
    cout << "entered ALUSrcA switch" << endl;
#endif

    /* the second operand */
    switch (info[index].ALUSrcB) {
        case R_RS2:
            op2 = reg[info[index].rs2];
            break;
            
        case IMM:
            op2 = info[index].imm;
            break;
            
        case IMM_05:
            op2 = info[index].imm & 63;
            break;
            
        case IMM_SLL_12:
            op2 = info[index].imm << 12;
            break;
            
        case FOUR:
            op2 = 4;
            break;
    }
#ifdef DEBUG
    cout << "entered ALUSrcB switch" << endl;
#endif
    
    /* which operation */
    switch(info[index].ALUOp){
        case ADD:
            alu_rst = op1 + op2;
            break;
            
        case SUB:
            alu_rst = op1 - op2;
            break;
            
        case MUL:
            alu_rst = op1 * op2;
            break;
            
        case DIV:
            alu_rst = op1 / op2;
            break;
            
        case SLL:
            alu_rst = op1 << op2;
            break;
            
        case XOR:
            alu_rst = op1 ^ op2;
            break;
            
        case SRA:  // and EXT_OP, special operations
            ext_pos = 63 - (int)reg[info[index].rs2]; //63 - (int)reg[rs2]
        case SRL:
            alu_rst = op1 >> op2;
            break;
            
        case OR:
            alu_rst = op1 | op2;
            break;
            
        case AND:
            alu_rst = op1 & op2;
            break;
            
        case REM:
            alu_rst = op1 % op2;
            break;
            
        case EQ:
            alu_rst = (op1 == op2);
            break;
            
        case NEQ:
            alu_rst = (op1 != op2);
            break;
            
        case GT:
            alu_rst = (op1 > op2);
            break;
            
        case LT:
            alu_rst = (op1 < op2);
            break;
            
        case GE:
            alu_rst = (op1 >= op2);
            cout << "in GE: alu_rst = " << alu_rst << endl;
            break;
        
        case SLT:  // change to signed number
            alu_rst = ((long long)op1 < (long long)op2);
            break;
            
        case MULH:
            long long ah = ((op1 & 0xffff0000) >> 32) & 0xffff;
            long long al = op1 & 0x0000ffff;
            long long bh = ((op2 & 0xffff0000) >> 32) & 0xffff;
            long long bl = op2 & 0x0000ffff;
            long long ah_mul_bh = ah * bh;
            long long ah_mul_bl = ((ah * bl) >> 32) & 0xffff;
            long long al_mul_bh = ((al * bh) >> 32) & 0xffff;
            alu_rst = ah_mul_bh + ah_mul_bl + al_mul_bh;
            break;
    }
#ifdef DEBUG
    cout << "entered ALUOp switch" << endl;
#endif
    
    /* sign extending */
    if(info[index].ExtOp && info[index].MemRead == NO){
        alu_rst = ext_signed(alu_rst, ext_pos);
    }
    
    /* for 32-bit */
    if(info[index].MemRead == NO && info[index].MemWrite == NO && info[index].data_type == WORD){
        alu_rst = 0xffffffff & alu_rst;
    }
    
    /* store alu_rst back to info[index] */
    info[index].alu_rst = alu_rst;
    
    /* update PC */
    switch(info[index].PCSrc){
        case NORMAL:
#ifdef DEBUG
            cout << "entered branch NORMAL" << endl;
#endif
            
            info[index].PC += 4;
            break;
            
        case R_RS1_IMM:
#ifdef DEBUG
            cout << "entered branch R_RS1_IMM" << endl;
#endif
            info[index].PC = reg[info[index].rs1] + info[index].imm;
            break;
            
        case PC_IMM:
#ifdef DEBUG
            cout << "entered branch PC_IMM" << endl;
#endif
            if(info[index].alu_rst != 0){
#ifdef DEBUG
                cout << "WILL CHANGE PC!" << endl;
#endif
                info[index].PC += info[index].imm;
            }
            else{
                info[index].PC += 4;
            }
            break;
    }
#ifdef DEBUG
    cout << "entered PC switch! " << endl;
#endif
}

void memory_read_write(int index){
#ifdef DEBUG
    cout << "entered memory_read_write()" << endl;
#endif
    
    if(info[index].MemRead == YES){
        switch(info[index].data_type){
            case BYTE:
                data_from_memory = read_memory_byte(info[index].alu_rst);
                data_from_memory = ext_signed(data_from_memory, 7);
                break;
                
            case HALF:
                data_from_memory = read_memory_half(info[index].alu_rst);
                data_from_memory = ext_signed(data_from_memory, 15);
                break;
                
            case WORD:
                data_from_memory = read_memory_word(info[index].alu_rst);
                data_from_memory = ext_signed(data_from_memory, 31);
                break;
                
            case DOUBLEWORD:
                data_from_memory = read_memory_doubleword(info[index].alu_rst);
                break;
        }
        
        /* write data_from_memory to info[index] */
        info[index].data_from_memory = data_from_memory;
    }
    
    if(info[index].MemWrite == YES){
        switch(info[index].data_type){
            case BYTE:  // data_to_memory would always be in reg[rs2]
                write_to_memory(index[info].alu_rst, reg[index[info].rs2], 1);
                break;
                
            case HALF:
                write_to_memory(index[info].alu_rst, reg[index[info].rs2], 2);
                break;
                
            case WORD:
                write_to_memory(index[info].alu_rst, reg[index[info].rs2], 4);
                break;
                
            case DOUBLEWORD:
                write_to_memory(index[info].alu_rst, reg[index[info].rs2], 8);
                break;
        }
    }
}

void write_back(int index){
#ifdef DEBUG
    cout << "entered write_back()" << endl;
#endif
    
    if(info[index].RegWrite == YES){
        if(info[index].MemtoReg == YES){
            reg[info[index].rd] = info[index].data_from_memory;
        }
        else{
            reg[info[index].rd] = info[index].alu_rst;
        }
    }
}

void simulate()
{
    /* set when to end, the PC of atexit */
    int end=(int)endPC - 8; // the addr of 'ret' in main()
    if (end % 4 == 2){  // wierd actually.
        end -= 2;
    }
#ifdef DEBUG
    cout << "endPC: " << hex << end << endl;
#endif
    
    if(single_step){
        single_step_mode_description();
    }
    
    info[0].PC =  PC_;  // intialized as entry

    while(info[0].PC != end)
    {
        reg[0] = 0; // 一直为零
        
        if (run_til && info[0].PC == pause_addr){
            single_step = true;
            run_til = false;
        }
        
        /* five steps */
        fetch_instruction(0);
        decode(0);
        execute(0);
        memory_read_write(0);
        write_back(0);

#ifdef DEBUG
        print_control_info(0);
#endif
        
        if(exit_flag==1){
            cerr << "Can not simulate system calls." << endl;
            break;
        }
    
        if(single_step){
            cout << "The " << dec << inst_num << " instruction: " << hex << setw(8) << setfill('0') << info[0].inst << endl;
            cout << "PC: " << info[0].PC << endl;
            debug_choices();
        }
        
        /* update CPI accordingly */
        update_cpi(0);
    }
}

int main(int argc, char * argv[])
{
    /* get file name from command */
    if(argc < 2){
        cerr << "Missing operand. Please specify the executable file." << endl;
        return 0;
    }
    else if(argc > 2){
        cerr << "Too many operands. Please specify the executable file only." << endl;
        return 0;
    }
    const char* file_name = argv[1];
    
    /* decide whether to debug step by step */
    while (true){
        char response;
        cout << endl << separator << endl;
        cout << blank << "Enter Debug Mode? (y/n)"<< endl;
        cin >> response;
        fflush(stdin);
        if(response == 'y' || response == 'Y'){
            single_step = true;
            break;
        }
        else if(response == 'n' || response == 'N')
            break;
        else
            cerr << "Can not understand. Please input 'y' or 'n'."<< endl;
    }
    
    /* preparations */
	read_elf(file_name);  // explain the ELF file
    load_memory();
    PC_ = entry; // set the entrance PC
    reg[3]=gp; // set the global pointer
    reg[2]=MAX / 2; // set the stack pointer

    /* start simulation */
    simulate();

    end_description();
    end_check();
    print_cpi();
    
    /* close file */
    fclose(file);
    fclose(ftmp);
    
	return 0;
}
