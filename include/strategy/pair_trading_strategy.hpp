#pragma once
#include "filters/i_state_estimator.hpp"
#include "analysis/spread_analyzer.hpp"
#include "portfolio/simple_portfolio.hpp"
#include "core/instrument.hpp"
#include <memory>
#include <cstdint>

class PairTradingStrategy {
public:
    PairTradingStrategy(std::unique_ptr<IStateEstimator> estimator,
                        std::unique_ptr<SpreadAnalyzer> analyzer,
                        std::unique_ptr<SimplePortfolio> portfolio,
                        const Instrument& instr1,
                        const Instrument& instr2);

    void on_price_update(double price1, double price2, uint64_t timestamp_ms);

private:
    std::unique_ptr<IStateEstimator> estimator_;
    std::unique_ptr<SpreadAnalyzer> analyzer_;
    std::unique_ptr<SimplePortfolio> portfolio_;
    Instrument instr1_;
    Instrument instr2_;

    enum class PositionState { FLAT, LONG, SHORT };
    PositionState position_state_ = PositionState::FLAT;

    void enter_long(double spread, double z_score);
    void enter_short(double spread, double z_score);
    void exit_position(double spread, double z_score);
    double calculate_position_size(double spread, double z_score) const;
};
