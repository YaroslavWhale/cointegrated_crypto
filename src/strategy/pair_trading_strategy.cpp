#include "strategy/pair_trading_strategy.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>

PairTradingStrategy::PairTradingStrategy(
    std::unique_ptr<IStateEstimator> estimator,
    std::unique_ptr<SpreadAnalyzer> analyzer,
    std::unique_ptr<SimplePortfolio> portfolio,
    const Instrument& instr1,
    const Instrument& instr2)
    : estimator_(std::move(estimator))
    , analyzer_(std::move(analyzer))
    , portfolio_(std::move(portfolio))
    , instr1_(instr1)
    , instr2_(instr2)
{}

void PairTradingStrategy::on_price_update(double price1, double price2, uint64_t timestamp_ms) {
    estimator_->update(std::log(price1), std::log(price2));
    double spread = estimator_->get_spread();

    analyzer_->add_spread(spread);
    auto signal = analyzer_->current_signal();
    double z = analyzer_->z_score();
    double entry_thr = analyzer_->entry_threshold();
    double exit_thr = analyzer_->exit_threshold();

    std::cout << std::fixed << std::setprecision(4)
              << "[ts=" << timestamp_ms << "] "
              << "spread=" << spread << " "
              << "z=" << z << " "
              << "entry_thr=" << entry_thr << " "
              << "exit_thr=" << exit_thr << " ";

    switch (signal) {
    case SpreadAnalyzer::SignalAction::ENTER_LONG:
        if (position_state_ == PositionState::FLAT) {
            std::cout << "-> ENTER_LONG";
            enter_long(spread, z);
            position_state_ = PositionState::LONG;
        } else if (position_state_ == PositionState::SHORT) {
            std::cout << "-> EXIT_SHORT + ENTER_LONG";
            exit_position(spread, z);
            enter_long(spread, z);
            position_state_ = PositionState::LONG;
        }
        break;

    case SpreadAnalyzer::SignalAction::ENTER_SHORT:
        if (position_state_ == PositionState::FLAT) {
            std::cout << "-> ENTER_SHORT";
            enter_short(spread, z);
            position_state_ = PositionState::SHORT;
        } else if (position_state_ == PositionState::LONG) {
            std::cout << "-> EXIT_LONG + ENTER_SHORT";
            exit_position(spread, z);
            enter_short(spread, z);
            position_state_ = PositionState::SHORT;
        }
        break;

    case SpreadAnalyzer::SignalAction::EXIT:
        if (position_state_ != PositionState::FLAT) {
            std::cout << "-> EXIT";
            exit_position(spread, z);
            position_state_ = PositionState::FLAT;
        }
        break;

    default:
        std::cout << "-> HOLD";
        break;
    }

    std::cout << " | portfolio_value=N/A" << "\n";
}

void PairTradingStrategy::enter_long(double spread, double z_score) {
    std::cout << " [enter_long would open position]";
}

void PairTradingStrategy::enter_short(double spread, double z_score) {
    std::cout << " [enter_short would open position]";
}

void PairTradingStrategy::exit_position(double spread, double z_score) {
    std::cout << " [exit_position would close]";
}

double PairTradingStrategy::calculate_position_size(double spread, double z_score) const {
    return 1.0;
}
