#include<iostream>
#include<iomanip>
#include<string>
#include<stdio.h>
#include<math.h>
#include<time.h>
#include<stdlib.h>
#include"Reg_def.h"
using namespace std;

#define MAX 100000000

/* Memory */
unsigned char memory[MAX]={0};
/* Registers*/
REG reg[32]={0};
/* PC */
int PC_=0;

/* whether to debug step by step */
bool single_step = false;

/* whether to run until a specific instruction */
bool run_til = false;
unsigned int pause_addr = 0;

/* parts of instruction [Decode use only] */
unsigned int opcode=0;
unsigned int func3=0,func7=0;
int shamt=0;
int rs1=0,rs2=0,rd=0;
unsigned int imm12=0;
unsigned int imm20=0;
unsigned int imm7=0;
unsigned int imm5=0;
unsigned long long imm=0; // the sign-extended immediate number

/* Control signal [Decode use only] */
char MemRead;
char MemWrite;
char MemtoReg;
char RegWrite;
char ALUSrcA;
char ALUSrcB;
char ALUOp;
char PCSrc;
char ExtOp;
char data_type;

/* inner registers */
unsigned long long alu_rst;  // might also be data_to_reg [Execute use only]
unsigned long long data_from_memory;  // data read out from M[alu_rst], [Memory stage use only]

/* CPI related */
long long inst_num = 0;  // number of instructions executed
long long cycle_num = 0;  // number of clock cycles

int exit_flag=0;  // whether there's a system call

string reg_names[32] = {"r0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0/fp", "s1", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
    "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "t3", "t4", "t5", "t6"};
string separator = "=================================================";
string blank = "         ";

/* related functions */
extern unsigned int find_addr(string);
void load_memory();
void simulate();

void fetch_instruction(int);
void decode(int);
void fill_control_R(int);
void fill_control_I(int);
void fill_control_S(int);
void fill_control_U(int);
void fill_control_SB(int);
void fill_control_UJ(int);
void execute(int);
void memory_read_write(int);
void write_back(int);

unsigned int getbit(unsigned inst,int s,int e);
unsigned long long ext_signed(unsigned int src,int bit);
unsigned char read_memory_byte(unsigned long long start);
unsigned short read_memory_half(unsigned long long start);
unsigned int read_memory_word(unsigned long long start);
unsigned long long read_memory_doubleword(unsigned long long start);
unsigned char get_byte(unsigned long long reg_info, int ind);
void write_to_memory(unsigned long long start, unsigned long long reg_info, int num);
void print_register();
void print_specific_register();
void print_memory();
void run_until();
void check_var_addr();
void single_step_mode_description();
bool debug_choices();
void end_check();
void end_description();
void print_control_info(int);
void update_cpi(int);
void print_cpi();

/* signed extension of byte/half/word to doubleword
 bit: the index of sign bit(for byte, bit = 7)
 */
unsigned int getbit(unsigned inst,int s,int e)
{
    unsigned int mask = 0xffffffff;
    mask = (mask >> s) << s;
    mask = (mask << (31 - e)) >> (31 - e);
    unsigned int rst = inst & mask;
    rst = rst >> s;
    return rst;
}

unsigned long long ext_signed(unsigned int src,int bit)
{
    unsigned long long rst = src, mask = 0xffffffffffffffff << (bit + 1);
    if (0x1 & (src >> bit)) // sign bit = 1
        rst |= mask;
    return rst;
}


/* read 1 byte from 'start', little-endian */
unsigned char read_memory_byte(unsigned long long start){
    unsigned char rst = 0;
    unsigned char tmp;
    for (int i = 0; i < 1; ++i){
        tmp = memory[start + i];
        rst |= tmp;
    }
    return rst;
}

/* read 2 bytes from 'start', little-endian */
unsigned short read_memory_half(unsigned long long start){
    unsigned short rst = 0;
    unsigned short tmp;
    for (int i = 0; i < 2; ++i){
        tmp = (unsigned short)memory[start + (1 - i)];
        tmp <<= 8 * (1 - i);
        rst |= tmp;
    }
    return rst;
}

/* read 4 bytes from 'start', little-endian */
unsigned int read_memory_word(unsigned long long start){
    unsigned int rst = 0;
    unsigned int tmp;
    for (int i = 0; i < 4; ++i){
        tmp = (unsigned int)memory[start + (3 - i)];
        tmp <<= 8 * (3 - i);
        rst |= tmp;
    }
    return rst;
}

/* read 8 bytes from 'start',  little-endian*/
unsigned long long read_memory_doubleword(unsigned long long start){
    unsigned long long rst = 0;
    unsigned long long tmp;
    for (int i = 0; i < 8; ++i){
        tmp = (unsigned long long)memory[start + (7 - i)];
        tmp <<= 8 * (7 - i);
        rst |= tmp;
    }
    return rst;
}

/* get the 'ind' byte (counting from right to left) from reg_info */
unsigned char get_byte(unsigned long long reg_info, int ind){
    unsigned char rst;
    rst = (unsigned char)((reg_info >> (ind << 3)) & 0xff);
    return rst;
}

/* write the lower 'num' bytes of reg_info to memory from 'start'
 little-endian */
void write_to_memory(unsigned long long start, unsigned long long reg_info, int num){
    for (int i = 0; i < num; ++i){
        memory[start + i] = get_byte(reg_info, i);
    }
}

/* print all registers */
void print_register(){
    cout << blank << "Check all registers." << endl;
    for(int i = 0;i < 32; ++i){
        cout << setfill(' ') << setw(10) << reg_names[i] << ": ";
        cout << setfill('0') << setw(16) << hex << reg[i]<< endl;
    }
    cout << separator<< endl;
}

void print_specific_register(){
    string response;
    bool matched = false;
    
    while (true){
        cout << "Please input the name of the register. For instance, \"ra\", \"sp\", \"a6\", etc."<< endl;
        cin >> response;
        fflush(stdin);
        for(int i = 0; i < 32; ++i){
            if(reg_names[i] == response){
                matched = true;
                cout << setfill(' ') << setw(10) << reg_names[i] << ": ";
                cout << setfill('0') << setw(16) << hex << reg[i]<< endl;
                break;
            }
            else if (response == "fp" || response == "s0"){
                matched = true;
                cout << setfill(' ') << setw(10) << reg_names[8] << ": ";
                cout << setfill('0') << setw(20) << hex << reg[8]<< endl;
                break;
            }
        }
        if(matched)
            break;
        cerr << "Invalid register name: "<< response << endl;
    }
}

/* read 'num' bytes from 'start' consecutively */
void print_memory(){
    int start;
    int num;
    
    while (true){
        cout << "Please input the starting addr(hex): ";
        cin >> hex >> start;
        fflush(stdin);
        if(start < 0)
            cerr << "Invalid address."<< endl;
        else
            break;
    }
    while (true){
        cout << "Please input the number of bytes you want to check: ";
        cin >> dec >> num;
        fflush(stdin);
        if(num <= 0)
            cerr << "Invalid byte number."<< endl;
        else
            break;
    }
    for(int i = 0; i < num; ++i)
        cout << setw(2) << setfill('0') << hex << (int)memory[start + i] << " ";
    cout << endl;
    cout << separator << endl;
}

void run_until(){
    unsigned int start;
    run_til = true;
    single_step = false;
    
    while (true){
        cout << "Please input the address(hex) of the instruction: ";
        cin >> hex >> start;
        fflush(stdin);
        if(start % 2)
            cerr << start << " does not seem to be an valid addr for an instruction." << endl;
        else{
            pause_addr = start;
            break;
        }
    }
}

/* single step mode description */
void single_step_mode_description(){
    cout << endl << blank << "Entered single step mode." << endl << endl;
    while (true){
        cout << blank << "---------Descriptions----------"<< endl;
        cout << blank << "(1) Start Debug "<< endl;
        cout << blank << "    Hit key 'g'."<< endl;
        cout << blank << "(2) Next step"<< endl;
        cout << blank << "    Hit key 'g'."<< endl;
        cout << blank << "(3) Check Registers"<< endl;
        cout << blank << "    - All Registers"<< endl;
        cout << blank << "        Enter \"ar\"."<< endl;
        cout << blank << "    - A Specific Register" << endl;
        cout << blank << "        Hit key 'r', then follow instructions."<< endl;
        cout << blank << "(4) Check Memory" << endl;
        cout << blank << "    Hit key 'm', then follow instructions." << endl;
        cout << blank << "(5) Show address of a global variable" << endl;
        cout << blank << "    Hit 'v', then follow instructions." << endl;
        cout << blank << "(6) Run until a specific instruction"<<endl;
        cout << blank << "    Enter \"ru\"." << endl;
        cout << blank << "(7) Quit single step mode." << endl;
        cout << blank << "    Hit key 'q'." << endl;
        if(debug_choices())
            break;
    }
    cout << separator<< endl;
}

/* Select debug choice */
bool debug_choices(){
    string response;
    while(true){
        cin >> response;
        fflush(stdin);
        if(response == "g")
            break;
        else if(response == "ar")
            print_register();
        else if(response == "r")
            print_specific_register();
        else if(response == "m")
            print_memory();
        else if(response == "h" || response == "help")
            single_step_mode_description();
        else if (response == "q"){
            cout << blank << "Quit single step mode." << endl;
            single_step = false;
            cout << separator << endl;
            break;
        }
        else if (response == "ru"){
            run_until();
            break;
        }
        else if(response == "v")
            check_var_addr();
        else{
            cerr << "Can not understande '" << response << "'.\nPlease follow instructions."<< endl;
            single_step_mode_description();
        }
    }
    return true;
}

void end_description(){
    cout << separator << endl << endl;
    cout << blank << "Simulation over!"<<endl;
    cout << endl << separator << endl;
    cout << blank << "---------Checkings----------"<< endl;
    cout << blank << "(1) Check Registers"<< endl;
    cout << blank << "    - All Registers"<< endl;
    cout << blank << "        Enter \"ar\"."<< endl;
    cout << blank << "    - A Specific Register" << endl;
    cout << blank << "        Hit key 'r', then follow instructions."<< endl;
    cout << blank << "(2) Check Memory" << endl;
    cout << blank << "    Hit key 'm', then follow instructions." << endl;
    cout << blank << "(3) Show address of a global variable" << endl;
    cout << blank << "    Hit 'v', then follow instructions." << endl;
    cout << blank << "(4) Quit" << endl;
    cout << blank << "    Hit key 'q'." << endl;
    cout << endl << separator << endl;
}

void end_check(){
    string response;
    while (true){
        cin >> response;
        fflush(stdin);
        if(response == "ar")
            print_register();
        else if(response == "r")
            print_specific_register();
        else if(response == "m")
            print_memory();
        else if(response == "q"){
            cout << blank << "Quit from simulation." << endl;
            break;
        }
        else if(response == "v")
            check_var_addr();
        else{
            cerr << "Can not understand '" << response << "'.\nPlease follow instructions."<< endl;
            end_description();
        }
    }
}

void check_var_addr(){
    string var_name;
    cout << "Please input the global variable's name: ";
    cin >> var_name;
    fflush(stdin);
    unsigned int addr_ = find_addr(var_name);
    if(addr_ == (-1))
        cerr << "Can not find a global variable with name: "<< var_name << endl;
    else
        cout << var_name << "'s address(hex) starts from: " << hex << addr_<< endl;
    cout << separator << endl;
}

/* write control signals into info[index] */
void write_control_signal(int index){
    info[index].MemRead = MemRead;
    info[index].MemWrite = MemWrite;
    info[index].MemtoReg = MemtoReg;
    info[index].RegWrite = RegWrite;
    info[index].ALUSrcA = ALUSrcA;
    info[index].ALUSrcB = ALUSrcB;
    info[index].ALUOp = ALUOp;
    info[index].PCSrc = PCSrc;
    info[index].ExtOp = ExtOp;
    info[index].data_type = data_type;
}

/* write instruction related info into info[index] */
void write_inst_info(int index){
    info[index].imm = imm;
    info[index].rs1 = rs1;
    info[index].rs2 = rs2;
    info[index].rd = rd;
}

/* print out control info in info[index] */
void print_control_info(int index){
    string yn_[2] = {"NO", "YES"};
    string ALUOp_[20] = {"ADD", "SUB", "MUL", "DIV", "SLL", "XOR", "SRA", "SRL",
        "OR", "AND", "REM", "EQ", "NEQ", "GT", "LT", "GE", "MULH", "SLT"};
    string ALUSrcA_[5] = {"R_RS1", "PROGRAM_COUNTER", "ZERO"};
    string ALUSrcB_[5] = {"R_RS2", "IMM", "IMM_05", "IMM_SLL_12", "FOUR"};
    string PCSrc_[5] = {"NORMAL", "R_RS1_IMM", "PC_IMM"};
    string data_type_[5] = {"BYTE", "HALF", "WORD", "DOUBLEWORD"};
    
    cout << "MemRead: " << yn_[info[index].MemRead] << endl;
    cout << "MemWrite: " << yn_[info[index].MemWrite] << endl;
    cout << "MemtoReg: " << yn_[info[index].MemtoReg] << endl;
    cout << "RegWrite: " << yn_[info[index].RegWrite] << endl;
    cout << "ALUSrcA: " << ALUSrcA_[info[index].ALUSrcA] << endl;
    cout << "ALUSrcB: " << ALUSrcB_[info[index].ALUSrcB] << endl;
    cout << "ALUOp: " << ALUOp_[info[index].ALUOp] << endl;
    cout << "PCSrc: " << PCSrc_[info[index].PCSrc] << endl;
    cout << "ExtOp: " << yn_[info[index].ExtOp] << endl;
    cout << "data_type: " << data_type_[info[index].data_type] << endl;
    cout << separator << endl;
}

/* update cycle accordingly */
void update_cpi(int index){
    // cout << "update cpi" << endl;
    
    inst_num += 1;
    
    cycle_num += 4;  // IF, DC, M, WB only takes 1 cycle
    
    switch (info[index].ALUOp) {
        case MUL:
            if(info[index].data_type == WORD){
                cout << "32mul"<< endl;
                cycle_num += 1;
            }
            else{
                cout << "mul"<< endl;
                cycle_num += 2;
            }
            break;
            
        case DIV:
        case REM:
            cout << "div"<< endl;
            cycle_num += 40;
            break;
            
        default:
            cycle_num += 1;
            break;
    }
}

void print_cpi(){
    cout << separator << endl;
    cout << "Cycle Count: " << dec << cycle_num << endl;
    cout << "Instruction Count: " << dec << inst_num << endl;
    cout << "CPI = " << (float)cycle_num / (float)inst_num << endl;
}
