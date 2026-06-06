#include "domain/spread_analyzer.h"
#include <numeric>
#include <cmath>
#include <algorithm>

SpreadAnalyzer::SpreadAnalyzer(const std::string& sym1, const std::string& sym2,
                               size_t window_size)
    : sym1_(sym1), sym2_(sym2), window_size_(window_size) {}

std::string SpreadAnalyzer::add_spread(double spread) {
    // Сначала вычисляем z‑score по текущему буферу (без нового значения)
    double z = 0.0;
    if (spread_buffer_.size() >= window_size_ && window_size_ > 1) {
        z = compute_z_score(spread_buffer_.back()); // по последнему элементу старого окна?
        // Нет, мы хотим z для нового наблюдения, но на основе статистик без него.
        // Поэтому вычисляем среднее и std по spread_buffer_ без нового spread.
        double sum = std::accumulate(spread_buffer_.begin(), spread_buffer_.end(), 0.0);
        double mean = sum / spread_buffer_.size();
        double sq_sum = 0.0;
        for (double s : spread_buffer_) sq_sum += (s - mean) * (s - mean);
        double stddev = std::sqrt(sq_sum / spread_buffer_.size());
        if (stddev > 0) z = (spread - mean) / stddev;
    }

    // Добавляем новое значение и поддерживаем размер окна
    spread_buffer_.push_back(spread);
    if (spread_buffer_.size() > window_size_)
        spread_buffer_.pop_front();

    // Сглаживание гистерезисом
    if (std::abs(z - last_z_score_) < HYSTERESIS) {
        consecutive_signals_++;
    } else {
        consecutive_signals_ = 0;
    }
    last_z_score_ = z;

    return signal_from_z_filtered(z);
}

double SpreadAnalyzer::get_z_score() const {
    if (spread_buffer_.empty()) return 0.0;
    // Для внешнего запроса возвращаем z последнего спреда по окну (уже с ним внутри)
    double last = spread_buffer_.back();
    double sum = std::accumulate(spread_buffer_.begin(), spread_buffer_.end(), 0.0);
    double mean = sum / spread_buffer_.size();
    double sq_sum = 0.0;
    for (double s : spread_buffer_) sq_sum += (s - mean) * (s - mean);
    double stddev = std::sqrt(sq_sum / spread_buffer_.size());
    if (stddev == 0.0) return 0.0;
    return (last - mean) / stddev;
}

double SpreadAnalyzer::compute_z_score(double current_spread) const {
    // Этот метод больше не используется внутри, оставлен для совместимости
    if (spread_buffer_.size() < 2) return 0.0;
    double sum = std::accumulate(spread_buffer_.begin(), spread_buffer_.end(), 0.0);
    double mean = sum / spread_buffer_.size();
    double sq_sum = 0.0;
    for (double s : spread_buffer_) sq_sum += (s - mean) * (s - mean);
    double stddev = std::sqrt(sq_sum / spread_buffer_.size());
    if (stddev == 0.0) return 0.0;
    return (current_spread - mean) / stddev;
}

std::string SpreadAnalyzer::signal_from_z_filtered(double z) {
    if (spread_buffer_.size() < window_size_) {
        return "CALIBRATING (need " + std::to_string(window_size_ - spread_buffer_.size()) + ")";
    }

    if (consecutive_signals_ < REQUIRED_CONSECUTIVE) {
        return "WAITING_CONFIRMATION";
    }

    if (z > 2.0)  return "SELL " + sym1_ + " / BUY " + sym2_;
    if (z < -2.0) return "BUY " + sym1_ + " / SELL " + sym2_;
    if (z > 1.0)  return sym1_ + " overpriced";
    if (z < -1.0) return sym2_ + " overpriced";
    return "Balanced";
}

void SpreadAnalyzer::reset() {
    spread_buffer_.clear();
    last_z_score_ = 0.0;
    consecutive_signals_ = 0;
}
