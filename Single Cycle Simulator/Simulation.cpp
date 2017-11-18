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
extern unsigned long long gp;
extern unsigned int madr;
extern unsigned int endPC;
extern unsigned int entry;
extern bool single_step;
extern bool run_til;
extern unsigned int pause_addr;
extern FILE *file;
extern FILE *ftmp;

// #define DEBUG
#define NO_INF

//指令运行数
long long inst_num=0;

//系统调用退出指示
int exit_flag=0;

// whether the instruction changed the PC by itself
bool update_PC = false;

//加载代码段
//初始化PC
void load_memory()
{
    /* load .text */
    fseek(file,cadr,SEEK_SET);
    fread(&memory[vadr],1,csize,file);

    /* load .data */
    fseek(file, dadr, SEEK_SET);
    fread(&memory[vdadr], 1, dsize, file);
}

void fetch_instruction(){
    /* put instruction in IF_ID */
    IF_ID.inst = read_memory_word(PC);
    IF_ID.PC = PC;
    // PC += 4;
}

/* decode and execute */
void decode(){
    unsigned int inst = IF_ID.inst;
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
            
            execute_R();
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
            imm12 = getbit(inst, 20, 31);
            imm = ext_signed((unsigned int)imm12, 11);
            func7 = getbit(inst, 25, 31);
            
            execute_I();
            break;
            
            /* S (store) */
        case 0x23:
            imm5 = getbit(inst, 7, 11);
            func3 = getbit(inst, 12, 14);
            rs1 = getbit(inst, 15, 19);
            rs2 = getbit(inst, 20, 24);
            imm7 = getbit(inst, 25, 31);
            tmp = (imm7 << 5) | imm5;
            imm = ext_signed(tmp, 11);
            func7 = getbit(inst, 25, 31);
            
            execute_S();
            break;
            
            /* SB (switch branch) */
        case 0x63:
            func3 = getbit(inst, 12, 14);
            rs1 = getbit(inst, 15, 19);
            rs2 = getbit(inst, 20, 24);
            tmp = (getbit(inst, 8, 11) << 1) | (getbit(inst, 25, 30) << 5) | (getbit(inst, 7, 7) << 11) | (getbit(inst, 31, 31) << 12);
            imm = ext_signed(tmp, 12);
            func7 = getbit(inst, 25, 31);
            
            execute_SB();
            break;
            
            /* U */
        case 0x17:
        case 0x37:
            rd = getbit(inst, 7, 11);
            imm20 = getbit(inst, 12, 31);
            tmp = imm20;// << 12;
            imm = ext_signed(tmp, 31);
            
            execute_U();
            break;
            
            /* UJ */
        case 0x6f:
            rd = getbit(inst, 7, 11);
            tmp = (getbit(inst, 12, 19) << 12) | (getbit(inst, 20, 20) << 11) | (getbit(inst, 21, 30) << 1) | (getbit(inst, 31, 31) << 20);
            imm = ext_signed(tmp, 20);
            func7 = getbit(inst, 25, 31);
            
            execute_UJ();
            break;
            
            /* extended */
        case 0x3b:
            rd = getbit(inst, 7, 11);
            func3 = getbit(inst, 12, 14);
            rs1 = getbit(inst, 15, 19);
            rs2 = getbit(inst, 20, 24);
            func7 = getbit(inst, 25, 31);
            
            execute_R();
            break;
    }
}

void execute_R(){
    switch(opcode){
        /* regular R-type */
        case 0x33:
            switch (func3){
                case 0x0:
                    switch (func7){
                        case 0x00: // add rd, rs1, rs2
                            reg[rd] = reg[rs1] + reg[rs2];
                            break;
                        case 0x01: // mul rd, rs1, rs2
                            reg[rd] = reg[rs1] * reg[rs2];
                            break;
                        case 0x20: // sub rd, rs1, rs2
                            reg[rd] = reg[rs1] - reg[rs2];
                            break;
                    }
                    break;
                    
                case 0x1:
                    switch (func7){
                        case 0x00: // sll rd, rs1, rs2
                            reg[rd] = reg[rs1] << reg[rs2];
                            break;
                        case 0x01: // mulh rd, rs1, rs2
                            long long ah = ((reg[rs1] & 0xffff0000) >> 32) & 0xffff;
                            long long al = reg[rs1] & 0x0000ffff;
                            long long bh = ((reg[rs2] & 0xffff0000) >> 32) & 0xffff;
                            long long bl = reg[rs2] & 0x0000ffff;
                            long long ah_mul_bh = ah * bh;
                            long long ah_mul_bl = ((ah * bl) >> 32) & 0xffff;
                            long long al_mul_bh = ((al * bh) >> 32) & 0xffff;
                            reg[rd] = ah_mul_bh + ah_mul_bl + al_mul_bh;
                            break;
                    }
                    break;
                    
                case 0x2:
                    if(func7 == 0x00){ // slt rd, rs1, rs2
                        reg[rd] = ((long long)reg[rs1] < (long long)reg[rs2]) ? 1: 0;
                    }
                    break;
                    
                case 0x4:
                    switch (func7){
                        case 0x00: // xor rd, rs1, rs2
                            reg[rd] = reg[rs1] ^ reg[rs2];
                            break;
                        case 0x01: // div rd, rs1, rs2
                            reg[rd] = reg[rs1] / reg[rs2];
                            break;
                    }
                    break;
                    
                case 0x5:
                    switch (func7){
                        case 0x00: // srl rd, rs1, rs2
                            reg[rd] = reg[rs1] >> reg[rs2];
                            break;
                        case 0x20: // sra rd, rs1, rs2
                            reg[rd] = reg[rs1] >> reg[rs2];
                            reg[rd] = ext_signed(reg[rd], 63 - (int)reg[rs2]);
                            break;
                    }
                    break;
                    
                case 0x6:
                    switch (func7){
                        case 0x00: // or rd, rs1, rs2
                            reg[rd] = reg[rs1] | reg[rs2];
                            break;
                        case 0x01: // rem rd, rs1, rs2
                            reg[rd] = reg[rs1] % reg[rs2];
                            break;
                    }
                    break;
                    
                case 0x7:
                    if(func7 == 0x00){ // and rd, rs1, rs2
                        reg[rd] = reg[rs1] & reg[rs2];
                    }
                    break;
                    
            }
            break;
        
        /* extended */
        case 0x3b:
            switch (func3){
                case 0x0:
                    if(func7 == 0x00) // addw rd, rs1, rs2
                        reg[rd] = 0xffffffff & (reg[rs1] + reg[rs2]);
                    else if(func7 == 0x20) // subw rd, rs1, rs2
                        reg[rd] = 0xffffffff & (reg[rs1] - reg[rs2]);
                    else if(func7 == 0x01){ // mulw rd, rs1, rs2
#ifdef DEBUG
                        cout << "entered mulw"<< endl;
                        cout << "reg[rs1]: " << hex << reg[rs1]<< endl;
                        cout << "reg[rs2]: " << hex << reg[rs2]<< endl;
                        cout << "reg[rs1] * reg[rs2] " << hex << reg[rs1] * reg[rs2] << endl;
#endif
                        reg[rd] = reg[rs1] * reg[rs2];
                    }
                    break;
                case 0x1: // sllw
                    reg[rd] = 0xffffffff & (reg[rs1] << reg[rs2]);
                    break;
                case 0x4: // divw
                    reg[rd] = 0xffffffff & (reg[rs1] / reg[rs2]);
                case 0x5:
                    if(func7 == 0x00)// srlw rd, rs1, rs2
                        reg[rd] = 0xffffffff & (reg[rs1] >> reg[rs2]);
                    else{ // sraw rd, rs1, rs2
                        reg[rd] = reg[rs1] >> reg[rs2];
                        reg[rd] = 0xffffffff & ext_signed(reg[rd], 63 - (int)reg[rs2]);
                    }
                    break;
            }
    }
    
}

void execute_I(){
    unsigned int tmp;
    switch (opcode) {
        case 0x03:
            switch (func3){
                case 0x0: // lb rd, offset(rs1)
                    tmp = read_memory_byte((int)(reg[rs1] + imm));
                    reg[rd] = ext_signed(tmp, 7);
                    break;
                    
                case 0x1: // lh rd, offset(rs1)
                    tmp = read_memory_half((int)(reg[rs1] + imm));
                    reg[rd] = ext_signed(tmp, 15);
                    break;
                    
                case 0x2: // lw rd, offset(rs1)
#ifdef DEBUG
                    cout << "entered lw"<< endl;
                    cout << "reg[rs1]: " << hex << reg[rs1]<< endl;
                    cout << "imm: " << hex << imm<< endl;
                    cout << "reg[rs1] + imm: " << hex << (int)(reg[rs1] + imm) << endl;
                    tmp = read_memory_word((int)(reg[rs1] + imm));
                    cout << "M[reg[rs1] + imm]: " << hex << tmp << endl;
                    cout << "sign extended: " << hex << ext_signed(tmp, 31) << endl;
#endif
                    tmp = read_memory_word((int)(reg[rs1] + imm));
                    reg[rd] = ext_signed(tmp, 31);
                    break;
                    
                case 0x3: // ld rd, offset(rs1)
                    reg[rd] = read_memory_doubleword((int)(reg[rs1] + imm));
                    break;
            }
            break;
            
        case 0x13:
            switch (func3){
                case 0x0: // addi rd, rs1, imm
                    reg[rd] = reg[rs1] + imm;
                    break;
                    
                case 0x1: // slli rd, rs1, imm
                    if (func7 == 0x00){
                        reg[rd] = reg[rs1] << (imm & 63);
                    }
                    break;
                    
                case 0x2: // slti rd, rs1, imm
                    reg[rd] = (long long)reg[rs1] < (long long)imm? 1: 0;
                    break;
                    
                case 0x4: // xori rd, rs1, imm
                    reg[rd] = reg[rs1] ^ imm;
                    break;
                    
                case 0x5: // srli rd, rs1, imm
                    if (func7 == 0x00){
                        reg[rd] = reg[rs1] >> (imm & 63);
                    }
                    else if (func7 == 0x10){ // srai rd, rs1, imm
                        reg[rd] = ext_signed(reg[rs1], 63 - (int)(imm & 63));
                    }
                    break;
                    
                case 0x6: // ori rd, rs1, imm
                    reg[rd] = reg[rs1] | imm;
                    break;
                    
                case 0x7: // andi rd, rs1, imm
                    reg[rd] = reg[rs1] & imm;
                    break;
            }
            break;
            
        case 0x1B:
            switch (func3){
                case 0x0: // addiw rd, rs1, imm
                    reg[rd] = ext_signed(0xffffffff & (reg[rs1] + imm), 31);
                    break;
                case 0x1: // slliw rd, rs1, imm
#ifdef DEBUG
                    cout << "entered slliw"<< endl;
                    cout << "reg[rs1]: " << hex << reg[rs1]<< endl;
                    cout << "imm: " << hex << imm<< endl;
                    cout << "reg[rs1] * imm: " << hex << (int)(reg[rs1] << imm) << endl;
                    cout << "sign extended: " << hex << ext_signed(0xffffffff & (reg[rs1] << imm), 31) << endl;
#endif
                    reg[rd] = ext_signed(0xffffffff & (reg[rs1] << imm), 31);
                    break;
                case 0x5:
                    // srliw rd, rs1, imm
                    if (getbit(IF_ID.inst, 30, 30) == 0){
#ifdef DEBUG
                        cout << "entered srliw"<< endl;
                        cout << "reg[rs1]: " << hex << reg[rs1]<< endl;
                        cout << "imm: " << hex << imm<< endl;
                        cout << "reg[rs1] >> imm: " << hex << (0xffffffff & (reg[rs1] >> (imm & 63))) << endl;
#endif
                        reg[rd] = 0xffffffff & (reg[rs1] >> (imm & 63));
                    }
                    // sraiw rd, rs1, imm
                    else{
#ifdef DEBUG
                        cout << "entered sraiw"<< endl;
                        cout << "reg[rs1]: " << hex << reg[rs1]<< endl;
                        cout << "imm: " << hex << imm<< endl;
                        cout << "reg[rs1] >> imm: " << hex << (0xffffffff & (ext_signed(reg[rd], 63 - (int)(imm & 63)))) << endl;
#endif
                        reg[rd] = 0xffffffff & (reg[rs1] >> (imm & 63));
                        reg[rd] = 0xffffffff & (ext_signed(reg[rd], 63 - (int)(imm & 63)));
                    }
                    break;
            }
            break;
            
        case 0x67:
            switch (func3){
                case 0x0: // Jalr rd, rs1, imm
                    reg[31] = PC + 4; // use temporaries register in case rs1 == rd
                    PC = reg[rs1] + imm; // the last bit is set to 0
                    reg[rd] = reg[31];
                    update_PC = true;
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
}

void execute_S(){
    switch (opcode){
        case 0x23:
            if(func3 == 0x0){ // sb rs2, offset(rs1)
                write_to_memory(reg[rs1] + imm, reg[rs2], 1);
            }
            else if(func3 == 0x1){ // sh rs2, offset(rs1)
                write_to_memory(reg[rs1] + imm, reg[rs2], 2);
            }
            else if(func3 == 0x2){ // sw rs2, offset(rs1)
#ifdef DEBUG
                cout << "entered sw"<< endl;
                cout << "reg[rs1]: " << hex << reg[rs1]<< endl;
                cout << "imm: " << hex << imm<< endl;
                cout << "reg[rs1] + imm: " << hex <<reg[rs1] + imm<< endl;
                cout << "reg[rs2]: " << reg[rs2] << endl;
#endif
                write_to_memory(reg[rs1] + imm, reg[rs2], 4);
            }
            else if(func3 == 0x3){ // sd rs2, offset(rs1)
                write_to_memory(reg[rs1] + imm, reg[rs2], 8);
            }
            break;
    }
}

void execute_SB(){
    switch (opcode){
        case 0x63:
            if(func3 == 0x0){ // beq rs1, rs2, offset
                if(reg[rs1] == reg[rs2]){
                    PC = PC + imm;
                    update_PC = true;
                }
            }
            else if(func3 == 0x1){ // bne rs1, rs2, offset
                if(reg[rs1] != reg[rs2]){
                    PC = PC + imm;
                    update_PC = true;
                }
            }
            else if(func3 == 0x4){ // blt rs1, rs2, offset
                if(reg[rs1] < reg[rs2]){
                    PC = PC + imm;
                    update_PC = true;
                }
            }
            else if(func3 == 0x5){ // bge rs1, rs2, offset
                if(reg[rs1] >= reg[rs2]){
                    PC = PC + imm;
                    update_PC = true;
                }
            }
            break;
    }
}

void execute_U(){
    switch (opcode){
        case 0x17: // auipc rd, offset
            reg[rd] = PC + (imm << 12);
            break;
            
        case 0x37: // lui rd, offset
#ifdef DEBUG
            cout << "entered lui"<< endl;
            cout << "imm: " << hex << imm << endl;
            cout << "(imm << 12): " << hex << (imm << 12) << endl;
            cout << "reg[rd]: " << hex << reg[rd] << endl;
#endif
            reg[rd] = imm << 12;
            break;
    }
}

void execute_UJ(){
    switch (opcode){
        case 0x6f: // jal rd, imm
#ifdef DEBUG
//            cout << "original PC: " << hex << PC << endl;
//            cout << "imm: "<< dec << imm << " (dec), "<< hex <<" (hex)"<< imm << endl;
#endif
            reg[rd] = PC + 4;
            PC = PC + imm;
            update_PC = true;
            break;
    }
}

void simulate()
{
    //结束PC的设置
    int end=(int)endPC - 4; // the addr of 'ret' in main()
    
#ifdef DEBUG
    cout << separator << endl;
    cout << "entry: "<< hex << entry << endl;
    cout << "end: "<< hex << end << endl;
    cout << separator << endl;
#endif
    
    if(single_step){
        single_step_mode_description();
        // cout << "Please hit 'g' to start."<< endl;
    }
    
    while(IF_ID.PC != end)
    {
        /* execute */
        fetch_instruction();
        decode();

#ifdef DEBUG
        if (IF_ID.inst == 0 && inst_num != 0){
            cerr << "Something went wrong with the instruction fetched"<< endl;
            break;
        }
        if(inst_num < -1){
            cerr << "probably stuck in endless loops."<< endl;
            break;
        }
        cout << "The " << dec << inst_num << " instruction: " << hex << setw(8) << setfill('0') << IF_ID.inst << endl;
        cout << "PC: " << PC << endl;
        cout << endl;
#endif
 
#ifdef NO_INF
        if(inst_num > 1000){
            //cerr << "probably stuck in endless loops."<< endl;
            break;
        }
#endif
        if(exit_flag==1){
            cerr << "Can not simulate system calls." << endl;
            break;
        }
        
        reg[0] = 0;//一直为零
        
        // Update PC [no pipeline]
        if (!update_PC)
            PC += 4;
        
        if (run_til)
            if (IF_ID.PC == pause_addr){
                single_step = true;
                run_til = false;
            }
        
        if(single_step){
            cout << "The " << dec << inst_num << " instruction: " << hex << setw(8) << setfill('0') << IF_ID.inst << endl;
            cout << "PC: " << PC << endl;
            // cout << "IF_ID.PC: " << IF_ID.PC << endl;
            debug_choices();
        }
        
        update_PC = false;
        inst_num += 1;
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
    
	//解析elf文件
	read_elf(file_name);
	
    //加载内存
    load_memory();

    //设置入口地址
    PC = entry;

    //设置全局数据段地址寄存器
    reg[3]=gp;

    reg[2]=MAX/2;//栈基址 （sp寄存器）

    simulate();

    end_description();
    end_check();
    
    // close file
    fclose(file);
    fclose(ftmp);
    
	return 0;
}
