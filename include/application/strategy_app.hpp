#pragma once
#include <memory>
#include <string>
#include "core/instrument.hpp"
#include "data/i_live_data_feed.hpp"
#include "strategy/pair_trading_strategy.hpp"

struct AppConfig {
    std::string symbol1 = "BTC";
    std::string symbol2 = "ETH";
    std::string quote = "USDT";
    std::string interval = "1m";
    int spread_window = 150;
    int warmup_bars = 300;
    double initial_balance = 10000.0;
    double commission = 0.001;
    double threshold_mult = 2.0;
    double min_threshold = 0.5;
};

class StrategyApplication {
public:
    explicit StrategyApplication(const AppConfig& config);
    int run();

private:
    std::unique_ptr<IStateEstimator> create_kalman();
    std::unique_ptr<ILiveDataFeed> create_feed();

    AppConfig config_;
    Instrument instr1_, instr2_;
};
