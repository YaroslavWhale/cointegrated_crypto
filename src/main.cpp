#include "app/trading_engine.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

int main(int argc, char* argv[]) {
    std::string sym1 = "BTC";
    std::string sym2 = "ETH";
    int window = 50;
    //int coint_check = 60;

    auto to_upper = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        return s;
    };

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--sym1" && i + 1 < argc) sym1 = to_upper(argv[++i]);
        else if (arg == "--sym2" && i + 1 < argc) sym2 = to_upper(argv[++i]);
        else if (arg == "--window" && i + 1 < argc) window = std::stoi(argv[++i]);
        //else if (arg == "--coint_check" && i + 1 < argc) coint_check = std::stoi(argv[++i]);
        else {
            std::cerr << "Usage: " << argv[0]
                      << " [--sym1 BTC] [--sym2 ETH] [--window 50]\n";
            return 1;
        }
    }

    //run_live_strategy(sym1, sym2, window, coint_check);
    run_live_strategy(sym1, sym2, window);
    return 0;
}
