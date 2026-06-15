#pragma once
#include <string>

class IPortfolio {
public:
    virtual ~IPortfolio() = default;
    virtual void process_signal(const std::string& signal, double price1, double price2) = 0;
    virtual double equity(double price1, double price2) const = 0;
    virtual void close_at_market(double price1, double price2) = 0;
    virtual bool has_position() const = 0;
    virtual void print_status(double price1, double price2) const = 0;
};
