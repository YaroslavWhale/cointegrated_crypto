#include "app/trading_engine.h"
#include "domain/kalman_filter.h"
#include "domain/spread_analyzer.h"
#include "data/websocket_client.h"
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <cctype>

void run_live_strategy(const std::string& sym1, const std::string& sym2, int spread_window) {
    auto to_pair = [](const std::string& sym) -> std::string {
        static const std::unordered_map<std::string, std::string> map = {
            {"BTC", "BTCUSDT"}, {"ETH", "ETHUSDT"}, {"BNB", "BNBUSDT"},
            {"SOL", "SOLUSDT"}, {"XRP", "XRPUSDT"}
        };
        auto it = map.find(sym);
        return it != map.end() ? it->second : "";
    };

    std::string pair1 = to_pair(sym1);
    std::string pair2 = to_pair(sym2);
    if (pair1.empty() || pair2.empty()) {
        std::cerr << "Unsupported symbols" << std::endl;
        return;
    }

    KalmanFilter kf(0.001, 0.0001, 0.0001);
    SpreadAnalyzer analyzer(sym1, sym2, spread_window);

    std::unordered_map<std::string, double> latest_price;

    WebSocketPriceFeed::PriceCallback callback = [&](const std::string& symbol, double price) {
        latest_price[symbol] = price;
        if (latest_price.count(pair1) && latest_price.count(pair2)) {
            double p1 = latest_price[pair1];
            double p2 = latest_price[pair2];
            kf.update(p1, p2);
            double spread = kf.get_spread();
            std::string signal = analyzer.add_spread(spread);
            double z = analyzer.get_z_score();

            std::cout << "Price " << sym1 << "=" << p1
                      << ", " << sym2 << "=" << p2
                      << " | Spread=" << spread
                      << " | Z=" << z
                      << " | Signal: " << signal << std::endl;
        }
    };

    WebSocketPriceFeed feed(callback);
    feed.subscribe(pair1 + "@kline_1m");
    feed.subscribe(pair2 + "@kline_1m");

    std::cout << "Starting live strategy for " << sym1 << "/" << sym2
              << " (window=" << spread_window << ")\n";
    feed.run();
}
