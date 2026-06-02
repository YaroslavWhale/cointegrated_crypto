#include "data/api_client.h"
#include <iostream>
#include <map>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Вспомогательная функция, скрытая внутри cpp
namespace {
    std::string symbol_to_pair(const std::string& symbol) {
        static const std::map<std::string, std::string> mapping = {
            {"BTC", "BTCUSDT"}, {"ETH", "ETHUSDT"}, {"BNB", "BNBUSDT"},
            {"SOL", "SOLUSDT"}, {"XRP", "XRPUSDT"}, {"ADA", "ADAUSDT"},
            {"DOGE", "DOGEUSDT"}, {"LTC", "LTCUSDT"}, {"DOT", "DOTUSDT"},
            {"MATIC", "MATICUSDT"}
        };
        auto it = mapping.find(symbol);
        return it != mapping.end() ? it->second : "";
    }
}

std::vector<double> fetch_historical_prices(const std::string& symbol,
                                            const std::string& interval,
                                            int limit) {
    std::string pair = symbol_to_pair(symbol);
    if (pair.empty()) {
        std::cerr << "Unsupported symbol: " << symbol << std::endl;
        return {};
    }

    std::string url = "https://api.binance.com/api/v3/klines?symbol=" + pair +
                      "&interval=" + interval + "&limit=" + std::to_string(limit);

    cpr::Response r = cpr::Get(cpr::Url{url});
    if (r.status_code != 200) {
        std::cerr << "HTTP error " << r.status_code << " for " << symbol << std::endl;
        return {};
    }

    try {
        json data = json::parse(r.text);
        if (!data.is_array() || data.empty()) {
            std::cerr << "Binance returned empty or invalid JSON for " << symbol << std::endl;
            return {};
        }

        std::vector<double> prices;
        prices.reserve(data.size());
        for (const auto& candle : data) {
            if (candle.is_array() && candle.size() > 4) {
                prices.push_back(std::stod(candle[4].get<std::string>()));
            }
        }
        return prices;
    } catch (const std::exception& e) {
        std::cerr << "JSON parse error for " << symbol << ": " << e.what() << std::endl;
        return {};
    }
}
