#include "domain/spread_analyzer.h"
#include <numeric>
#include <cmath>
#include <iostream>

SpreadAnalyzer::SpreadAnalyzer(const std::string& sym1, const std::string& sym2,
                               size_t window_size)
    : sym1_(sym1), sym2_(sym2), window_size_(window_size) {}

double SpreadAnalyzer::compute_z_for_spread(double spread) const {
    if (spread_buffer_.size() < 2) return 0.0;
    double sum = std::accumulate(spread_buffer_.begin(), spread_buffer_.end(), 0.0);
    double mean = sum / spread_buffer_.size();
    double sq_sum = 0.0;
    for (double s : spread_buffer_) {
        double d = s - mean;
        sq_sum += d * d;
    }
    double stddev = std::sqrt(sq_sum / spread_buffer_.size());
    if (stddev < 1e-8) return 0.0;
    return (spread - mean) / stddev;
}

std::string SpreadAnalyzer::add_spread(double spread) {
    spread_buffer_.push_back(spread);
    if (spread_buffer_.size() > window_size_)
        spread_buffer_.pop_front();

    double z = compute_z_for_spread(spread);

    if (std::abs(z) > 2.0)
        ++consecutive_exceed_;
    else
        consecutive_exceed_ = 0;

    return signal_from_z(z);
}

double SpreadAnalyzer::get_z_score() const {
    if (spread_buffer_.empty()) return 0.0;
    return compute_z_for_spread(spread_buffer_.back());
}

std::string SpreadAnalyzer::signal_from_z(double z) const {
    if (spread_buffer_.size() < window_size_)
        return "CALIBRATING (need " + std::to_string(window_size_ - spread_buffer_.size()) + ")";

    if (consecutive_exceed_ >= REQUIRED_CONSECUTIVE) {
        if (z > 2.0)  return "SELL " + sym1_ + " / BUY " + sym2_;
        if (z < -2.0) return "BUY " + sym1_ + " / SELL " + sym2_;
    }

    if (z > 2.0)  return sym1_ + " overpriced (need confirm)";
    if (z < -2.0) return sym2_ + " overpriced (need confirm)";
    if (z > 1.0)  return sym1_ + " slightly overpriced";
    if (z < -1.0) return sym2_ + " slightly overpriced";
    return "Balanced";
}

void SpreadAnalyzer::reset() {
    spread_buffer_.clear();
    consecutive_exceed_ = 0;
}
