#include <iostream>
#include <cstdint>
#include <vector>
#include <sstream>
#include <random>
#include <cmath>
#include <array>
#include <algorithm>
#include <csignal>
#include <memory>

//#define DEBUG_MACHINE_INFO
//#define DEBUG_PREDICT_PROGRESS

using machstream = std::vector<uint16_t>;
using memorytape = std::vector<uint16_t>;

std::uniform_real_distribution<double> dis(0.0, 1.0);
const std::size_t MEMORY_TAPES = 8;
const uint8_t CODE_PART_DIVISION = 32;
const uint8_t CODE_OP_AMOUNT = 6;
std::random_device rd;
std::uint32_t seed = rd();
std::mt19937 gen(seed);

template<typename T>
std::string vectorToString(const std::vector<T>& vec, size_t max = 0) {
    if (max == 0) {max = vec.size();}
    std::ostringstream oss;
    if (!vec.empty()) {
        oss << static_cast<int>(vec[0]);
        for (size_t i = 1; i < max; ++i) {
            oss << " " << static_cast<int>(vec[i]);
        }
    }
    return oss.str();
}

template<typename T, size_t s>
std::string arrayToString(const std::array<T, s>& arr, size_t max = 0) {
    if (max == 0) {max = arr.size();}
    std::ostringstream oss;
    if (!arr.empty()) {
        oss << static_cast<int>(arr[0]);
        for (size_t i = 1; i < max; ++i) {
            oss << " " << static_cast<int>(arr[i]);
        }
    }
    return oss.str();
}

struct Machine {
    uint16_t length;
    //For each element in code: (we call it ins)
    //ins // CODE_PART_DIVISION = op
    //ins % CODE_PART_DIVISION = val
    //op=0: add pointed loc in memory with val
    //op=1: sub pointed loc in memory with val
    //op=2: move loc pointer to right
    //op=3: move loc pointer to left
    //op=4: move to another tape
    //op=5: output the current loc
    std::array<uint8_t, 65536> codeop;
    std::array<uint8_t, 65536> codeval;
    std::array<uint16_t, 65536> zeroRedirect;
    std::array<uint16_t, 65536> redirect;
};

struct RunningMachine {
    Machine machine;
    uint16_t tapelength;
    std::array<memorytape, MEMORY_TAPES> memories;
    std::array<uint16_t, MEMORY_TAPES> pointers;
    uint8_t tapepointer;
    uint16_t codepointer;
    machstream output;

    RunningMachine& operator=(const RunningMachine& other) {
        if (this != &other) { // self-assignment check
            machine = other.machine;
            tapelength = other.tapelength;
            memories = other.memories;
            pointers = other.pointers;
            tapepointer = other.tapepointer;
            codepointer = other.codepointer;
            output = other.output;
        }
        return *this;
    }
};

template<typename T>
T random_with_bias(T start_value, T end_value, double bias_factor = 0.6) {
    double weighted_choice = 1 - pow(dis(gen), bias_factor);
    return start_value + weighted_choice * (end_value - start_value);
}

Machine generateRandomMachine(uint16_t prog_len_limit) {
    Machine machine;

    std::uniform_int_distribution<uint16_t> length_dist(0, 65535);
    uint16_t max_length = prog_len_limit;
    double log_max_length = std::log(max_length + 1);
    double log_length = std::exp(length_dist(gen) * log_max_length / std::numeric_limits<uint16_t>::max()) - 1;
    machine.length = static_cast<uint16_t>(log_length)+1;

    std::uniform_int_distribution<uint8_t> op_dist(0, std::numeric_limits<uint8_t>::max() / CODE_PART_DIVISION);
    std::uniform_int_distribution<uint8_t> val_dist(0, CODE_PART_DIVISION);

    for (uint16_t i = 0; i < machine.length; ++i) {
        machine.codeop[i] = op_dist(gen) % CODE_OP_AMOUNT;
        machine.codeval[i] = random_with_bias<uint8_t>(0, CODE_PART_DIVISION, 0.1);
        if (dis(gen) < 0.2) {
            machine.zeroRedirect[i] = static_cast<uint16_t>(i + 1 + ((dis(gen) > 0) ? 1 : -1) * pow(2, random_with_bias<uint16_t>(0, 15))) % machine.length;
        } else {
            machine.zeroRedirect[i] = i + 1;
        };
        if (dis(gen) < 0.1) {
            machine.redirect[i] = static_cast<uint16_t>(i + 1 + ((dis(gen) > 0) ? 1 : -1) * pow(2, random_with_bias<uint16_t>(0, 15))) % machine.length;
        } else {
            machine.redirect[i] = i + 1;
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
    result.codepointer = 0;
    result.tapepointer = 0;
    for (int i = 0; i < MEMORY_TAPES; ++i) {
        result.memories[i] = initiateTape(memory_len);
        result.pointers[i] = 0;
    }

    return result;
}

void runTick(RunningMachine& runmachine) {
    Machine& machine = runmachine.machine;

    if (runmachine.codepointer >= machine.length) {
        runmachine.codepointer = 0;
    }

    //get from code pointer and tape pointer
    uint8_t op = machine.codeop[runmachine.codepointer];
    uint8_t val = machine.codeval[runmachine.codepointer];
    uint16_t& zeroRedirect = machine.zeroRedirect[runmachine.codepointer];
    uint16_t& redirect = machine.redirect[runmachine.codepointer];
    memorytape& currentTape = runmachine.memories[runmachine.tapepointer];
    uint16_t& currentPointer = runmachine.pointers[runmachine.tapepointer];
    uint16_t& currentLoc = currentTape[currentPointer];

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

    if (currentLoc == 0) {
        runmachine.codepointer = zeroRedirect;
    } else {
        runmachine.codepointer = redirect;
    }

    runmachine.codepointer += 1;
}

RunningMachine roll(uint16_t exec_limit, uint16_t prog_len_limit, uint16_t memory_len, machstream target) {
    Machine machine = generateRandomMachine(prog_len_limit);
    RunningMachine runmachine = initiateMachine(machine, memory_len);

    for (int i = 0; i < exec_limit; ++i) {
        if (runmachine.output.size() >= target.size()) {
            break;
        }
        runTick(runmachine);
    }

    return runmachine;
}

uint32_t error(machstream target, machstream prediction) {
    if (target.size() > prediction.size()) {
        return std::numeric_limits<uint32_t>::max();
    }

    uint32_t sum = 0;
    for (uint16_t i = 0; i < target.size(); ++i) {
            sum += std::min<uint16_t>(target[i] - prediction[i], prediction[i] - target[i]);
    }

    return sum;
}

void continueExec(RunningMachine& runmachine, uint16_t req, uint32_t continue_limit) {
#ifdef DEBUG_PREDICT_PROGRESS
    std::cerr << " ...continue" << std::endl;
#endif
    for (int i = 0; i < continue_limit; ++i) {
        if (runmachine.output.size() >= req) {
            break;
        }
        runTick(runmachine);
    }
}

RunningMachine predict(machstream target, uint16_t exec_limit, uint16_t prog_len_limit, uint16_t memory_len, uint32_t search_depth, uint16_t continue_req, uint32_t continue_limit) {
    uint32_t search_index = 0;
    std::unique_ptr<RunningMachine> best;
    uint32_t besterror = std::numeric_limits<uint32_t>::max();
    uint16_t this_prog_len_limit = prog_len_limit;
    while (true) {
#ifdef DEBUG_PREDICT_PROGRESS
        if (search_index % 1000 == 0) {
            std::cerr << "f";
        }
#endif

        if (search_index >= search_depth) {
            break;
        }

        RunningMachine alt = roll(exec_limit, this_prog_len_limit, memory_len, target);
        uint32_t alterror;

        alterror = error(target, alt.output);

        if (alterror < besterror) {
            best.reset(new RunningMachine(std::move(alt)));
            besterror = alterror;
        }
        else if (alterror == 0) {
            best.reset(new RunningMachine(std::move(alt)));
            besterror = 0;
            this_prog_len_limit = best->machine.length;
        }

        search_index += 1;
    }

    continueExec(*best, target.size() + continue_req, continue_limit);

    return *best;
}

template<typename InputStream>
machstream parseInput(InputStream& input) {
    machstream result;
    uint16_t num;

    while (input >> num) {
        result.push_back(num);
    }

    return result;
}

void signalHandler(int signal) {
    std::cerr << "We found signal: " << signal << std::endl;

    exit(1);
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGILL, signalHandler);
    std::signal(SIGABRT, signalHandler);
    std::signal(SIGFPE, signalHandler);
    std::signal(SIGSEGV, signalHandler);

    std::vector<std::string> args(argv, argv + argc);

    uint16_t exec_limit = 500;
    uint16_t prog_len_limit = 100;
    uint16_t memory_len = 100;
    uint32_t search_depth = 10000;
    uint16_t continue_req = 10;
    uint32_t continue_limit = 10000;

    bool alreadyCalc = false;

    machstream target;

    for (int i = 1; i < argc; ++i) {
        if ((args[i] == "-o") || (args[i] == "--options")) {
            if (i + 6 >= argc) {
                std::cerr << "Invalid input: option flag.";
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
            ++i;
            if (args[i] != "d") {
                continue_req = std::stoi(args[i]);
            }
            ++i;
            if (args[i] != "d") {
                continue_limit = std::stoi(args[i]);
            }
        }
        if ((args[i] == "-s") || (args[i] == "--seed")) {
            if (i + 1 >= argc) {
                std::cerr << "Invalid input: seed flag." << std::endl;
                return 1;
            }
            ++i;
            seed = std::stoi(args[i]);
            gen = std::mt19937(seed);
        }
        if (args[i] == "-c" or args[i] == "--compute") {
            alreadyCalc = true;
            if (i + 1 < argc) {
                for (int j = ++i; j < argc; ++j) {
                    target.push_back(std::stoi(args[j]));

                }
            } else {
                target = parseInput(std::cin);
            }

            RunningMachine result = predict(target, exec_limit, prog_len_limit, memory_len, search_depth, continue_req, continue_limit);
            std::cout << vectorToString(result.output) << std::endl;
#ifdef DEBUG_MACHINE_INFO
            std::cerr << arrayToString(result.machine.codeop, result.machine.length) << std::endl;
            std::cerr << arrayToString(result.machine.codeval, result.machine.length) << std::endl;
            std::cerr << arrayToString(result.machine.zeroRedirect, result.machine.length) << std::endl;
            std::cerr << arrayToString(result.machine.redirect, result.machine.length) << std::endl;
            std::cerr << error(target, result.output) << std::endl;
#endif
        }
    }

    if (not alreadyCalc) {
        target = parseInput(std::cin);

        RunningMachine result = predict(target, exec_limit, prog_len_limit, memory_len, search_depth, continue_req, continue_limit);
        std::cout << vectorToString(result.output) << std::endl;
    }

    return 0;
}
