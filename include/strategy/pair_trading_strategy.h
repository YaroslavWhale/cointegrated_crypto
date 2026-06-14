#pragma once
#include <memory>
#include "filters/i_state_estimator.h"
#include "analysis/spread_analyzer.h"
#include "portfolio/i_portfolio.h"
#include "data/i_live_data_feed.h"

class PairTradingStrategy {
public:
    PairTradingStrategy(std::unique_ptr<IStateEstimator> estimator,
                        std::unique_ptr<SpreadAnalyzer> analyzer,
                        std::unique_ptr<IPortfolio> portfolio,
                        double pseudo_beta_R);

    void on_price_update(double price1, double price2, uint64_t timestamp);
    void reset();

private:
    std::unique_ptr<IStateEstimator> estimator_;
    std::unique_ptr<SpreadAnalyzer> analyzer_;
    std::unique_ptr<IPortfolio> portfolio_;
    double pseudo_beta_R_;
    int tick_counter_ = 0;
};
