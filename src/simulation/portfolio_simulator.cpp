#include "simulation/portfolio_simulator.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cmath>

PortfolioSimulator::PortfolioSimulator(double initial_balance_usdt, double commission_rate,
                                       const std::string& sym1, const std::string& sym2)
    : sym1_(sym1), sym2_(sym2), initial_balance_(initial_balance_usdt),
    balance_usdt_(initial_balance_usdt), position_sym1_(0.0), position_sym2_(0.0),
    commission_rate_(commission_rate), avg_entry_sym1_(0.0), avg_entry_sym2_(0.0),
    current_direction_(0) {}

void PortfolioSimulator::process_signal(const std::string& signal, double price_sym1, double price_sym2) {
    int new_direction = 0;
    if (signal.find("SELL " + sym1_) != std::string::npos && signal.find("BUY " + sym2_) != std::string::npos) {
        new_direction = -1;
    } else if (signal.find("BUY " + sym1_) != std::string::npos && signal.find("SELL " + sym2_) != std::string::npos) {
        new_direction = 1;
    } else {
        return;
    }

    if (new_direction == current_direction_)
        return;

    if (current_direction_ != 0) {
        double close_s1 = -position_sym1_;
        double close_s2 = -position_sym2_;

        double proceeds = close_s1 * price_sym1 + close_s2 * price_sym2;
        double open_cost = position_sym1_ * avg_entry_sym1_ + position_sym2_ * avg_entry_sym2_;
        double commission = (std::abs(close_s1 * price_sym1) + std::abs(close_s2 * price_sym2)) * commission_rate_;
        double pnl_closed = proceeds - open_cost - commission;

        balance_usdt_ += pnl_closed;
        execute_trade("CLOSE", price_sym1, price_sym2, close_s1, close_s2, pnl_closed);

        position_sym1_ = 0.0;
        position_sym2_ = 0.0;
        avg_entry_sym1_ = 0.0;
        avg_entry_sym2_ = 0.0;
        current_direction_ = 0;
    }

    double capital = balance_usdt_;
    double exposure = capital * 0.5;

    if (new_direction == -1) {
        position_sym1_ = -exposure / price_sym1;
        position_sym2_ =  exposure / price_sym2;
    } else {
        position_sym1_ =  exposure / price_sym1;
        position_sym2_ = -exposure / price_sym2;
    }

    double cost_s1 = position_sym1_ * price_sym1;
    double cost_s2 = position_sym2_ * price_sym2;
    double total_cost = cost_s1 + cost_s2;
    double commission = (std::abs(cost_s1) + std::abs(cost_s2)) * commission_rate_;
    balance_usdt_ -= total_cost;
    balance_usdt_ -= commission;

    avg_entry_sym1_ = price_sym1;
    avg_entry_sym2_ = price_sym2;
    current_direction_ = new_direction;
    execute_trade((new_direction == -1) ? "SHORT " + sym1_ + "/LONG " + sym2_
                                        : "LONG " + sym1_ + "/SHORT " + sym2_,
                  price_sym1, price_sym2, position_sym1_, position_sym2_, 0.0);
}

double PortfolioSimulator::get_equity(double price_sym1, double price_sym2) const {
    return balance_usdt_ + position_sym1_ * price_sym1 + position_sym2_ * price_sym2;
}

double PortfolioSimulator::get_unrealized_pnl(double price_sym1, double price_sym2) const {
    double current = position_sym1_ * price_sym1 + position_sym2_ * price_sym2;
    double cost = position_sym1_ * avg_entry_sym1_ + position_sym2_ * avg_entry_sym2_;
    return current - cost;
}

void PortfolioSimulator::print_status(double price_sym1, double price_sym2) const {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n=== PORTFOLIO STATUS ===\n";
    std::cout << "Free USDT: " << balance_usdt_ << "\n";
    std::cout << "Position " << sym1_ << ": " << position_sym1_
              << " (entry " << avg_entry_sym1_ << ")\n";
    std::cout << "Position " << sym2_ << ": " << position_sym2_
              << " (entry " << avg_entry_sym2_ << ")\n";
    std::cout << "Unrealized P&L: " << get_unrealized_pnl(price_sym1, price_sym2) << " USDT\n";
    double equity = get_equity(price_sym1, price_sym2);
    std::cout << "Total Equity: " << equity << " USDT\n";
    std::cout << "Total return: " << (equity / initial_balance_ - 1.0) * 100 << "%\n";
    std::cout << "========================\n";
}

void PortfolioSimulator::reset() {
    balance_usdt_ = initial_balance_;
    position_sym1_ = 0.0;
    position_sym2_ = 0.0;
    avg_entry_sym1_ = 0.0;
    avg_entry_sym2_ = 0.0;
    trades_.clear();
    current_direction_ = 0;
}

std::string PortfolioSimulator::current_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void PortfolioSimulator::execute_trade(const std::string& action,
                                       double price_sym1, double price_sym2,
                                       double size_sym1, double size_sym2,
                                       double pnl_realized) {
    TradeRecord t;
    t.timestamp = current_timestamp();
    t.action = action;
    t.price_sym1 = price_sym1;
    t.price_sym2 = price_sym2;
    t.size_sym1 = size_sym1;
    t.size_sym2 = size_sym2;
    t.commission = (std::abs(size_sym1 * price_sym1) + std::abs(size_sym2 * price_sym2)) * commission_rate_;
    t.pnl_realized = pnl_realized;
    trades_.push_back(t);

    std::cout << "[TRADE] " << action << " | " << sym1_ << " " << size_sym1
              << " @ " << price_sym1 << " | " << sym2_ << " " << size_sym2
              << " @ " << price_sym2 << " | comm: " << t.commission
              << " | PnL: " << t.pnl_realized << "\n";
}

bool PortfolioSimulator::has_position() const {
    return position_sym1_ != 0.0 || position_sym2_ != 0.0;
}

void PortfolioSimulator::close_at_market(double price_sym1, double price_sym2) {
    if (!has_position()) return;

    double close_s1 = -position_sym1_;
    double close_s2 = -position_sym2_;
    double proceeds = close_s1 * price_sym1 + close_s2 * price_sym2;
    double open_cost = position_sym1_ * avg_entry_sym1_ + position_sym2_ * avg_entry_sym2_;
    double commission = (std::abs(close_s1 * price_sym1) + std::abs(close_s2 * price_sym2)) * commission_rate_;
    double pnl_closed = proceeds - open_cost - commission;

    balance_usdt_ += pnl_closed;
    execute_trade("CLOSE (mean-revert)", price_sym1, price_sym2, close_s1, close_s2, pnl_closed);

    position_sym1_ = 0.0;
    position_sym2_ = 0.0;
    avg_entry_sym1_ = 0.0;
    avg_entry_sym2_ = 0.0;
    current_direction_ = 0;
}
