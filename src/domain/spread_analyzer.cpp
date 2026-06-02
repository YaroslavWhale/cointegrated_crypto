#include "domain/spread_analyzer.h"
#include <numeric>
#include <cmath>

SpreadAnalyzer::SpreadAnalyzer(const std::string& sym1, const std::string& sym2,
                               size_t window_size)
    : sym1_(sym1), sym2_(sym2), window_size_(window_size) {}

std::string SpreadAnalyzer::add_spread(double spread) {
    spread_buffer_.push_back(spread);
    if (spread_buffer_.size() > window_size_)
        spread_buffer_.pop_front();

    double z = compute_z_score(spread);
    return signal_from_z(z, spread_buffer_.size());
}

double SpreadAnalyzer::get_z_score() const {
    if (spread_buffer_.empty()) return 0.0;
    return compute_z_score(spread_buffer_.back());
}

double SpreadAnalyzer::compute_z_score(double current_spread) const {
    if (spread_buffer_.size() < 30) return 0.0;
    double sum = std::accumulate(spread_buffer_.begin(), spread_buffer_.end(), 0.0);
    double mean = sum / spread_buffer_.size();

    double sq_sum = 0.0;
    for (double s : spread_buffer_) {
        double d = s - mean;
        sq_sum += d * d;
    }
    double stddev = std::sqrt(sq_sum / spread_buffer_.size());
    if (stddev == 0.0) return 0.0;
    return (current_spread - mean) / stddev;
}

std::string SpreadAnalyzer::signal_from_z(double z, size_t steps_done) const {
    if (steps_done < 30) {
        return "CALIBRATING (need " + std::to_string(30 - steps_done) + ")";
    }
    if (z > 2.0)  return "SELL " + sym1_ + " / BUY " + sym2_;
    if (z < -2.0) return "BUY " + sym1_ + " / SELL " + sym2_;
    if (z > 1.0)  return sym1_ + " overpriced";
    if (z < -1.0) return sym2_ + " overpriced";
    return "Balanced";
}

void SpreadAnalyzer::reset() {
    spread_buffer_.clear();
}
