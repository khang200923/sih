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

template<typename T>
void configVal(std::vector<std::string> commands, T& configVar, int& i, std::vector<std::string>& args) {
    if (std::find(commands.begin(), commands.end(), args[i]) != commands.end()) {
        ++i;
        configVar = std::stoi(args[i]);
    }
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
        configVal(std::vector<std::string>{"-el", "--exec-limit"}, exec_limit, i, args);
        configVal(std::vector<std::string>{"-pll", "--prog-len-limit"}, prog_len_limit, i, args);
        configVal(std::vector<std::string>{"-ml", "--memory-len"}, memory_len, i, args);
        configVal(std::vector<std::string>{"-sd", "--search-depth"}, search_depth, i, args);
        configVal(std::vector<std::string>{"-cr", "--continue-req"}, continue_req, i, args);
        configVal(std::vector<std::string>{"-cl", "--continue-limit"}, continue_limit, i, args);
        configVal(std::vector<std::string>{"-tn", "--threads-num"}, threads_num, i, args);
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
            std::cerr << mach::error(target, result.output) << std::endl;
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
