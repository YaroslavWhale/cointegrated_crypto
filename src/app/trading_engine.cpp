#include "app/trading_engine.h"
#include "domain/kalman_filter.h"
#include "domain/z_signal.h"
#include "data/rest_client.h"
#include "data/websocket_client.h"
#include "simulation/portfolio_simulator.h"
#include <iostream>
#include <deque>
#include <atomic>
#include <cmath>
#include <csignal>
#include <thread>
#include <chrono>

static std::atomic<bool> active{true};

void signal_handler(int /*sig*/) {
    std::cout << "\n[CTRL+C] Stopping strategy...\n";
    active = false;
}

void run_live_strategy(const std::string& sym1, const std::string& sym2,
                       int spread_window) {
    std::cout.setf(std::ios::unitbuf);
    std::signal(SIGINT, signal_handler);

    std::cout << "\n=== Starting Pair Trading Strategy ===\n";
    std::cout << "Pair: " << sym1 << "/" << sym2 << std::endl;
    std::cout << "Spread window: " << spread_window << std::endl << std::endl;

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
        std::cerr << "Unsupported symbols\n";
        return;
    }

    // Фиксированные параметры для фильтра Калмана (beta=1, alpha=0)
    const double alpha_fixed = 0.0;
    const double beta_fixed = 1.0;

    std::cout << "[1/3] Initializing Kalman filter and spread analyzer...\n";
    KalmanFilter kf(0.001, 0.0001, 0.0001, alpha_fixed, beta_fixed);
    SpreadAnalyzer analyzer(sym1, sym2, spread_window);
    PortfolioSimulator portfolio(10000.0, 0.001, sym1, sym2);

    // Прогрев фильтра Калмана и анализатора историческими минутными свечами
    const int warmup_bars = std::max(spread_window, 100);
    std::cout << "[2/3] Warming up Kalman filter with historical 1m candles ("
              << warmup_bars << " bars)...\n";
    auto warmup_closes1 = RestClient::fetch_klines(pair1, "1m", warmup_bars);
    auto warmup_closes2 = RestClient::fetch_klines(pair2, "1m", warmup_bars);

    if (warmup_closes1.size() >= 2 && warmup_closes2.size() >= 2) {
        size_t n = std::min(warmup_closes1.size(), warmup_closes2.size());
        for (size_t i = 0; i < n; ++i) {
            double log_p1 = std::log(warmup_closes1[i]);
            double log_p2 = std::log(warmup_closes2[i]);
            kf.update(log_p1, log_p2);
            analyzer.add_spread(kf.get_spread());
        }
        std::cout << "[2/3] Warm-up complete. Kalman filter and spread analyzer initialized with "
                  << n << " historical 1m candles.\n";
    } else {
        std::cerr << "[2/3] Not enough historical 1m data for warm-up. Starting cold.\n";
    }

    std::cout << "[3/3] Starting WebSocket (minute klines)...\n";
    WebSocketPriceFeed feed;

    feed.set_callback([&](const std::string& s1, double p1,
                          const std::string& s2, double p2,
                          uint64_t /*close_time*/) {
        if (!active) return;
        double log_p1, log_p2;
        if (s1 == pair1) {
            log_p1 = std::log(p1);
            log_p2 = std::log(p2);
        } else {
            log_p1 = std::log(p2);
            log_p2 = std::log(p1);
        }

        kf.update(log_p1, log_p2);
        double spread = kf.get_spread();
        std::string signal = analyzer.add_spread(spread);
        double z = analyzer.get_z_score();

        double price1 = std::exp(log_p1);
        double price2 = std::exp(log_p2);
        std::cout << sym1 << "=" << price1 << "  "
                  << sym2 << "=" << price2
                  << "  | Log spread=" << spread
                  << "  | Z=" << z
                  << "  | " << signal << std::endl;

        if (signal.find("SELL") != std::string::npos || signal.find("BUY") != std::string::npos) {
            portfolio.process_signal(signal, price1, price2);
            portfolio.print_status(price1, price2);
        }
    });

    feed.subscribe(pair1 + "@kline_1m");
    feed.subscribe(pair2 + "@kline_1m");

    std::cout << "\n=== STRATEGY IS LIVE ===\nPress Ctrl+C to stop\n\n";

    std::thread ws_thread([&feed]() { feed.run(); });

    while (active) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    feed.stop();
    if (ws_thread.joinable()) ws_thread.join();
    std::cout << "Strategy terminated.\n";
}
