#include "data/rest_client.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <iostream>

std::vector<double> RestClient::fetch_klines(const std::string& symbol,
                                             const std::string& interval,
                                             int limit) {
    std::vector<double> closes;
    std::cout << "[REST] Fetching " << limit << " " << interval
              << " candles for " << symbol << std::endl;

    std::string url = "https://api.binance.com/api/v3/klines";

    cpr::Response r = cpr::Get(
        cpr::Url{url},
        cpr::Parameters{
            {"symbol", symbol},
            {"interval", interval},
            {"limit", std::to_string(limit)}
        },
        cpr::Timeout{10000}
        );

    std::cout << "[REST] Status: " << r.status_code << std::endl;

    if (r.status_code != 200) {
        std::cerr << "[REST] Error: HTTP " << r.status_code << std::endl;
        std::cerr << "[REST] Response: " << r.text.substr(0, 200) << std::endl;
        return closes;
    }

    try {
        auto j = nlohmann::json::parse(r.text);
        std::cout << "[REST] Parsed " << j.size() << " candles" << std::endl;

        for (const auto& candle : j) {
            double close = std::stod(candle[4].get<std::string>());
            closes.push_back(close);
        }
    } catch (const std::exception& e) {
        std::cerr << "[REST] JSON error: " << e.what() << std::endl;
        std::cerr << "[REST] Raw: " << r.text.substr(0, 300) << std::endl;
    }

    std::cout << "[REST] Fetched " << closes.size() << " closes" << std::endl;
    return closes;
}
