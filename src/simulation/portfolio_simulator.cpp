#include "simulation/portfolio_simulator.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

PortfolioSimulator::PortfolioSimulator(double initial_balance_usdt, double commission_rate,
                                       const std::string& sym1, const std::string& sym2,
                                       double leverage, double min_balance)
    : sym1_(sym1), sym2_(sym2), initial_balance_(initial_balance_usdt)
    , balance_usdt_(initial_balance_usdt), position_sym1_(0.0), position_sym2_(0.0)
    , commission_rate_(commission_rate), avg_entry_sym1_(0.0), avg_entry_sym2_(0.0)
    , leverage_(leverage), initial_margin_rate_(1.0 / leverage), min_balance_(min_balance) {}

void PortfolioSimulator::process_signal(const std::string& signal, double price_sym1, double price_sym2) {
    if (signal.find("SELL") == std::string::npos || signal.find("BUY") == std::string::npos)
        return;

    int signal_direction = 0;
    if (signal.find("SELL " + sym1_) != std::string::npos && signal.find("BUY " + sym2_) != std::string::npos)
        signal_direction = -1;
    else if (signal.find("SELL " + sym2_) != std::string::npos && signal.find("BUY " + sym1_) != std::string::npos)
        signal_direction = 1;
    else
        return;

    if (signal_direction == current_direction_)
        return;

    double risk_usdt = balance_usdt_ * 0.95;
    double size_sym1 = 0.0, size_sym2 = 0.0;

    if (signal_direction == -1) {
        size_sym1 = -risk_usdt / price_sym1;
        size_sym2 =  risk_usdt / price_sym2;
    } else {
        size_sym1 =  risk_usdt / price_sym1;
        size_sym2 = -risk_usdt / price_sym2;
    }

    if (std::abs(position_sym1_) > 1e-8 || std::abs(position_sym2_) > 1e-8) {
        double close_s1 = -position_sym1_;
        double close_s2 = -position_sym2_;
        double cost_s1 = close_s1 * price_sym1;
        double cost_s2 = close_s2 * price_sym2;
        double commission = (std::abs(cost_s1) + std::abs(cost_s2)) * commission_rate_;
        double pnl = -(cost_s1 + cost_s2) - commission;
        balance_usdt_ += pnl;
        execute_trade("CLOSE", price_sym1, price_sym2, close_s1, close_s2);
        position_sym1_ = 0.0;
        position_sym2_ = 0.0;
        avg_entry_sym1_ = 0.0;
        avg_entry_sym2_ = 0.0;
        current_direction_ = 0;
    }

    double trade_value = size_sym1 * price_sym1 + size_sym2 * price_sym2;
    double total_commission = (std::abs(size_sym1 * price_sym1) + std::abs(size_sym2 * price_sym2)) * commission_rate_;
    double new_balance = balance_usdt_ - trade_value - total_commission;
    if (new_balance < min_balance_) {
        std::cout << "[PORTFOLIO] Insufficient balance to open new position\n";
        return;
    }

    double required_margin = (std::abs(size_sym1 * price_sym1) + std::abs(size_sym2 * price_sym2)) * initial_margin_rate_;
    if (new_balance < required_margin + min_balance_) {
        std::cout << "[PORTFOLIO] Margin requirement not met\n";
        return;
    }

    balance_usdt_ = new_balance;
    position_sym1_ = size_sym1;
    position_sym2_ = size_sym2;
    avg_entry_sym1_ = price_sym1;
    avg_entry_sym2_ = price_sym2;
    current_direction_ = signal_direction;
    execute_trade(signal, price_sym1, price_sym2, size_sym1, size_sym2);
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
    std::cout << "Balance USDT: " << balance_usdt_ << "\n";
    std::cout << "Position " << sym1_ << ": " << position_sym1_
              << " (entry ~" << avg_entry_sym1_ << ")\n";
    std::cout << "Position " << sym2_ << ": " << position_sym2_
              << " (entry ~" << avg_entry_sym2_ << ")\n";
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
                                       double size_sym1, double size_sym2) {
    TradeRecord t;
    t.timestamp = current_timestamp();
    t.action = action;
    t.price_sym1 = price_sym1;
    t.price_sym2 = price_sym2;
    t.size_sym1 = size_sym1;
    t.size_sym2 = size_sym2;
    t.commission = (std::abs(size_sym1 * price_sym1) + std::abs(size_sym2 * price_sym2)) * commission_rate_;
    t.pnl_realized = 0.0;
    trades_.push_back(t);

    std::cout << "[TRADE] " << action << " | " << sym1_ << " " << size_sym1
              << " @ " << price_sym1 << " | " << sym2_ << " " << size_sym2
              << " @ " << price_sym2 << " | comm: " << t.commission << "\n";
}
