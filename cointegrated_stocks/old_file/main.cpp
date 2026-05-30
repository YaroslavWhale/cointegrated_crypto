#include <iostream>
#include <deque>
#include <cmath>
#include "api_client.h"
#include "kalman_filter.h"   // ваш заголовочный файл с фильтром

// Функция для вычисления Z‑score по буферу спредов
double compute_z_score(const std::deque<double>& spreads, double current_spread) {
    if (spreads.size() < 30) return 0.0;  // недостаточно данных
    double sum = 0.0;
    for (double s : spreads) sum += s;
    double mean = sum / spreads.size();
    double sq_sum = 0.0;
    for (double s : spreads) {
        double d = s - mean;
        sq_sum += d * d;
    }
    double stddev = std::sqrt(sq_sum / spreads.size());
    if (stddev == 0.0) return 0.0;
    return (current_spread - mean) / stddev;
}

int main(int argc, char* argv[]) {
    std::string sym1 = "BTC";
    std::string sym2 = "ETH";
    if (argc >= 3) {
        sym1 = argv[1];
        sym2 = argv[2];
    }

    std::string interval = "1d";
    int limit = 100;

    // 1. Загружаем цены
    std::vector<double> prices1 = fetch_historical_prices(sym1, interval, limit);
    std::vector<double> prices2 = fetch_historical_prices(sym2, interval, limit);
    if (prices1.empty() || prices2.empty()) return 1;

    size_t n = std::min(prices1.size(), prices2.size());
    prices1.resize(n);
    prices2.resize(n);
    if (n < 50) return 1;

    // 2. Создаём фильтр Калмана
    //    R (шум измерения) – оцениваем по дисперсии остатков (например, 0.001)
    //    Q_alpha, Q_beta – маленькие (коэффициенты меняются медленно)
    double R = 0.001;
    double Q_alpha = 0.0001;
    double Q_beta = 0.0001;
    KalmanFilter kf(R, Q_alpha, Q_beta, 0.0, 1.0, 100.0, 100.0);

    // 3. Буфер для спредов (скользящее окно, например, 50 последних значений)
    std::deque<double> spread_buffer;
    const size_t SPREAD_WINDOW = 50;

    // 4. Проход по всем точкам (имитация реального времени)
    std::cout << "Time\tPrice1\tPrice2\tAlpha\tBeta\tSpread\tZ-score\n";
    for (size_t i = 0; i < n; ++i) {
        double p1 = prices1[i];
        double p2 = prices2[i];

        // Обновляем фильтр новой парой
        kf.update(p1, p2);

        // Текущий спред (можно взять из get_spread, но лучше использовать y – инновацию,
        // которая уже сохранена во временной переменной update? Она локальна. Просто вызовем get_spread.
        double spread = kf.get_spread(p1, p2);

        // Добавляем в буфер
        spread_buffer.push_back(spread);
        if (spread_buffer.size() > SPREAD_WINDOW) {
            spread_buffer.pop_front();
        }

        // Вычисляем Z‑score
        double z_score = compute_z_score(spread_buffer, spread);

        // Выводим отладочную информацию (первые 30 точек – калибровка)
        std::cout << i << "\t" << p1 << "\t" << p2 << "\t"
                  << kf.get_alpha() << "\t" << kf.get_beta() << "\t"
                  << spread << "\t" << z_score << "\n";

        // Генерация сигнала (как в вашей старой функции generate_signal, только с z_score)
        if (i < 30) {
            std::cout << "   [CALIBRATING]\n";
        } else if (z_score > 2.0) {
            std::cout << "   🔴 SELL " << sym1 << ", BUY " << sym2 << "\n";
        } else if (z_score < -2.0) {
            std::cout << "   🟢 BUY " << sym1 << ", SELL " << sym2 << "\n";
        } else if (z_score > 1.0) {
            std::cout << "   📈 " << sym1 << " overpriced\n";
        } else if (z_score < -1.0) {
            std::cout << "   📉 " << sym2 << " overpriced\n";
        } else {
            std::cout << "   ✅ Balanced\n";
        }
    }

    return 0;
}
