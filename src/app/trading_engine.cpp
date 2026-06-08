#include "app/trading_engine.h"
#include "domain/kalman_filter.h"
#include "domain/spread_analyzer.h"
#include "domain/cointegration_test.h"
#include "data/rest_client.h"
#include "data/websocket_client.h"
#include <iostream>
#include <unordered_map>
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
                       int spread_window, int coint_check_minutes) {
    std::cout.setf(std::ios::unitbuf);
    std::signal(SIGINT, signal_handler);

    std::cout << "\n=== Starting Pair Trading Strategy ===\n";
    std::cout << "Pair: " << sym1 << "/" << sym2 << std::endl;
    std::cout << "Spread window: " << spread_window << std::endl;
    std::cout << "Coint check every: " << coint_check_minutes << " minutes\n\n";

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

    const int hist_limit = 200;
    const std::string hist_interval = "1h";
    std::cout << "[1/4] Loading " << hist_limit << " " << hist_interval << " candles...\n";
    auto closes1_raw = RestClient::fetch_klines(pair1, hist_interval, hist_limit);
    auto closes2_raw = RestClient::fetch_klines(pair2, hist_interval, hist_limit);
    if (closes1_raw.size() < 100 || closes2_raw.size() < 100) {
        std::cerr << "Not enough historical data.\n";
        return;
    }

    std::vector<double> log1, log2;
    log1.reserve(closes1_raw.size());
    log2.reserve(closes2_raw.size());
    for (double v : closes1_raw) log1.push_back(std::log(v));
    for (double v : closes2_raw) log2.push_back(std::log(v));

    std::cout << "[2/4] Testing for cointegration on log prices...\n";
    double alpha_eg, beta_eg, p_value;
    bool cointegrated = CointegrationTest::test(log1, log2, alpha_eg, beta_eg, p_value);
    if (!cointegrated) {
        std::cout << "[2/4] WARNING: not cointegrated (p=" << p_value << "). Using alpha=0, beta=1.\n";
        alpha_eg = 0.0; beta_eg = 1.0;
    } else {
        std::cout << "[2/4] Cointegrated (p=" << p_value << "), alpha=" << alpha_eg << ", beta=" << beta_eg << "\n";
    }

    std::cout << "[3/4] Initializing Kalman filter and spread analyzer...\n";
    KalmanFilter kf(0.001, 0.0001, 0.0001, alpha_eg, beta_eg);
    SpreadAnalyzer analyzer(sym1, sym2, spread_window);

    std::deque<double> hourly_log1, hourly_log2;
    double sum_log1 = 0.0, sum_log2 = 0.0;
    int minute_cnt = 0;
    uint64_t current_hour_start = 0;
    uint64_t last_coint_check_ms = 0;

    struct Kline { double log_price; uint64_t close_time; };
    std::unordered_map<std::string, Kline> pending_klines;

    std::cout << "[4/4] Starting WebSocket (minute klines)...\n";
    WebSocketPriceFeed feed;

    feed.set_callback([&](const std::string& symbol, double price, uint64_t close_time) {
        if (!active) return;

        pending_klines[symbol] = {std::log(price), close_time};

        if (pending_klines.count(pair1) && pending_klines.count(pair2)) {
            const auto& k1 = pending_klines[pair1];
            const auto& k2 = pending_klines[pair2];
            if (k1.close_time == k2.close_time) {
                double log_p1 = k1.log_price, log_p2 = k2.log_price;
                uint64_t ts = k1.close_time;

                uint64_t hour_start = (ts / 3600000) * 3600000;
                if (current_hour_start == 0) {
                    current_hour_start = hour_start;
                    sum_log1 = sum_log2 = 0.0;
                    minute_cnt = 0;
                }
                if (hour_start == current_hour_start) {
                    sum_log1 += log_p1; sum_log2 += log_p2;
                    ++minute_cnt;
                } else {
                    if (minute_cnt > 0) {
                        hourly_log1.push_back(sum_log1 / minute_cnt);
                        hourly_log2.push_back(sum_log2 / minute_cnt);
                        if (hourly_log1.size() > 200) {
                            hourly_log1.pop_front();
                            hourly_log2.pop_front();
                        }
                    }
                    current_hour_start = hour_start;
                    sum_log1 = log_p1; sum_log2 = log_p2;
                    minute_cnt = 1;
                }

                if (coint_check_minutes > 0 && hourly_log1.size() >= 100) {
                    if (last_coint_check_ms == 0)
                        last_coint_check_ms = ts;
                    else if (ts - last_coint_check_ms >= static_cast<uint64_t>(coint_check_minutes) * 60000) {
                        std::vector<double> h1(hourly_log1.begin(), hourly_log1.end());
                        std::vector<double> h2(hourly_log2.begin(), hourly_log2.end());
                        double a, b, p;
                        if (CointegrationTest::test(h1, h2, a, b, p)) {
                            std::cout << "[COINT] Cointegration restored (p=" << p << "), updating parameters.\n";
                            kf.reset(a, b);
                        } else {
                            std::cout << "[COINT] Not cointegrated (p=" << p << ").\n";
                        }
                        last_coint_check_ms = ts;
                    }
                }

                kf.update(log_p1, log_p2);
                double spread = kf.get_spread();
                std::string signal = analyzer.add_spread(spread);
                double z = analyzer.get_z_score();

                std::cout << sym1 << "=" << std::exp(log_p1) << "  "
                          << sym2 << "=" << std::exp(log_p2)
                          << "  | Log spread=" << spread
                          << "  | Z=" << z
                          << "  | " << signal << std::endl;

                pending_klines.erase(pair1);
                pending_klines.erase(pair2);
            }
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
