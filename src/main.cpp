#include "application/strategy_app.hpp"
#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>

int main(int argc, char* argv[]) {
    AppConfig cfg;

    auto to_upper = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        return s;
    };

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--sym1" && i + 1 < argc)
            cfg.symbol1 = to_upper(argv[++i]);
        else if (arg == "--sym2" && i + 1 < argc)
            cfg.symbol2 = to_upper(argv[++i]);
        else if (arg == "--window" && i + 1 < argc)
            cfg.spread_window = std::stoi(argv[++i]);
        else if (arg == "--interval" && i + 1 < argc)
            cfg.interval = argv[++i];
        else {
            std::cerr << "Usage: " << argv[0]
                      << " [--sym1 BTC] [--sym2 ETH] [--window 150] [--interval 1m]\n";
            return 1;
        }
    }

    StrategyApplication app(cfg);
    return app.run();
}
