#ifndef TRADING_PAIR_H
#define TRADING_PAIR_H

#include <vector>
#include <cmath>
#include <algorithm>

class TradingPair {
public:
    TradingPair(int reg_window = 100, int spr_window = 50)
        : regression_window(reg_window), spread_window(spr_window),
        hedge_ratio(1.0), current_spread(0.0), mean_spread(0.0),
        stddev_spread(0.0), z_score(0.0) {}

    // Загрузить историю цен (синхронизированные векторы одинаковой длины)
    void load_history(const std::vector<double>& prices1,
                      const std::vector<double>& prices2) {
        price1_history = prices1;
        price2_history = prices2;
        // Обрезаем до нужной длины
        if ((int)price1_history.size() > regression_window) {
            price1_history.erase(price1_history.begin(),
                                  price1_history.end() - regression_window);
            price2_history.erase(price2_history.begin(),
                                  price2_history.end() - regression_window);
        }
        recalc_all();
    }

   /* // Добавить новую пару цен (для потокового режима)
    void add_prices(double price1, double price2) {
        price1_history.push_back(price1);
        price2_history.push_back(price2);
        if ((int)price1_history.size() > regression_window) {
            price1_history.erase(price1_history.begin());
            price2_history.erase(price2_history.begin());
        }
        recalc_all();
    }
*/
    // Пересчитать коэффициент β (линейная регрессия) по текущей истории
    void update_beta() {
        size_t n = price1_history.size();
        if (n < 10) return;

        double sum_x = 0.0, sum_y = 0.0;
        for (size_t i = 0; i < n; ++i) {
            sum_x += price1_history[i];
            sum_y += price2_history[i];
        }
        double mean_x = sum_x / n;
        double mean_y = sum_y / n;

        double numerator = 0.0, denominator = 0.0;
        for (size_t i = 0; i < n; ++i) {
            double dx = price1_history[i] - mean_x;
            double dy = price2_history[i] - mean_y;
            numerator += dx * dy;
            denominator += dx * dx;
        }
        if (denominator != 0.0) {
            hedge_ratio = numerator / denominator;
        }
        recalc_spreads(); // пересчитываем спреды с новым β
    }

    // Геттеры
    double get_hedge_ratio() const { return hedge_ratio; }
    double get_current_spread() const { return current_spread; }
    double get_mean_spread() const { return mean_spread; }
    double get_stddev_spread() const { return stddev_spread; }
    double get_z_score() const { return z_score; }
    int get_history_size() const { return price1_history.size(); }
    int get_spread_history_size() const { return spread_history.size(); }

private:
    std::vector<double> price1_history;
    std::vector<double> price2_history;
    int regression_window;
    int spread_window;

    double hedge_ratio;
    std::vector<double> spread_history;
    double current_spread;
    double mean_spread;
    double stddev_spread;
    double z_score;

    void recalc_spreads() {
        spread_history.clear();
        size_t n = std::min(price1_history.size(), price2_history.size());
        for (size_t i = 0; i < n; ++i) {
            double spread = price1_history[i] - hedge_ratio * price2_history[i];
            spread_history.push_back(spread);
        }
        // Оставляем только последние spread_window значений
        if ((int)spread_history.size() > spread_window) {
            spread_history.erase(spread_history.begin(),
                                 spread_history.begin() + (spread_history.size() - spread_window));
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

        if (!spread_history.empty())
            current_spread = spread_history.back();
        else
            current_spread = 0.0;

        if (stddev_spread != 0.0)
            z_score = (current_spread - mean_spread) / stddev_spread;
        else
            z_score = 0.0;
    }

    void recalc_all() {
        update_beta();       // сначала β, потом спреды (update_beta вызывает recalc_spreads)
    }
};

#endif // TRADING_PAIR_H
