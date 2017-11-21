# Pipeline Simulator

## Description
This is an implementation of multiple cycle simulator.

### Five Stages
Each instruction will go through the following five stage:
- Instruction Fetch
- Decode
- Execute
- Memory Read/Write
- Write Back


### Data Flow
The data flow in this pipeline simulator is shown below.

![](https://github.com/VegB/RISCV-Simulator/blob/master/img/pipeline.png)


Yellow blocks are control signals.
Gray blocks for selectors and orange blocks stands for memory/register.


## How to run
To run this version of simulator, just type in the following instructions:
> sh sim.sh xxx.c

xxx.c is a C program specified by users.

Notice that the simulator can not handle system calls and encountering such instructions might cause the simulator to abort.

