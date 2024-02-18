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
#include <mutex>
#include <thread>
#include <future>

#include "machine.h"

//#define DEBUG_MACHINE_INFO
//#define DEBUG_PREDICT_PROGRESS

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

template<typename InputStream>
mach::machstream parseInput(InputStream& input) {
    mach::machstream result;
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
    uint32_t threads_num = 1;

    bool alreadyCalc = false;

    mach::machstream target;

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
            ++i;
            if (args[i] != "d") {
                threads_num = std::stoi(args[i]);
            }
        }
        if ((args[i] == "-s") || (args[i] == "--seed")) {
            if (i + 1 >= argc) {
                std::cerr << "Invalid input: seed flag." << std::endl;
                return 1;
            }
            ++i;
            mach::seed = std::stoi(args[i]);
            mach::gen = std::mt19937(mach::seed);
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

            mach::RunningMachine result = mach::predict(target, exec_limit, prog_len_limit, memory_len, search_depth, continue_req, continue_limit, threads_num);
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

        mach::RunningMachine result = mach::predict(target, exec_limit, prog_len_limit, memory_len, search_depth, continue_req, continue_limit, threads_num);
        std::cout << vectorToString(result.output) << std::endl;
    }

    return 0;
}
