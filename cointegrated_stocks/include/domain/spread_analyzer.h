#pragma once
#include <deque>
#include <string>

// Анализирует буфер спредов: вычисляет Z‑score и генерирует торговые сигналы
class SpreadAnalyzer {
public:
    // Конструктор: sym1 и sym2 нужны только для текстового описания сигналов
    SpreadAnalyzer(const std::string& sym1, const std::string& sym2,
                   size_t window_size = 50);

    // Добавить новое значение спреда и вернуть текстовый сигнал
    // Если данных недостаточно, возвращает калибровочное сообщение
    std::string add_spread(double spread);

    // Принудительно получить текущий Z‑score (0.0 если данных мало)
    double get_z_score() const;

    // Сбросить накопленный буфер
    void reset();

private:
    std::string sym1_;
    std::string sym2_;
    size_t window_size_;
    std::deque<double> spread_buffer_;

    double compute_z_score(double current_spread) const;
    std::string signal_from_z(double z, size_t steps_done) const;
};
