#include <iostream>
#include <stdio.h> // FILE
#include <cassert>
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
#include <boost/math/constants/constants.hpp>

#include "cxxopts.hpp"
#include "QuEST.h"
#include "Simulator.h" // Modified from YH's file
#include "util_sim.h" // YH's files

using namespace std;

#define pi (boost::math::constants::pi<long double>())

int main(int argc, char **argv)
{
    cxxopts::Options options("Options", "QC simulation options");
    options.add_options()
        ("h, help", "produce help message")
        ("s, sim_qasm", "simulate qasm file string", cxxopts::value<std::string>()->implicit_value(""))
        ("d, seed", "seed for random number generator", cxxopts::value<unsigned int>()->implicit_value("1"))
        ("p, print_info", "print simulation statistics such as runtime, memory, etc.")
        ("t, type", "the simulation type being executed.\n"
                    "0: weak simulation (default option) where the sampled outcome(s) will be provided after the simulation. "
                    "The number of outcomes being sampled can be set by argument \"shots\" (1 by default).\n"
                    "1: strong simulation where the resulting state vector will be shown after the simulation. "
                    "Note that this option should be avoided if the number of qubits is large since it will result in extremely long runtime.", cxxopts::value<unsigned int>()->default_value("0"))
        ("b, shots",   "the number of outcomes being sampled, " 
                    "this argument is only used when the " 
                    "simulation type is set to \"weak\".", cxxopts::value<unsigned int>()->default_value("1"))
    ;

    auto vm = options.parse(argc, argv);

    if (vm.count("help"))
    {
        std::cout << options.help() << "\n";
        exit(0);
    }
    
    struct timeval t1, t2;
    double elapsedTime;

    int type = vm["type"].as<unsigned int>(), shots = vm["shots"].as<unsigned int>();
    if (type == 1)
    {
        shots = 1;
    }
    else
    {
        type = 0;
    }
    std::random_device rd;
    unsigned int seed;
    if (vm.count("seed")) 
		seed = vm["seed"].as<unsigned int>();
    else
        seed = rd();

    // start timer
    gettimeofday(&t1, NULL);

    assert(shots > 0);

    Simulator simulator = Simulator(type, shots, seed);
    if (vm.count("sim_qasm"))
    {
        // read in file into a string
        std::stringstream strStream;
        if (vm["sim_qasm"].as<std::string>() == "")
        {
            strStream << std::cin.rdbuf();
        }
        else
        {
            std::ifstream inFile;
            inFile.open(vm["sim_qasm"].as<std::string>()); //open the input file
            strStream << inFile.rdbuf(); //read the file
        }
        
        std::string inFile_str = strStream.str(); //str holds the content of the file
        simulator.sim_qasm(inFile_str);
    }

    //end timer
    gettimeofday(&t2, NULL);
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;

    double runtime = elapsedTime / 1000;
    size_t memPeak = getPeakRSS();
    if (vm.count("print_info"))
        simulator.print_info(runtime, memPeak);

    return 0;
}

void Simulator::sim_qasm(std::string qasm)
{
    sim_qasm_file(qasm); // simulate
/*
    // measure based on simulator type
    if (sim_type == 0) // weak
    {
        if (isMeasure == 0)
        {
            std::string noMeasureBasis;
            for (int i = 0; i < n; i++)
                noMeasureBasis += '0';
            state_count[noMeasureBasis] = shots;
        }
        else
            measurement();
    }
    else if (sim_type == 1) // strong
    {
        if (isMeasure == 0)
        {
            std::string noMeasureBasis;
            for (int i = 0; i < n; i++)
                noMeasureBasis += '0';
            state_count[noMeasureBasis] = 1;
        }
        else
            measurement();
        //getStatevector(); // TODO
    }
    */
    print_results();
}

void Simulator::sim_qasm_file(std::string qasm)
{
    std::string inStr;
    std::stringstream inFile_ss(qasm);
    QuESTEnv env = createQuESTEnv();
    //env.numRanks = 67108864;
    // 36:32, 42:2048, 49:262144, 56:67108864
    Qureg qubits;
    ComplexMatrix4 u = {
        .real={
            {1, 0, 0, 0},
            {0, 0, 1, 0},
            {0, 1, 0, 0},
            {0, 0, 0, 1}
        }
    };

    while (getline(inFile_ss, inStr))
    {
        inStr = inStr.substr(0, inStr.find("//"));
        if (inStr.find_first_not_of("\t\n ") != std::string::npos)
        {
            std::stringstream inStr_ss(inStr);
            getline(inStr_ss, inStr, ' ');
            if (inStr == "qreg")
            {
                getline(inStr_ss, inStr, '[');
                getline(inStr_ss, inStr, ']');
                qubits = createQureg(stoi(inStr), env);
                initZeroState(qubits);

                /*
                 * REPORT SYSTEM AND ENVIRONMENT
                 */
                printf("\nThis is our environment:\n");
                reportQuregParams(qubits);
                reportQuESTEnv(env);
            }
            else if (inStr == "creg"){;}
            else if (inStr == "OPENQASM"){;}
            else if (inStr == "include"){;}
            else if (inStr == "measure")
            {
                isMeasure = 1;
                getline(inStr_ss, inStr, '[');
                getline(inStr_ss, inStr, ']');
                int qIndex = stoi(inStr);
                getline(inStr_ss, inStr, '[');
                getline(inStr_ss, inStr, ']');
                #ifdef DEBUG
                std::cout << "Note: QuEST does not provide classical bits";
                std::cout << "Measure " << qIndex << ", (" << stoi(inStr) << ")\n";
                #endif
                int result = measure(qubits, qIndex);
                std::cout << result;
                #ifdef DEBUG
                std::cout << "Qubit " << qIndex << " is " << result << ".\n";
                #endif
            }
            else
            {
                if (inStr == "x")
                {
                    getline(inStr_ss, inStr, '[');
                    getline(inStr_ss, inStr, ']');
                    #ifndef DEBUG
                    std::cout << "x " << inStr << "\n";
                    #endif
                    pauliX(qubits, stoi(inStr));
                }
                else if (inStr == "y")
                {
                    getline(inStr_ss, inStr, '[');
                    getline(inStr_ss, inStr, ']');
                    #ifndef DEBUG
                    std::cout << "y " << inStr << "\n";
                    #endif
                    pauliY(qubits, stoi(inStr));
                }
                else if (inStr == "z")
                {
                    std::vector<int> iqubit(1);
                    getline(inStr_ss, inStr, '[');
                    getline(inStr_ss, inStr, ']');
                    iqubit[0] = stoi(inStr);
                    #ifndef DEBUG
                    std::cout << "z " << inStr << "\n";
                    #endif
                    pauliZ(qubits, stoi(inStr));
                    iqubit.clear();
                }
                else if (inStr == "h")
                {
                    getline(inStr_ss, inStr, '[');
                    getline(inStr_ss, inStr, ']');
                    #ifndef DEBUG
                    std::cout << "h " << inStr << "\n";
                    #endif
                    hadamard(qubits, stoi(inStr));
                }
                else if (inStr == "s")
                {
                    getline(inStr_ss, inStr, '[');
                    getline(inStr_ss, inStr, ']');
                    int iqubit = stoi(inStr);
                    #ifndef DEBUG
                    std::cout << "s " << inStr << "\n";
                    #endif
                    sGate(qubits, iqubit);
                }
                else if (inStr == "sdg")
                {
                    getline(inStr_ss, inStr, '[');
                    getline(inStr_ss, inStr, ']');
                    int iqubit = stoi(inStr);
                    #ifndef DEBUG
                    std::cout << "sdg " << inStr << "\n";
                    #endif
                    phaseShift(qubits, iqubit, -pi/2);
                }
                else if (inStr == "t")
                {
                    getline(inStr_ss, inStr, '[');
                    getline(inStr_ss, inStr, ']');
                    int iqubit = stoi(inStr);
                    #ifndef DEBUG
                    std::cout << "t " << inStr << "\n";
                    #endif
                    tGate(qubits, iqubit);
                }
                else if (inStr == "tdg")
                {
                    getline(inStr_ss, inStr, '[');
                    getline(inStr_ss, inStr, ']');
                    int iqubit = stoi(inStr);
                    #ifndef DEBUG
                    std::cout << "tdg " << inStr << "\n";
                    #endif
                    phaseShift(qubits, iqubit, -pi/4);
                }
                else if (inStr == "rx(pi/2)")
                {
                    getline(inStr_ss, inStr, '[');
                    getline(inStr_ss, inStr, ']');
                    #ifndef DEBUG
                    std::cout << "rx(pi/2) " << inStr << "\n";
                    #endif
                    rotateX(qubits, stoi(inStr), pi/2);
                }
                else if (inStr == "ry(pi/2)")
                {
                    getline(inStr_ss, inStr, '[');
                    getline(inStr_ss, inStr, ']');
                    #ifndef DEBUG
                    std::cout << "ry(pi/2) " << inStr << "\n";
                    #endif
                    rotateY(qubits, stoi(inStr), pi/2);
                }
                else if (inStr == "cx")
                {
                    std::vector<int> cont(1);
                    std::vector<int> ncont(0);
                    int targ;
                    getline(inStr_ss, inStr, '[');
                    getline(inStr_ss, inStr, ']');
                    cont[0] = stoi(inStr);
                    getline(inStr_ss, inStr, '[');
                    getline(inStr_ss, inStr, ']');
                    targ = stoi(inStr);
                    #ifndef DEBUG
                    std::cout << "cx " << targ << ", " << cont[0] << "\n";
                    #endif
                    controlledNot(qubits, cont[0], targ);
                    cont.clear();
                    ncont.clear();
                }
                else if (inStr == "cz")
                {
                    std::vector<int> iqubit(2);
                    for (int i = 0; i < 2; i++)
                    {
                        getline(inStr_ss, inStr, '[');
                        getline(inStr_ss, inStr, ']');
                        iqubit[i] = stoi(inStr);
                    }
                    #ifndef DEBUG
                    std::cout << "cz " << iqubit[0] << ", " << iqubit[1] << "\n";
                    #endif
                    controlledPhaseFlip(qubits, iqubit[0], iqubit[1]);
                    iqubit.clear();
                }
                else if (inStr == "swap")
                {
                    int swapA, swapB;
                    for (int i = 0; i < 2; i++)
                    {
                        getline(inStr_ss, inStr, '[');
                        getline(inStr_ss, inStr, ']');
                        if (i == 0)
                            swapA = stoi(inStr);
                        else
                            swapB = stoi(inStr);
                    }
                    #ifndef DEBUG
                    std::cout << "swap " << swapA << ", " << swapB << "\n";
                    #endif
                    swapGate(qubits, swapA, swapB);
                }
                else if (inStr == "cswap")
                {
                    int swapA, swapB;
                    swapA = swapB = 0;
                    std::vector<int> cont(1);
                    for (int i = 0; i < 3; i++)
                    {
                        getline(inStr_ss, inStr, '[');
                        getline(inStr_ss, inStr, ']');
                        if (i == 0)
                            cont[i] = stoi(inStr);
                        else if (i == 1)
                            swapA = stoi(inStr);
                        else
                            swapB = stoi(inStr);
                    }
                    #ifndef DEBUG
                    std::cout << "cswap " << swapA << ", " << swapB << ", " << cont[0] << "\n";
                    #endif
                    
                    controlledTwoQubitUnitary(qubits, cont[0], swapA, swapB, u);
                    cont.clear();
                }
                else if (inStr == "ccx" || inStr == "mcx")
                {
                    std::vector<int> cont(0);
                    std::vector<int> ncont(0);
                    int targ;
                    getline(inStr_ss, inStr, '[');
                    while(getline(inStr_ss, inStr, ']'))
                    {
                        cont.push_back(stoi(inStr));
                        getline(inStr_ss, inStr, '[');
                    }
                    targ = cont.back();
                    ncont.push_back(targ);
                    cont.pop_back();
                    #ifndef DEBUG
                    std::cout << "ccx/mcx " << targ << "\n";
                    #endif
                    multiControlledMultiQubitNot(qubits, &cont[0], cont.size(), &ncont[0], 1);
                    cont.clear();
                    ncont.clear();
                }
                else
                {
                    std::cerr << std::endl
                            << "[warning]: Syntax \'" << inStr << "\' is not supported in this simulator. The line is ignored ..." << std::endl;
                }
            }
        }
    }

    /*
     * FREE MEMORY
     */

    destroyQureg(qubits, env); 
    
    /*
     * CLOSE QUEST ENVIRONMET
     * (Required once at end of program)
     */
    destroyQuESTEnv(env);
}

void Simulator::print_results()
{
    // write output string based on state_count and statevector
    std::unordered_map<std::string, int>::iterator it;

    run_output = "{ \"counts\": { ";
    for (it = state_count.begin(); it != state_count.end(); it++)
    {
        if (std::next(it) == state_count.end())
            run_output = run_output + "\"" + it->first + "\": " + std::to_string(it->second);
        else
            run_output = run_output + "\"" + it->first + "\": " + std::to_string(it->second) + ", ";
    }
    run_output = run_output + " }";
    run_output += (statevector != "null") ? ", \"statevector\": " + statevector + " }" : " }";

    std::cout << run_output << std::endl;
}

void Simulator::measurement()
{
    state_count.clear();
    for (int i=0; i<shots; ++i)
    {
        std::string measure_outcome_clbits;
        for (int j=0; j<n; ++j)
        {
            std::cerr << j << "\n";
            char c = measure(qubits, j) + '0';
            std::cerr << c;
            measure_outcome_clbits.push_back(measure(qubits, j));
        }
        auto it = state_count.find(measure_outcome_clbits);
        if (it != state_count.end())
            state_count[measure_outcome_clbits] = it->second + 1;
        else
            state_count[measure_outcome_clbits] = 1;
    }
}


void Simulator::print_info(double runtime, size_t memPeak)
{
    std::cout << "  Runtime: " << runtime << " seconds" << std::endl;
    std::cout << "  Peak memory usage: " << memPeak << " bytes" << std::endl; //unit in bytes
    std::cout << "  #Applied gates: " << gatecount << std::endl;
    std::cout << "  Accuracy loss: " << error << std::endl;
    // std::cout << "  #Integers: " << w << std::endl;
    
    // std::unordered_map<std::string, int>::iterator it;
    // std::cout << "  Measurement: " << std::endl;
    // for(it = state_count.begin(); it != state_count.end(); it++)
    //     std::cout << "      " << it->first << ": " << it->second << std::endl;
}
