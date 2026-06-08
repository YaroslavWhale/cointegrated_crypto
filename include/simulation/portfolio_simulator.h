#pragma once
#include <string>
#include <vector>
#include <chrono>

struct TradeRecord {
    std::string timestamp;
    std::string action;
    double price_sym1;
    double price_sym2;
    double size_sym1;
    double size_sym2;
    double commission;
    double pnl_realized;
};

class PortfolioSimulator {
public:
    PortfolioSimulator(double initial_balance_usdt, double commission_rate = 0.001,
                       const std::string& sym1 = "BTC", const std::string& sym2 = "ETH",
                       double leverage = 1.0, double min_balance = 10.0);

    void process_signal(const std::string& signal, double price_sym1, double price_sym2);
    double get_equity(double price_sym1, double price_sym2) const;
    double get_balance() const { return balance_usdt_; }
    double get_unrealized_pnl(double price_sym1, double price_sym2) const;
    void print_status(double price_sym1, double price_sym2) const;
    const std::vector<TradeRecord>& get_trades() const { return trades_; }
    void reset();

private:
    std::string sym1_, sym2_;
    double initial_balance_;
    double balance_usdt_;
    double position_sym1_;
    double position_sym2_;
    double commission_rate_;
    double avg_entry_sym1_;
    double avg_entry_sym2_;
    std::vector<TradeRecord> trades_;

    double leverage_;
    double initial_margin_rate_;
    double min_balance_;

    int current_direction_ = 0;

    std::string current_timestamp() const;
    void execute_trade(const std::string& action, double price_sym1, double price_sym2,
                       double size_sym1, double size_sym2);
};
