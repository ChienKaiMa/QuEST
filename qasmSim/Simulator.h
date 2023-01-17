#ifndef _SIMULATOR_H_
#define _SIMULATOR_H_

#include <iostream>
#include <stdio.h> // FILE
#include <unordered_map>
#include <sys/time.h> //estimate time
#include <fstream> //fstream
#include <sstream> // int to string
#include <cstdlib> //atoi
#include <string> //string
#include <sstream>
#include <random>
#include <cmath>
#include <vector>
#include "QuEST.h"

class Simulator
{
public:
    // constructor and destructor
    Simulator(int type, int nshots, int seed) : 
    n(0), w(4), k(0), shift(0), error(0), 
    normalize_factor(1), gatecount(0), isMeasure(0), measured_qubits(0), shots(nshots)
    , sim_type(type), statevector("null"), gen(std::default_random_engine(seed)){
    }
    Simulator(int nshots, int seed) : 
    n(0), w(4), k(0), shift(0), error(0), 
    normalize_factor(1), gatecount(0), isMeasure(0), measured_qubits(0), shots(nshots)
    , sim_type(0), statevector("null"), gen(std::default_random_engine(seed)){
    }
    ~Simulator()  { 
        clear();
    }

    /* measurement */
    void measurement();
    void getStatevector();

    /* simulation */
    void init_simulator(int n);
    void sim_qasm_file(std::string qasm);
    void sim_qasm(std::string qasm);
    void print_results();

    /* misc */
    void reorder();
    void decode_entries();
    void print_info(double runtime, size_t memPeak);
    int noise_option;
    double noise_prob;
private:
    Qureg qubits; // Qubits in QuEST
    int n; // # of qubits
    int w; // # of integers
    int k; // k in algebraic representation
    int shift; // # of right shifts
    double error;
    double normalize_factor; // normalization factor used in measurement
    unsigned long gatecount;

    bool isMeasure;
    std::vector<int> measured_qubits; // i-th element is the i-th qubit measured

    int shots;
    int sim_type; // 0: statevector, 1: measure
    int *measured_qubits_to_clbits; // -1 if not measured
    std::string measure_outcome;
    
    std::string statevector;
    std::default_random_engine gen; // random generator
    
    std::unordered_map<std::string, int> state_count;
    std::string run_output; // output string for Qiskit


    /* measurement */
    double measure_probability(int kd2, int nVar, int nAnci_fourInt, int edge);
    void measure_one(int position, int kd2, double H_factor, int nVar, int nAnci_fourInt, std::string *outcome);

    // Clean up Simulator
    void clear() {
        delete [] measured_qubits_to_clbits;
        measured_qubits.clear();
        measure_outcome.clear();
        state_count.clear();
    };
};

#endif
