// app/trading_engine.cpp
#include "app/trading_engine.h"
#include "domain/kalman_filter.h"
#include "domain/z_signal.h"
#include "data/rest_client.h"
#include "data/websocket_client.h"
#include "simulation/portfolio_simulator.h"
#include "utils/statistical_utils.h"

#include <iostream>
#include <deque>
#include <atomic>
#include <cmath>
#include <csignal>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <vector>

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
        if (sym.size() >= 4 && sym.substr(sym.size() - 4) == "USDT")
            return sym;
        return sym + "USDT";
    };

    std::string pair1 = to_pair(sym1);
    std::string pair2 = to_pair(sym2);
    if (pair1.empty() || pair2.empty() || pair1 == pair2) {
        std::cerr << "Invalid symbols or same pair. Exiting.\n";
        return;
    }

    // Увеличенный период разогрева: минимум 500 баров для сходимости фильтра
    const int warmup_bars = std::max(spread_window, 500);
    std::cout << "[1/4] Fetching historical 1m candles (" << warmup_bars << " bars)...\n";
    auto warmup_closes1 = RestClient::fetch_klines(pair1, "1m", warmup_bars);
    auto warmup_closes2 = RestClient::fetch_klines(pair2, "1m", warmup_bars);

    if (warmup_closes1.size() < 2 || warmup_closes2.size() < 2) {
        std::cerr << "[1/4] Not enough historical data. Exiting.\n";
        return;
    }

    size_t n = std::min(warmup_closes1.size(), warmup_closes2.size());
    std::vector<double> log_prices1(n), log_prices2(n);
    for (size_t i = 0; i < n; ++i) {
        log_prices1[i] = std::log(warmup_closes1[i]);
        log_prices2[i] = std::log(warmup_closes2[i]);
    }

    OLSResult ols = compute_ols(log_prices2, log_prices1);
    double alpha_init = ols.alpha;
    double beta_init = ols.beta;
    std::cout << "[2/4] Initial OLS: alpha=" << alpha_init << ", beta=" << beta_init << "\n";

    std::cout << "[3/4] Optimizing Kalman parameters...\n";
    KalmanParams params = optimize_kalman_parameters(log_prices1, log_prices2, alpha_init, beta_init);

    // Используем параметры по умолчанию P_alpha = 1e-4, P_beta = 1e-6 (заданы в конструкторе)
    KalmanFilter kf(params.R, params.Q_alpha, params.Q_beta, alpha_init, beta_init);
    SpreadAnalyzer analyzer(sym1, sym2, spread_window, spread_window, 2.0, 0.5);
    PortfolioSimulator portfolio(10000.0, 0.001, sym1, sym2);

    std::cout << "[4/4] Warming up filter and spread analyzer...\n";
    for (size_t i = 0; i < n; ++i) {
        kf.update(log_prices1[i], log_prices2[i]);
        analyzer.add_spread(kf.get_spread());
    }
    analyzer.reset_signal_counters();
    std::cout << "[4/4] Warm-up complete. Continuing with same filter state.\n";

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

        // --- Принудительное закрытие по возврату Z к нейтральному уровню ---
        if (portfolio.has_position() && std::abs(z) < 0.5) {
            std::cout << "[CLOSE] Z returned to neutral, closing position...\n";
            portfolio.close_at_market(price1, price2);
            portfolio.print_status(price1, price2);
            return;
        }

        // --- Открытие / переворот по сигналу ---
        if (signal.find("SELL") != std::string::npos ||
            signal.find("BUY")  != std::string::npos) {
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
