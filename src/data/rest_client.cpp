#include "data/rest_client.hpp"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <iostream>

std::vector<double> BinanceRestClient::fetch_klines(const std::string& symbol,
                                                     const std::string& interval,
                                                     int limit) {
    std::vector<double> closes;
    std::string url = "https://api.binance.com/api/v3/klines";
    auto r = cpr::Get(cpr::Url{url},
                      cpr::Parameters{{"symbol", symbol},
                                      {"interval", interval},
                                      {"limit", std::to_string(limit)}},
                      cpr::Timeout{10000});
    if (r.status_code != 200) {
        std::cerr << "[REST] HTTP " << r.status_code << "\n";
        return {};
    }
    auto j = nlohmann::json::parse(r.text);
    for (auto& c : j)
        closes.push_back(std::stod(c[4].get<std::string>()));
    std::cout << "[REST] Fetched " << closes.size() << " closes\n";
    return closes;
}
