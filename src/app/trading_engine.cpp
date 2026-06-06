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
#include <cmath>

void run_live_strategy(const std::string& sym1, const std::string& sym2,
                       int spread_window, int coint_check_minutes) {
    std::cout.setf(std::ios::unitbuf);

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

    // 1. Загрузка длинной истории (часовые свечи)
    const int hist_limit = 200;
    const std::string hist_interval = "1h";
    std::cout << "[1/4] Loading " << hist_limit << " " << hist_interval
              << " candles...\n";
    auto closes1_raw = RestClient::fetch_klines(pair1, hist_interval, hist_limit);
    auto closes2_raw = RestClient::fetch_klines(pair2, hist_interval, hist_limit);
    if (closes1_raw.size() < 100 || closes2_raw.size() < 100) {
        std::cerr << "Not enough historical data.\n";
        return;
    }
    std::cout << "[1/4] Loaded " << closes1_raw.size() << " and "
              << closes2_raw.size() << " closes\n";

    // Преобразование в логарифмы для коинтеграционного теста
    std::vector<double> log1, log2;
    log1.reserve(closes1_raw.size());
    log2.reserve(closes2_raw.size());
    for (double v : closes1_raw) log1.push_back(std::log(v));
    for (double v : closes2_raw) log2.push_back(std::log(v));

    // 2. Тест Энгла–Грейнджера на логарифмах
    std::cout << "[2/4] Testing for cointegration on log prices...\n";
    double alpha_eg, beta_eg, p_value;
    bool cointegrated = CointegrationTest::test(log1, log2, alpha_eg, beta_eg, p_value);
    if (!cointegrated) {
        std::cout << "[2/4] WARNING: pair is NOT cointegrated (p="
                  << p_value << ").\n";
        std::cout << "[2/4] Running with adaptive Kalman filter (alpha=0, beta=1).\n";
        alpha_eg = 0.0;
        beta_eg  = 1.0;
    } else {
        std::cout << "[2/4] Cointegrated (p=" << p_value
                  << "), alpha=" << alpha_eg << ", beta=" << beta_eg << "\n";
    }

    // 3. Инициализация торговых модулей
    std::cout << "[3/4] Initializing Kalman filter and spread analyzer...\n";
    // Для Калмана будем использовать логарифмы цен (спред = log(P1) - (alpha + beta*log(P2)))
    KalmanFilter kf(0.001, 0.0001, 0.0001, alpha_eg, beta_eg);
    // Внимание: alpha_eg, beta_eg получены на логарифмах, Калман тоже работает с логарифмами.
    SpreadAnalyzer analyzer(sym1, sym2, spread_window);

    // Буферы для часовых свечей (логарифмы)
    std::deque<double> hourly_log1, hourly_log2;
    double sum_log1 = 0.0, sum_log2 = 0.0;
    int minute_cnt = 0;
    uint64_t current_hour_start = 0; // временная метка начала текущего часа (в мс)
    size_t hour_cnt = 0;

    std::atomic<bool> active{true};
    std::unordered_map<std::string, double> latest_log_price; // будем хранить логарифмы
    int kline_cnt = 0;

    // 4. Запуск WebSocket
    std::cout << "[4/4] Starting WebSocket (minute klines)...\n";
    WebSocketPriceFeed feed;

    feed.set_callback([&](const std::string& symbol, double price, uint64_t close_time) {
        if (!active) return;

        // Преобразуем цену в логарифм
        double log_price = std::log(price);
        latest_log_price[symbol] = log_price;
        ++kline_cnt;
        if (kline_cnt % 10 == 0) {
            std::time_t now = std::time(nullptr);
            std::cout << "[LIVE] " << kline_cnt << " klines received ["
                      << std::ctime(&now) << "]";
        }

        if (!latest_log_price.count(pair1) || !latest_log_price.count(pair2))
            return;

        double log_p1 = latest_log_price[pair1];
        double log_p2 = latest_log_price[pair2];

        // Агрегация минутных логарифмических цен в часовые свечи по времени закрытия
        // close_time - Unix время в миллисекундах
        uint64_t hour_start = (close_time / 3600000) * 3600000; // начало часа

        if (current_hour_start == 0) {
            current_hour_start = hour_start;
            sum_log1 = 0.0; sum_log2 = 0.0; minute_cnt = 0;
        }

        if (hour_start == current_hour_start) {
            sum_log1 += log_p1;
            sum_log2 += log_p2;
            ++minute_cnt;
        } else {
            // Завершился предыдущий час – сохраняем среднее
            if (minute_cnt > 0) {
                double avg_log1 = sum_log1 / minute_cnt;
                double avg_log2 = sum_log2 / minute_cnt;
                hourly_log1.push_back(avg_log1);
                hourly_log2.push_back(avg_log2);
                if (hourly_log1.size() > 200) {
                    hourly_log1.pop_front();
                    hourly_log2.pop_front();
                }
                ++hour_cnt;

                // Периодическая проверка коинтеграции (без деления на ноль)
                if (coint_check_minutes > 0 &&
                    ((hour_cnt * 60) % coint_check_minutes == 0) &&
                    hourly_log1.size() >= 100) {
                    std::vector<double> h1(hourly_log1.begin(), hourly_log1.end());
                    std::vector<double> h2(hourly_log2.begin(), hourly_log2.end());
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
            // Начинаем новый час
            current_hour_start = hour_start;
            sum_log1 = log_p1;
            sum_log2 = log_p2;
            minute_cnt = 1;
        }

        // Обновление Калмана на логарифмах
        kf.update(log_p1, log_p2);
        double spread = kf.get_spread(); // логарифмический спред
        std::string signal = analyzer.add_spread(spread);
        double z = analyzer.get_z_score();

        // Вывод: для наглядности можно показывать реальные цены, но здесь выводим лог-цены
        std::cout << sym1 << "=" << std::exp(log_p1) << "  " << sym2 << "=" << std::exp(log_p2)
                  << "  | Log spread=" << spread
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
