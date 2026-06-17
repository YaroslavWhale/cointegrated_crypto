#include "analysis/spread_analyzer.hpp"
#include <numeric>
#include <cmath>

SpreadAnalyzer::SpreadAnalyzer(size_t spread_w, size_t z_w,
                               double mult, double min_thr)
    : spread_window_(spread_w), z_window_(z_w),
    threshold_multiplier_(mult), min_threshold_(min_thr) {}

double SpreadAnalyzer::compute_z(double spread) const {
    if (spread_buffer_.size() < 2) return 0.0;
    double sum = std::accumulate(spread_buffer_.begin(), spread_buffer_.end(), 0.0);
    double mean = sum / spread_buffer_.size();
    double sq = 0.0;
    for (double v : spread_buffer_) sq += (v - mean) * (v - mean);
    double stddev = std::sqrt(sq / (spread_buffer_.size() - 1));
    if (stddev < 1e-12) return 0.0;
    return (spread - mean) / stddev;
}

double SpreadAnalyzer::compute_threshold() const {
    if (z_history_.size() < 2) return min_threshold_;
    double sum = std::accumulate(z_history_.begin(), z_history_.end(), 0.0);
    double mean = sum / z_history_.size();
    double sq = 0.0;
    for (double z : z_history_) sq += (z - mean) * (z - mean);
    double stdz = std::sqrt(sq / (z_history_.size() - 1));
    return std::max(min_threshold_, threshold_multiplier_ * stdz);
}

std::string SpreadAnalyzer::add_spread(double spread) {
    spread_buffer_.push_back(spread);
    if (spread_buffer_.size() > spread_window_) spread_buffer_.pop_front();

    double z = 0.0;
    if (spread_buffer_.size() >= 2) {
        z = compute_z(spread);
    }
    last_z_ = z;

    double threshold = compute_threshold();

    z_history_.push_back(z);
    if (z_history_.size() > z_window_) z_history_.pop_front();

    if (z > threshold) {
        consecutive_exceed_ = (prev_sign_ > 0) ? consecutive_exceed_ + 1 : 1;
        prev_sign_ = 1;
    } else if (z < -threshold) {
        consecutive_exceed_ = (prev_sign_ < 0) ? consecutive_exceed_ + 1 : 1;
        prev_sign_ = -1;
    } else {
        consecutive_exceed_ = 0;
        prev_sign_ = 0;
    }

    return signal_from_z(z, threshold);
}

double SpreadAnalyzer::z_score() const {
    return last_z_;
}

double SpreadAnalyzer::current_threshold() const {
    return compute_threshold();
}

std::string SpreadAnalyzer::signal_from_z(double z, double threshold) const {
    if (spread_buffer_.size() < spread_window_)
        return "CALIBRATING";
    if (consecutive_exceed_ >= required_consecutive_) {
        if (z > threshold)  return "SELL_SYM1";
        if (z < -threshold) return "BUY_SYM1";
    }
    return "NEUTRAL";
}

void SpreadAnalyzer::reset() {
    spread_buffer_.clear();
    z_history_.clear();
    consecutive_exceed_ = 0;
    prev_sign_ = 0;
    last_z_ = 0.0;
}

void SpreadAnalyzer::reset_signal_counters() {
    consecutive_exceed_ = 0;
    prev_sign_ = 0;
}
