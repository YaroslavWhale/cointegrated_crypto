#pragma once
#include <cstdint>
#include <functional>
#include <string>

using PriceCallback = std::function<void(const std::string& symbol, double price, uint64_t timestamp)>;
using PairPriceCallback = std::function<void(double price1, double price2, uint64_t timestamp)>;
