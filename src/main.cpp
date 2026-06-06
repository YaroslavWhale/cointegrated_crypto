#include "app/trading_engine.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string sym1 = "BTC";
    std::string sym2 = "ETH";
    int window = 50;
    int coint_check = 60;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--sym1" && i + 1 < argc) sym1 = argv[++i];
        else if (arg == "--sym2" && i + 1 < argc) sym2 = argv[++i];
        else if (arg == "--window" && i + 1 < argc) window = std::stoi(argv[++i]);
        else if (arg == "--coint_check" && i + 1 < argc) coint_check = std::stoi(argv[++i]);
        else {
            std::cerr << "Usage: " << argv[0]
                      << " [--sym1 BTC] [--sym2 ETH] [--window 50] [--coint_check 60]\n";
            return 1;
        }
    }

    run_live_strategy(sym1, sym2, window, coint_check);
    return 0;
}
