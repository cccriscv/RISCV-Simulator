g++ Read_Elf.cpp Simulation.cpp -o sim
riscv64-unknown-elf-gcc -Wa,-march=rv64imf -o a.out $1
./sim a.out
