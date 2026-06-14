#include "portfolio/simple_portfolio.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cmath>

SimplePortfolio::SimplePortfolio(double initial_balance, double commission,
                                 const Instrument& sym1, const Instrument& sym2)
    : sym1_(sym1), sym2_(sym2), initial_balance_(initial_balance),
      balance_usdt_(initial_balance), commission_rate_(commission) {}

bool SimplePortfolio::has_position() const {
    return pos1_ != 0.0 || pos2_ != 0.0;
}

void SimplePortfolio::process_signal(const std::string& signal,
                                     double price1, double price2) {
    int new_dir = 0;
    if (signal == "SELL_SYM1") new_dir = -1;
    else if (signal == "BUY_SYM1") new_dir = 1;
    else return;

    if (new_dir == direction_) return;

    if (has_position()) {
        double close1 = -pos1_, close2 = -pos2_;
        double proceeds = close1*price1 + close2*price2;
        double cost = pos1_*avg_entry1_ + pos2_*avg_entry2_;
        double comm = (std::abs(close1*price1) + std::abs(close2*price2)) * commission_rate_;
        double pnl = proceeds - cost - comm;
        balance_usdt_ += pnl;
        execute_trade("CLOSE", price1, price2, close1, close2, pnl);
        pos1_ = pos2_ = 0.0;
        avg_entry1_ = avg_entry2_ = 0.0;
        direction_ = 0;
    }

    double capital = balance_usdt_ * 0.5;
    if (new_dir == -1) {
        pos1_ = -capital / price1;
        pos2_ =  capital / price2;
    } else {
        pos1_ =  capital / price1;
        pos2_ = -capital / price2;
    }
    double cost = pos1_*price1 + pos2_*price2;
    double comm = (std::abs(pos1_*price1) + std::abs(pos2_*price2)) * commission_rate_;
    balance_usdt_ -= (cost + comm);
    avg_entry1_ = price1; avg_entry2_ = price2;
    direction_ = new_dir;
    execute_trade((new_dir == -1) ? "SHORT_SYM1_LONG_SYM2" : "LONG_SYM1_SHORT_SYM2",
                  price1, price2, pos1_, pos2_, 0.0);
}

void SimplePortfolio::close_at_market(double price1, double price2) {
    if (!has_position()) return;
    double close1 = -pos1_, close2 = -pos2_;
    double proceeds = close1*price1 + close2*price2;
    double cost = pos1_*avg_entry1_ + pos2_*avg_entry2_;
    double comm = (std::abs(close1*price1) + std::abs(close2*price2)) * commission_rate_;
    double pnl = proceeds - cost - comm;
    balance_usdt_ += pnl;
    execute_trade("CLOSE_AT_MARKET", price1, price2, close1, close2, pnl);
    pos1_ = pos2_ = 0.0; direction_ = 0;
}

double SimplePortfolio::equity(double p1, double p2) const {
    return balance_usdt_ + pos1_*p1 + pos2_*p2;
}

void SimplePortfolio::print_status(double p1, double p2) const {
    std::cout << std::fixed << std::setprecision(2)
              << "\n=== PORTFOLIO ===\n"
              << "Free: " << balance_usdt_ << "\n"
              << sym1_.symbol << ": " << pos1_ << " @ " << avg_entry1_
              << "\n" << sym2_.symbol << ": " << pos2_ << " @ " << avg_entry2_
              << "\nEquity: " << equity(p1, p2) << "\n\n";
}

void SimplePortfolio::execute_trade(const std::string& action,
                                    double p1, double p2,
                                    double s1, double s2, double pnl) {
    TradeRecord rec;
    rec.timestamp = current_timestamp();
    rec.action = action; rec.price1 = p1; rec.price2 = p2;
    rec.size1 = s1; rec.size2 = s2;
    rec.commission = (std::abs(s1*p1) + std::abs(s2*p2)) * commission_rate_;
    rec.pnl_realized = pnl;
    trades_.push_back(rec);
    std::cout << "[TRADE] " << action << " | " << sym1_.base << " " << s1
              << " @ " << p1 << " | " << sym2_.base << " " << s2
              << " @ " << p2 << " | PnL: " << pnl << "\n";
}

std::string SimplePortfolio::current_timestamp() const {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%F %T");
    return ss.str();
}
