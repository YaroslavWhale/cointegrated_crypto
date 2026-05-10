#ifndef TICKER_H
#define TICKER_H

#include <iostream>
#include <string>
#include <vector>

class Ticker {
private:
    std::string symbol;
    double price;
    std::vector<double> price_history;

public:
    Ticker() : symbol(""), price(0.0) {}
    Ticker(const std::string& sym, double pr) : symbol(sym), price(pr) {}

    void set_symbol(const std::string& s) { symbol = s; }
    void set_price(double p) { price = p; }

    std::string get_symbol() const { return symbol; }
    double get_price() const { return price; }

    void add_to_history(double p) {
        price_history.push_back(p);
    }

    const std::vector<double>& get_history() const {
        return price_history;
    }

    void clear_history() { price_history.clear(); }

    void print() const {
        std::cout << "Ticker: " << symbol
                  << ", Price: $" << price
                  << ", History size: " << price_history.size()
                  << std::endl;
    }
};

#endif // TICKER_H
