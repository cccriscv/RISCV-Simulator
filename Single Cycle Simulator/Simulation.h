#include<iostream>
#include<iomanip>
#include<string>
#include<stdio.h>
#include<math.h>
//#include <io.h>
//#include <process.h>
#include<time.h>
#include<stdlib.h>
#include"Reg_def.h"
using namespace std;

#define OP_JAL 111
#define OP_R 51

#define F3_ADD 0
#define F3_MUL 0

#define F7_MSE 1
#define F7_ADD 0

#define OP_I 19
#define F3_ADDI 0

#define OP_SW 35
#define F3_SB 0

#define OP_LW 3
#define F3_LB 0

#define OP_BEQ 99
#define F3_BEQ 0

#define OP_IW 27
#define F3_ADDIW 0

#define OP_RW 59
#define F3_ADDW 0
#define F7_ADDW 0


#define OP_SCALL 115
#define F3_SCALL 0
#define F7_SCALL 0

#define MAX 100000000

//主存
unsigned char memory[MAX]={0};
//寄存器堆
REG reg[32]={0};
//PC
int PC=0;

// whether to debug step by step
bool single_step = false;

// whether to run until a specific instruction
bool run_til = false;
unsigned int pause_addr = 0;

//各个指令解析段
unsigned int opcode=0;
unsigned int func3=0,func7=0;
int shamt=0;
int rs1=0,rs2=0,rd=0;
unsigned int imm12=0;
unsigned int imm20=0;
unsigned int imm7=0;
unsigned int imm5=0;
unsigned long long imm=0; // the sign-extended immediate number

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
void fetch_instruction();
void decode();
void execute_R();
void execute_I();
void execute_S();
void execute_U();
void execute_SB();
void execute_UJ();

unsigned int getbit(unsigned inst,int s,int e);
unsigned long long ext_signed(unsigned int src,int bit);
unsigned char read_memory_byte(unsigned int start);
unsigned short read_memory_half(unsigned int start);
unsigned int read_memory_word(unsigned int start);
unsigned long long read_memory_doubleword(unsigned int start);
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
unsigned char read_memory_byte(unsigned int start){
    unsigned char rst = 0;
    unsigned char tmp;
    for (int i = 0; i < 1; ++i){
        tmp = memory[start + i];
        rst |= tmp;
    }
    return rst;
}

/* read 2 bytes from 'start', little-endian */
unsigned short read_memory_half(unsigned int start){
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
unsigned int read_memory_word(unsigned int start){
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
unsigned long long read_memory_doubleword(unsigned int start){
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
