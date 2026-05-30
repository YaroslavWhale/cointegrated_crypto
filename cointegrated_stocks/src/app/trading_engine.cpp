#include "app/trading_engine.h"
#include "data/api_client.h"
#include "domain/kalman_filter.h"
#include "domain/spread_analyzer.h"
#include <iostream>
#include <algorithm>

void run_strategy(const std::string& sym1,
                  const std::string& sym2,
                  int spread_window,
                  const std::string& interval,
                  int limit) {
    // 1. Загрузка данных
    auto prices1 = fetch_historical_prices(sym1, interval, limit);
    auto prices2 = fetch_historical_prices(sym2, interval, limit);
    if (prices1.empty() || prices2.empty()) {
        std::cerr << "Failed to load data. Exiting." << std::endl;
        return;
    }

    size_t n = std::min(prices1.size(), prices2.size());
    prices1.resize(n);
    prices2.resize(n);
    if (n < 30) {
        std::cerr << "Not enough data points (" << n << "), need at least 30." << std::endl;
        return;
    }

    // 2. Инициализация моделей
    double R = 0.001;
    double Q_alpha = 0.0001;
    double Q_beta  = 0.0001;
    KalmanFilter kf(R, Q_alpha, Q_beta);
    SpreadAnalyzer analyzer(sym1, sym2, spread_window);

    // 3. Цикл обработки
    std::cout << "Step\tPrice1\tPrice2\tAlpha\tBeta\tSpread\tZ-score\tSignal\n";
    for (size_t i = 0; i < n; ++i) {
        double p1 = prices1[i];
        double p2 = prices2[i];

        kf.update(p1, p2);
        double spread = kf.get_spread();

        std::string signal = analyzer.add_spread(spread);
        double z = analyzer.get_z_score();

        std::cout << i << "\t" << p1 << "\t" << p2 << "\t"
                  << kf.get_alpha() << "\t" << kf.get_beta() << "\t"
                  << spread << "\t" << z << "\t" << signal << "\n";
    }
}
