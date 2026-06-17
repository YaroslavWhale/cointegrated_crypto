#pragma once
#include <memory>
#include <cstdint>
#include "filters/i_state_estimator.hpp"
#include "analysis/spread_analyzer.hpp"
#include "portfolio/i_portfolio.hpp"

class PairTradingStrategy {
public:
    PairTradingStrategy(std::unique_ptr<IStateEstimator> est,
                        std::unique_ptr<SpreadAnalyzer> anal,
                        std::unique_ptr<IPortfolio> port);

    void on_price_update(double price1, double price2, uint64_t timestamp);

private:
    std::unique_ptr<IStateEstimator> estimator_;
    std::unique_ptr<SpreadAnalyzer> analyzer_;
    std::unique_ptr<IPortfolio> portfolio_;
    int tick_counter_ = 0;
};
