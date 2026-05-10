#ifndef TRADINGPAIR_H
#define TRADINGPAIR_H

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

class TradingPair {
private:
    std::string id1;
    std::string id2;

    double hedge_ratio;                  // β

    std::vector<double> price1_history;
    std::vector<double> price2_history;
    int regression_window;               // окно для расчёта β

    std::vector<double> spread_history;
    int spread_window;                   // окно для статистики спреда

    double current_spread;
    double mean_spread;
    double stddev_spread;
    double z_score;

    // Пересчёт всей истории спредов с текущим β
    void recalc_spreads_from_history() {
        spread_history.clear();
        size_t n = std::min(price1_history.size(), price2_history.size());
        for (size_t i = 0; i < n; ++i) {
            double spread = price1_history[i] - hedge_ratio * price2_history[i];
            spread_history.push_back(spread);
        }
        // Оставляем только последние spread_window значений
        while ((int)spread_history.size() > spread_window) {
            spread_history.erase(spread_history.begin());
        }
        update_statistics();
    }

    void update_statistics() {
        if (spread_history.empty()) return;

        double sum = 0.0;
        for (double s : spread_history) sum += s;
        mean_spread = sum / spread_history.size();

        double sq_sum = 0.0;
        for (double s : spread_history) {
            double d = s - mean_spread;
            sq_sum += d * d;
        }
        stddev_spread = std::sqrt(sq_sum / spread_history.size());

        if (stddev_spread != 0.0)
            z_score = (current_spread - mean_spread) / stddev_spread;
        else
            z_score = 0.0;
    }

public:
    // Конструктор – ИСПРАВЛЕН
    TradingPair(const std::string& id1_, const std::string& id2_,
                int reg_window = 100, int spr_window = 50)
        : id1(id1_), id2(id2_), hedge_ratio(1.0),
        regression_window(reg_window), spread_window(spr_window),
        current_spread(0.0), mean_spread(0.0), stddev_spread(0.0), z_score(0.0) {}

    // Обновление коэффициента хеджирования β через линейную регрессию
    void update_beta() {
        size_t n = std::min(price1_history.size(), price2_history.size());
        if (n < 10) return;

        size_t start = (n > (size_t)regression_window) ? n - regression_window : 0;
        size_t count = n - start;

        double sum_x = 0.0, sum_y = 0.0;
        for (size_t i = start; i < n; ++i) {
            sum_x += price1_history[i];
            sum_y += price2_history[i];
        }
        double mean_x = sum_x / count;
        double mean_y = sum_y / count;

        double numerator = 0.0, denominator = 0.0;
        for (size_t i = start; i < n; ++i) {
            double dx = price1_history[i] - mean_x;
            double dy = price2_history[i] - mean_y;
            numerator += dx * dy;
            denominator += dx * dx;
        }

        if (denominator != 0.0) {
            hedge_ratio = numerator / denominator;
            recalc_spreads_from_history();   // пересчитываем спреды с новой β
        }
    }

    void set_hedge_ratio(double beta) {
        hedge_ratio = beta;
        recalc_spreads_from_history();
    }

    double get_hedge_ratio() const { return hedge_ratio; }

    // Добавление новой пары цен (главный метод)
    void add_prices(double price1, double price2) {
        price1_history.push_back(price1);
        price2_history.push_back(price2);

        // Ограничиваем длину истории цен (для регрессии)
        while ((int)price1_history.size() > regression_window) {
            price1_history.erase(price1_history.begin());
            price2_history.erase(price2_history.begin());
        }

        current_spread = price1 - hedge_ratio * price2;
        spread_history.push_back(current_spread);

        // Ограничиваем историю спредов
        while ((int)spread_history.size() > spread_window) {
            spread_history.erase(spread_history.begin());
        }

        update_statistics();
    }

    //вынести в отдельный заголовочник
    // Генерация торгового сигнала на основе Z‑score
    std::string get_signal() const {
        if ((int)spread_history.size() < spread_window) {
            return "CALIBRATING... Need " +
                   std::to_string(spread_window - spread_history.size()) +
                   " more observations";
        }

        if (z_score > 2.0) {
            return "🔴 SELL " + id1 + ", BUY " + id2 +
                   " (Z=" + std::to_string(z_score) + ")";
        }
        if (z_score < -2.0) {
            return "🟢 BUY " + id1 + ", SELL " + id2 +
                   " (Z=" + std::to_string(z_score) + ")";
        }
        if (z_score > 1.0) {
            return "📈 " + id1 + " overpriced (Z=" + std::to_string(z_score) + ")";
        }
        if (z_score < -1.0) {
            return "📉 " + id2 + " overpriced (Z=" + std::to_string(z_score) + ")";
        }
        return "✅ Balanced (Z=" + std::to_string(z_score) + ")";
    }

    // Геттеры для отладки
    double get_current_spread() const { return current_spread; }
    double get_mean_spread()    const { return mean_spread; }
    double get_stddev_spread()  const { return stddev_spread; }
    double get_z_score()        const { return z_score; }
    int get_spread_history_size() const { return spread_history.size(); }
    int get_price_history_size() const { return price1_history.size(); }
};

#endif // TRADINGPAIR_H
