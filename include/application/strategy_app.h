#pragma once
#include <memory>
#include <string>
#include "core/instrument.h"
#include "data/i_live_data_feed.h"
#include "strategy/pair_trading_strategy.h"

struct AppConfig {
    std::string symbol1 = "BTC";
    std::string symbol2 = "ETH";
    std::string quote = "USDT";
    int spread_window = 50;
    int warmup_bars = 300;
    double initial_balance = 10000.0;
    double commission = 0.001;
    double threshold_mult = 2.0;
    double min_threshold = 0.5;
    double pseudo_beta_R = 0.5;
};

class StrategyApplication {
public:
    explicit StrategyApplication(const AppConfig& config);
    int run();

private:
    AppConfig config_;
    Instrument instr1_, instr2_;

    std::unique_ptr<IStateEstimator> create_kalman();
    std::unique_ptr<ILiveDataFeed> create_feed();
};
