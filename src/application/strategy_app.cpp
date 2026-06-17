#include "application/strategy_app.hpp"
#include "data/rest_client.hpp"
#include "data/websocket_feed.hpp"
#include "filters/kalman_filter_3d.hpp"
#include "portfolio/simple_portfolio.hpp"
#include "utils/statistical_utils.hpp"
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <atomic>

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

    std::cout << "[2/3] Optimizing 3D Kalman parameters (with quadratic term)...\n";
    OLSResult ols = compute_ols(log2, log1);
    std::cout << "Initial OLS: alpha=" << ols.alpha << ", beta=" << ols.beta << "\n";

    double init_gamma = 0.0;
    KalmanParams3D params = optimize_kalman_3d(log1, log2, ols.alpha, ols.beta, init_gamma);

    std::cout << "[3/3] Creating 3D Kalman filter and warming up...\n";
    auto est = std::make_unique<KalmanFilter3D>(params.R,
                                                params.Q_alpha, params.Q_beta, params.Q_gamma,
                                                ols.alpha, ols.beta, init_gamma,
                                                1e-4, 1e-6, 1e-8);

    for (size_t i = 0; i < n; ++i)
        est->update(log1[i], log2[i]);

    std::cout << "3D Filter warmed up on " << n << " data points.\n";
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

    std::cout << "Warming up spread analyzer with filter history...\n";
    auto analyzer = std::make_unique<SpreadAnalyzer>(config_.spread_window,
                                                     config_.spread_window,
                                                     config_.threshold_mult,
                                                     config_.min_threshold);
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
                                 std::move(portfolio));

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
