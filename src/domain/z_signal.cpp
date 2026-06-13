#include "domain/z_signal.h"
#include <numeric>
#include <cmath>
#include <iostream>

SpreadAnalyzer::SpreadAnalyzer(const std::string& sym1, const std::string& sym2,
                               size_t spread_window_size,
                               size_t z_window_size,
                               double threshold_multiplier,
                               double min_threshold)
    : sym1_(sym1), sym2_(sym2),
    spread_window_size_(spread_window_size),
    z_window_size_(z_window_size),
    threshold_multiplier_(threshold_multiplier),
    min_threshold_(min_threshold)
{}

double SpreadAnalyzer::compute_z_for_spread(double spread) const {
    if (spread_buffer_.size() < 2) return 0.0;
    double sum = std::accumulate(spread_buffer_.begin(), spread_buffer_.end(), 0.0);
    double mean = sum / spread_buffer_.size();
    double sq_sum = 0.0;
    for (double s : spread_buffer_) {
        double d = s - mean;
        sq_sum += d * d;
    }
    double stddev = std::sqrt(sq_sum / (spread_buffer_.size() - 1));
    if (stddev < 1e-8) return 0.0;
    return (spread - mean) / stddev;
}

double SpreadAnalyzer::compute_current_threshold() const {
    if (z_history_.size() < 2) return min_threshold_;

    double sum = std::accumulate(z_history_.begin(), z_history_.end(), 0.0);
    double mean = sum / z_history_.size();
    double sq_sum = 0.0;
    for (double z : z_history_) {
        double d = z - mean;
        sq_sum += d * d;
    }
    double std_z = std::sqrt(sq_sum / (z_history_.size() - 1));
    double raw_threshold = threshold_multiplier_ * std_z;
    return std::max(min_threshold_, raw_threshold);
}

std::string SpreadAnalyzer::add_spread(double spread) {
    double z = 0.0;
    if (spread_buffer_.size() >= 2) {
        z = compute_z_for_spread(spread);
    }

    double threshold = compute_current_threshold();

    z_history_.push_back(z);
    if (z_history_.size() > z_window_size_)
        z_history_.pop_front();

    if (z > threshold) {
        if (prev_z_sign_ > 0) consecutive_exceed_++;
        else consecutive_exceed_ = 1;
        prev_z_sign_ = 1;
    } else if (z < -threshold) {
        if (prev_z_sign_ < 0) consecutive_exceed_++;
        else consecutive_exceed_ = 1;
        prev_z_sign_ = -1;
    } else {
        consecutive_exceed_ = 0;
        prev_z_sign_ = 0;
    }

    spread_buffer_.push_back(spread);
    if (spread_buffer_.size() > spread_window_size_)
        spread_buffer_.pop_front();

    return signal_from_z(z, threshold);
}

double SpreadAnalyzer::get_z_score() const {
    if (spread_buffer_.empty()) return 0.0;
    return compute_z_for_spread(spread_buffer_.back());
}

double SpreadAnalyzer::get_current_threshold() const {
    return compute_current_threshold();
}

std::string SpreadAnalyzer::signal_from_z(double z, double threshold) const {
    if (spread_buffer_.size() < spread_window_size_)
        return "CALIBRATING (need " + std::to_string(spread_window_size_ - spread_buffer_.size()) + ")";

    if (consecutive_exceed_ >= REQUIRED_CONSECUTIVE) {
        if (z > threshold)  return "SELL " + sym1_ + " / BUY " + sym2_;
        if (z < -threshold) return "BUY " + sym1_ + " / SELL " + sym2_;
    }

    if (z > threshold)  return sym1_ + " overpriced (need confirm)";
    if (z < -threshold) return sym2_ + " overpriced (need confirm)";
    if (z > 0.5 * threshold)  return sym1_ + " slightly overpriced";
    if (z < -0.5 * threshold) return sym2_ + " slightly overpriced";
    return "Balanced";
}

void SpreadAnalyzer::reset() {
    spread_buffer_.clear();
    z_history_.clear();
    consecutive_exceed_ = 0;
    prev_z_sign_ = 0;
}

void SpreadAnalyzer::reset_signal_counters() {
    consecutive_exceed_ = 0;
    prev_z_sign_ = 0;
}
