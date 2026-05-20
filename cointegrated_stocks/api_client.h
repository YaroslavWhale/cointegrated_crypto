#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <string>
#include <vector>
#include <map>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

//inline
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

// Получение исторических цен закрытия (OHLC) для символа
//inline
std::vector<double> fetch_historical_prices(const std::string& symbol,
                                                   const std::string& interval = "1d",
                                                   int limit = 100) {
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
        std::vector<double> prices;
        for (const auto& candle : data) {
            if (candle.size() >= 5) {
                prices.push_back(std::stod(candle[4].get<std::string>()));
            }
        }
        return prices;
    } catch (const std::exception& e) {
        std::cerr << "JSON parse error for " << symbol << ": " << e.what() << std::endl;
        return {};
    }
}

#endif // API_CLIENT_H
