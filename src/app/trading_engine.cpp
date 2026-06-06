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
#include <ctime>

void run_live_strategy(const std::string& sym1, const std::string& sym2,
                       int spread_window, int coint_check_minutes) {
    // Отключаем буферизацию вывода, чтобы всё сразу показывалось
    std::cout.setf(std::ios::unitbuf);

    std::cout << "\n=== Starting Pair Trading Strategy ===\n";
    std::cout << "Pair: " << sym1 << "/" << sym2 << std::endl;
    std::cout << "Spread window: " << spread_window << std::endl;
    std::cout << "Coint check every: " << coint_check_minutes << " minutes\n\n";

    // Преобразование коротких имён в биржевые пары
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

    // ─── 1. Загрузка длинной истории (дневные свечи) ───────────────
    const int hist_limit = 200;
    const std::string hist_interval = "1h";
    std::cout << "[1/4] Loading " << hist_limit << " " << hist_interval
              << " candles...\n";
    auto closes1 = RestClient::fetch_klines(pair1, hist_interval, hist_limit);
    auto closes2 = RestClient::fetch_klines(pair2, hist_interval, hist_limit);
    if (closes1.size() < 100 || closes2.size() < 100) {
        std::cerr << "Not enough historical data.\n";
        return;
    }
    std::cout << "[1/4] Loaded " << closes1.size() << " and "
              << closes2.size() << " closes\n";

    // ─── 2. Тест Энгла‑Грейнджера (только для информации) ─────────
    std::cout << "[2/4] Testing for cointegration on daily data...\n";
    double alpha_eg, beta_eg, p_value;
    bool cointegrated = CointegrationTest::test(closes1, closes2,
                                                alpha_eg, beta_eg, p_value);
    if (!cointegrated) {
        std::cout << "[2/4] WARNING: pair is NOT cointegrated (p="
                  << p_value << ").\n";
        std::cout << "[2/4] Running with adaptive Kalman filter "
                     "(alpha=0, beta=1).\n";
        alpha_eg = 0.0;
        beta_eg  = 1.0;
    } else {
        std::cout << "[2/4] Cointegrated (p=" << p_value
                  << "), alpha=" << alpha_eg << ", beta=" << beta_eg << "\n";
    }

    // ─── 3. Инициализация торговых модулей ─────────────────────────
    std::cout << "[3/4] Initializing Kalman filter and spread analyzer...\n";
    KalmanFilter kf(0.001, 0.0001, 0.0001, alpha_eg, beta_eg);
    SpreadAnalyzer analyzer(sym1, sym2, spread_window);

    // Буферы для агрегации минутных свечей в часовые
    std::deque<double> hourly1, hourly2;
    double sum1 = 0.0, sum2 = 0.0;
    int minute_cnt = 0;
    size_t hour_cnt = 0;

    std::atomic<bool> active{true};
    std::unordered_map<std::string, double> latest_price;
    int kline_cnt = 0;

    // ─── 4. Запуск WebSocket ────────────────────────────────────────
    std::cout << "[4/4] Starting WebSocket (minute klines)...\n";
    WebSocketPriceFeed feed;

    feed.set_callback([&](const std::string& symbol, double price) {
        if (!active) return;

        latest_price[symbol] = price;
        ++kline_cnt;
        if (kline_cnt % 10 == 0) {
            std::time_t now = std::time(nullptr);
            std::cout << "[LIVE] " << kline_cnt << " klines received ["
                      << std::ctime(&now) << "]";
        }

        // Ждём цену для обеих монет
        if (!latest_price.count(pair1) || !latest_price.count(pair2))
            return;

        double p1 = latest_price[pair1];
        double p2 = latest_price[pair2];

        // Агрегируем минутные цены в часовые свечи (усреднение)
        sum1 += p1;
        sum2 += p2;
        ++minute_cnt;

        if (minute_cnt == 60) {
            double avg1 = sum1 / 60.0;
            double avg2 = sum2 / 60.0;
            hourly1.push_back(avg1);
            hourly2.push_back(avg2);

            // Храним только последние 200 часов
            if (hourly1.size() > 200) {
                hourly1.pop_front();
                hourly2.pop_front();
            }

            sum1 = sum2 = 0.0;
            minute_cnt = 0;
            ++hour_cnt;

            // Периодическая проверка коинтеграции
            if (coint_check_minutes > 0 &&
                (hour_cnt % (coint_check_minutes / 60) == 0) &&
                hourly1.size() >= 100) {
                std::vector<double> h1(hourly1.begin(), hourly1.end());
                std::vector<double> h2(hourly2.begin(), hourly2.end());
                double a, b, p;
                if (CointegrationTest::test(h1, h2, a, b, p)) {
                    std::cout << "[COINT] Cointegration restored (p="
                              << p << "), updating parameters.\n";
                    kf.reset(a, b);
                } else {
                    std::cout << "[COINT] Still not cointegrated (p="
                              << p << ").\n";
                }
            }
        }

        // Генерация сигналов
        kf.update(p1, p2);
        double spread = kf.get_spread();
        std::string signal = analyzer.add_spread(spread);
        double z = analyzer.get_z_score();

        std::cout << sym1 << "=" << p1 << "  " << sym2 << "=" << p2
                  << "  | Spread=" << spread
                  << "  | Z=" << z
                  << "  | " << signal << std::endl;
    });

    feed.subscribe(pair1 + "@kline_1m");
    feed.subscribe(pair2 + "@kline_1m");

    std::cout << "\n=== STRATEGY IS LIVE ===\n";
    std::cout << "Press Ctrl+C to stop\n\n";
    feed.run();

    std::cout << "Strategy terminated.\n";
}
