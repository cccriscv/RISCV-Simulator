# RISCV Simulator

## Description
This project is a lab for Architecture Design, PKU.

### What's this project about?
The simulator takes the executable file(compiled by riscv64-unknown-elf-gcc) as input. It reads the elf-related info such as:
- ELF Header
- Section Headers
- Program Headers
- Symbol Table
.data, .sdata, .text are loaded into memory during the process.
The simulator then simulates the executing process of instructions in main().

User may enter single step debug mode and check register value and memory.

### Requirements
- Unix-like OS
- [RISCV-Toolchain](https://github.com/riscv/riscv-tools)
    - riscv64-unknown-elf-gcc
    - riscv64-unknown-elf-readelf
    - riscv64-unknown-elf-objdump

### A few commands...
Here lists some of the commands that might help.
- compile target C program
    >  riscv64-unknown-elf-gcc -Wa,-march=rv64imf -o a.out xxx.c
- check elf file of target C program
    > riscv64-unkown-elf-readelf a.out > xxx.readelf
- check objdump of target C program
    > riscv64-unknown-elf-objdump -S a.out > xxx.objdump

## Contents
This project realized three versions of RISCV Simulator:
- [Single Cycle Simulator](https://github.com/VegB/RISCV-Simulator/tree/master/Single%20Cycle%20Simulator)
- [Multiple Cycle Simulator](https://github.com/VegB/RISCV-Simulator/tree/master/Multiple%20Cycle%20Simulator)
- [Pipeline Simulator](https://github.com/VegB/RISCV-Simulator/tree/master/Pipeline%20Simulator)

