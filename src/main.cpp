#include "application/strategy_app.h"
#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>

int main(int argc, char* argv[]) {
    AppConfig cfg;
    cfg.symbol1 = "BTC";
    cfg.symbol2 = "ETH";
    cfg.quote = "USDT";
    cfg.spread_window = 150;
    cfg.pseudo_beta_R = 0.5;

    auto to_upper = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        return s;
    };

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--sym1" && i+1 < argc) cfg.symbol1 = to_upper(argv[++i]);
        else if (arg == "--sym2" && i+1 < argc) cfg.symbol2 = to_upper(argv[++i]);
        else if (arg == "--window" && i+1 < argc) cfg.spread_window = std::stoi(argv[++i]);
        else if (arg == "--pseudo_beta_R" && i+1 < argc) cfg.pseudo_beta_R = std::stod(argv[++i]);
        else {
            std::cerr << "Usage: " << argv[0]
                      << " [--sym1 BTC] [--sym2 ETH] [--window 50] [--pseudo_beta_R 0.5]\n";
            return 1;
        }
    }

    StrategyApplication app(cfg);
    return app.run();
}
