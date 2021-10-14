# QASM file simulation
This folder implements simulation for QASM files.
Different from [QASMParser](https://github.com/oerc0122/QASMParser),
which outputs the circuit description in C language,
requiring build for every new circuit,
this is more convenient if you are to simulate a lot of different circuits.

## Caution
1. This simulator does not guarantee support for every function in QASM file.  You should check if you find that the result is strange.<br>
2. The source codes are tested with QuEST v3.4.1 release with [GRCS benchmarks](https://github.com/sboixo/GRCS) and simulate up to 64 qubits.
3. If the simulator is unable to allocate memory when the qubit number is under 64, you can try increasing `env.numRanks`.

## Usage
Build the binary `QuEST_qasm_sim`:
```
make
```
Execute the binary:
```
./QuEST_qasm_sim --sim_qasm=[filename] --type=0 --print_info
```
To see more options:
```
./QuEST_qasm_sim -h
```

## References
`cxxopts.hpp` is borrowed from [https://github.com/jarro2783/cxxopts/](https://github.com/jarro2783/cxxopts/) <br>
`memory_usage.cpp` is borrowed from [https://github.com/NTU-ALComLab/SliQSim](https://github.com/NTU-ALComLab/SliQSim) <br>
`util_sim.h` and `Simulator.h` are modified from source codes of [https://github.com/NTU-ALComLab/SliQSim](https://github.com/NTU-ALComLab/SliQSim).  Future fixes and code refactoring are required.
