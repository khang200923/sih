#include <iostream>
#include <cstdint>
#include <vector>
#include <sstream>
#include <random>
#include <cmath>
#include <array>

using machstream = std::vector<uint16_t>;
using memorytape = std::vector<uint16_t>;

std::uniform_real_distribution<double> dis(0.0, 1.0);
const std::size_t MEMORY_TAPES = 8;
const uint8_t CODE_PART_DIVISION = 32;
const uint8_t CODE_OP_AMOUNT = 6;

struct Machine {
    uint16_t length;
    std::array<uint8_t, 65536> code;
    //For each element in code: (we call it ins)
    //ins // CODE_PART_DIVISION = op
    //ins % CODE_PART_DIVISION = val
    //op=0: add pointed loc in memory with val
    //op=1: sub pointed loc in memory with val
    //op=2: move loc pointer to right
    //op=3: move loc pointer to left
    //op=4: move to another tape
    //op=5: output the current loc
    std::array<uint16_t, 65536> zeroRedirect;
};

struct RunningMachine {
    Machine machine;
    uint16_t tapelength;
    std::array<memorytape, MEMORY_TAPES> memories;
    std::array<uint16_t, MEMORY_TAPES> pointers;
    uint8_t tapepointer;
    uint16_t codepointer;
    machstream output;
    bool halt;
};

template<typename T>
T random_with_bias(T start_value, T end_value, double bias_factor = 0.6) {
    std::random_device rd;
    std::mt19937 gen(rd());

    double weighted_choice = 1 - pow(dis(gen), bias_factor);
    return start_value + weighted_choice * (end_value - start_value);
}

Machine generateRandomMachine(uint16_t prog_len_limit) {
    Machine machine;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> length_dist(0, 65535);
    uint16_t max_length = prog_len_limit;
    double log_max_length = std::log(max_length + 1);
    double log_length = std::exp(length_dist(gen) * std::log(1 + log_max_length) / std::numeric_limits<uint16_t>::max()) - 1;
    machine.length = static_cast<uint16_t>(log_length);

    std::uniform_int_distribution<uint8_t> code_dist(0, std::numeric_limits<uint8_t>::max());

    for (int i = 0; i < machine.length; ++i) {
        machine.code[i] = code_dist(gen);
        if (dis(gen) < 0.2) {
            machine.zeroRedirect[i] = i + ((dis(gen) > 0) ? 1 : -1) * pow(2, random_with_bias<uint16_t>(0, 15));
        } else {
            machine.zeroRedirect[i] = i;
        };
    }

    return machine;
}

memorytape initiateTape(uint16_t length) {
    memorytape tape = memorytape();
    for (int i = 0; i < length; ++i) {
        tape.push_back(0);
    }

    return tape;
}

RunningMachine initiateMachine(Machine machine, uint16_t memory_len) {
    RunningMachine result;
    result.machine = machine;
    result.tapelength = memory_len;
    for (int i = 0; i < MEMORY_TAPES; ++i) {
        result.memories[i] = initiateTape(memory_len);
    }
    result.halt = false;

    return result;
}

void runTick(RunningMachine& runmachine) {
    if (runmachine.halt) {
        return;
    }

    Machine& machine = runmachine.machine;

    //get from code pointer and tape pointer
    uint8_t& codePiece = machine.code[runmachine.codepointer];
    uint16_t& zeroRedirect = machine.zeroRedirect[runmachine.codepointer];
    memorytape& currentTape = runmachine.memories[runmachine.tapepointer];
    uint16_t& currentPointer = runmachine.pointers[runmachine.tapepointer];
    uint16_t& currentLoc = currentTape[currentPointer];

    //split into op and val
    uint8_t op = (codePiece / CODE_PART_DIVISION) % CODE_OP_AMOUNT;
    uint8_t val = codePiece % CODE_PART_DIVISION;

    //do some computation
    if (op == 0) {
        currentLoc += val;
    } else if (op == 1) {
        currentLoc -= val;
    } else if (op == 2) {
        currentPointer += val;
        currentPointer %= machine.length;
    } else if (op == 3) {
        currentPointer -= val;
        currentPointer %= machine.length;
    } else if (op == 4) {
        runmachine.tapepointer = val / (CODE_PART_DIVISION / MEMORY_TAPES);
    } else if (op == 5) {
        runmachine.output.push_back(currentLoc);
    }

    runmachine.codepointer += 1;

    if (runmachine.codepointer >= machine.code.size()) {
        runmachine.halt = true;
    }
}

machstream roll(uint16_t prog_len_limit, uint16_t memory_len, uint16_t exec_limit) {
    Machine machine = generateRandomMachine(prog_len_limit);
    RunningMachine runmachine = initiateMachine(machine, memory_len);

    for (int i = 0; i < exec_limit; ++i) {
        runTick(runmachine);
        if (runmachine.halt) {
            break;
        }
    }

    return runmachine.output;
}

template<typename InputStream>
machstream parseInput(InputStream& input) {
    machstream result;
    uint16_t num;

    while (input >> num) {
        result.push_back(num);
    }

    if (!input.eof()) {
        throw std::invalid_argument("Invalid input: must be array of integers in the range 0-65535.");
    }

    return result;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv, argv + argc);

    uint16_t exec_limit = 500;
    uint16_t prog_len_limit = 100;
    uint16_t memory_len = 100;
    uint16_t search_depth = 10000;

    machstream target;

    for (int i = 1; i < argc; ++i) {
        std::cout << ((args[i] == "-o") || (args[i] == "--options")) << std::endl;
        std::cout << (args[i] == "-o") << std::endl;
        if ((args[i] == "-o") || (args[i] == "--options")) {
            if (i + 4 >= argc) {
                std::cerr << "Invalid option flag";
                return 1;
            }
            ++i;
            if (args[i] != "d") {
                exec_limit = std::stoi(args[i]);
            }
            ++i;
            if (args[i] != "d") {
                prog_len_limit = std::stoi(args[i]);
            }
            ++i;
            if (args[i] != "d") {
                memory_len = std::stoi(args[i]);
            }
            ++i;
            if (args[i] != "d") {
                search_depth = std::stoi(args[i]);
            }
        }
        if (args[i] == "-c" or args[i] == "--compute") {
            if (i + 1 < argc) {
                for (int j = ++i; j < argc; ++j) {
                    target.push_back(std::stoi(args[i]));
                }
                break;
            }

            try {
                target = parseInput(std::cin);
            } catch (const std::invalid_argument& e) {
                std::cerr << "Invalid input: must be array of integers in the range 0-65535." << std::endl;
                return 1;
            }
        }
    }

    std::cout << "okay i am fine" << std::endl;
    std::cout << exec_limit << std::endl;
    std::cout << prog_len_limit << std::endl;
    std::cout << memory_len << std::endl;
    std::cout << search_depth << std::endl;

    return 0;
}