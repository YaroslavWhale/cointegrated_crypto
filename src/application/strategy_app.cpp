#include "application/strategy_app.hpp"
#include "data/rest_client.hpp"
#include "data/websocket_feed.hpp"
#include "filters/kalman_filter_2d.hpp"
#include "portfolio/simple_portfolio.hpp"
#include "analysis/spread_analyzer.hpp"
#include "strategy/pair_trading_strategy.hpp"
#include "utils/statistical_utils.hpp"
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <atomic>
#include <cmath>

static std::atomic<bool> running{true};
void signal_handler(int) { running = false; }

StrategyApplication::StrategyApplication(const AppConfig& cfg) : config_(cfg) {
    std::string q = cfg.quote;
    instr1_ = {cfg.symbol1 + q, cfg.symbol1, q};
    instr2_ = {cfg.symbol2 + q, cfg.symbol2, q};
}

std::unique_ptr<IStateEstimator> StrategyApplication::create_kalman() {
    BinanceRestClient rest;
    std::cout << "[1/3] Fetching historical data (" << config_.warmup_bars << " candles, "
              << config_.interval << ")...\n";
    auto closes1 = rest.fetch_klines(instr1_.normalized(), config_.interval, config_.warmup_bars);
    auto closes2 = rest.fetch_klines(instr2_.normalized(), config_.interval, config_.warmup_bars);

    if (closes1.size() < 2 || closes2.size() < 2)
        throw std::runtime_error("Not enough historical data");

    size_t n = std::min(closes1.size(), closes2.size());
    std::vector<double> log1(n), log2(n);
    for (size_t i = 0; i < n; ++i) {
        log1[i] = std::log(closes1[i]);
        log2[i] = std::log(closes2[i]);
    }

    std::cout << "[2/3] Optimizing 2D Kalman parameters...\n";
    OLSResult ols = compute_ols(log2, log1);
    KalmanParams params = optimize_kalman_2d(log1, log2, ols.alpha, ols.beta);

    std::cout << "[3/3] Creating 2D Kalman filter and warming up...\n";
    auto est = std::make_unique<KalmanFilter2D>(params.R, params.Q_alpha, params.Q_beta,
                                                ols.alpha, ols.beta, 1e-4, 1e-6);
    for (size_t i = 0; i < n; ++i)
        est->update(log1[i], log2[i]);

    std::cout << "2D Filter warmed up on " << n << " data points.\n";
    return est;
}

std::unique_ptr<ILiveDataFeed> StrategyApplication::create_feed() {
    return std::make_unique<BinanceWebSocketFeed>(config_.interval);
}

int StrategyApplication::run() {
    std::cout.setf(std::ios::unitbuf);
    std::signal(SIGINT, signal_handler);
    std::cout << "\n=== Pair Trading: " << instr1_.symbol
              << " / " << instr2_.symbol << " ===\n";
    std::cout << "Interval: " << config_.interval << "\n\n";

    auto estimator = create_kalman();

    double entry_multiplier = 2.0;
    double exit_multiplier  = 0.75;
    size_t vol_window       = 50;
    double vol_scale_factor = 0.5;

    auto analyzer = std::make_unique<SpreadAnalyzer>(
        config_.spread_window,
        config_.spread_window,
        entry_multiplier,
        exit_multiplier,
        config_.min_threshold,
        vol_window,
        vol_scale_factor
        );

    std::cout << "Warming up spread analyzer...\n";
    {
        BinanceRestClient rest;
        auto c1 = rest.fetch_klines(instr1_.normalized(), config_.interval, config_.spread_window);
        auto c2 = rest.fetch_klines(instr2_.normalized(), config_.interval, config_.spread_window);
        size_t n = std::min(c1.size(), c2.size());
        for (size_t i = 0; i < n; ++i) {
            estimator->update(std::log(c1[i]), std::log(c2[i]));
            analyzer->add_spread(estimator->get_spread());
        }
        analyzer->reset_signal_counters();
    }
    std::cout << "Spread analyzer ready.\n\n";

    auto portfolio = std::make_unique<SimplePortfolio>(config_.initial_balance,
                                                       config_.commission,
                                                       instr1_, instr2_);

    PairTradingStrategy strategy(std::move(estimator),
                                 std::move(analyzer),
                                 std::move(portfolio),
                                 instr1_, instr2_);

    auto feed = create_feed();
    feed->set_callback([&](double p1, double p2, uint64_t ts) {
        if (!running) return;
        strategy.on_price_update(p1, p2, ts);
    });
    feed->subscribe(instr1_.normalized(), instr2_.normalized());
    feed->start();

    std::cout << "\n=== STRATEGY IS LIVE ===\nPress Ctrl+C to stop\n\n";
    while (running) std::this_thread::sleep_for(std::chrono::milliseconds(200));
    feed->stop();
    std::cout << "Strategy stopped.\n";
    return 0;
}
