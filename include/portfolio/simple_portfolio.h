#pragma once
#include "portfolio/i_portfolio.h"
#include "core/instrument.h"
#include <vector>
#include <string>

struct TradeRecord {
    std::string timestamp;
    std::string action;
    double price1, price2;
    double size1, size2;
    double commission;
    double pnl_realized;
};

class SimplePortfolio : public IPortfolio {
public:
    SimplePortfolio(double initial_balance, double commission_rate,
                    const Instrument& sym1, const Instrument& sym2);

    void process_signal(const std::string& signal, double price1, double price2) override;
    double equity(double price1, double price2) const override;
    void close_at_market(double price1, double price2) override;
    bool has_position() const override;
    void print_status(double price1, double price2) const override;

    double balance() const { return balance_usdt_; }
    const std::vector<TradeRecord>& trades() const { return trades_; }

private:
    void execute_trade(const std::string& action, double price1, double price2,
                       double size1, double size2, double pnl = 0.0);
    std::string current_timestamp() const;

    Instrument sym1_, sym2_;
    double initial_balance_;
    double balance_usdt_;
    double pos1_ = 0.0, pos2_ = 0.0;
    double avg_entry1_ = 0.0, avg_entry2_ = 0.0;
    double commission_rate_;
    int direction_ = 0;
    std::vector<TradeRecord> trades_;
};
