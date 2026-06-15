#pragma once
#include <string>

struct Instrument {
    std::string symbol;
    std::string base;
    std::string quote;

    std::string normalized() const { return base + quote; }
};
