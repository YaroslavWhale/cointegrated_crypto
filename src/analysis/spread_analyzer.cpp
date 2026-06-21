#include "analysis/spread_analyzer.hpp"
#include <numeric>
#include <cmath>
#include <algorithm>

SpreadAnalyzer::SpreadAnalyzer(size_t spread_w, size_t z_w,
                               double entry_mult, double exit_mult,
                               double min_thr,
                               size_t vol_window, double vol_scale)
    : spread_window_(spread_w), z_window_(z_w),
    entry_multiplier_(entry_mult), exit_multiplier_(exit_mult),
    min_threshold_(min_thr),
    vol_window_(vol_window), vol_scale_factor_(vol_scale)
{}

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

double SpreadAnalyzer::compute_base_threshold(double multiplier) const {
    if (z_history_.size() < 2) return min_threshold_;
    double sum = std::accumulate(z_history_.begin(), z_history_.end(), 0.0);
    double mean = sum / z_history_.size();
    double sq = 0.0;
    for (double z : z_history_) sq += (z - mean) * (z - mean);
    double stdz = std::sqrt(sq / (z_history_.size() - 1));
    return std::max(min_threshold_, multiplier * stdz);
}

double SpreadAnalyzer::volatility_scale() const {
    if (!vol_ref_set_ || long_term_vol_ref_ < 1e-10) return 1.0;
    double ratio = ema_vol_ / long_term_vol_ref_;
    return 1.0 + vol_scale_factor_ * (std::sqrt(ratio) - 1.0);
}

double SpreadAnalyzer::entry_threshold() const {
    return compute_base_threshold(entry_multiplier_) * volatility_scale();
}

double SpreadAnalyzer::exit_threshold() const {
    return compute_base_threshold(exit_multiplier_) * volatility_scale();
}

void SpreadAnalyzer::update_volatility(double spread) {
    if (!spread_buffer_.empty()) {
        double prev = spread_buffer_.back();
        double diff = std::abs(spread - prev);
        spread_diff_buffer_.push_back(diff);
        if (spread_diff_buffer_.size() > vol_window_)
            spread_diff_buffer_.pop_front();

        constexpr double alpha = 0.1;
        if (ema_vol_ == 1.0 && spread_diff_buffer_.size() == 1) {
            ema_vol_ = diff;
        } else {
            ema_vol_ = alpha * diff + (1.0 - alpha) * ema_vol_;
        }

        if (!vol_ref_set_ && spread_diff_buffer_.size() >= vol_window_) {
            double sum = 0.0;
            for (double d : spread_diff_buffer_) sum += d;
            long_term_vol_ref_ = sum / spread_diff_buffer_.size();
            vol_ref_set_ = true;
        }
    }
}

SpreadAnalyzer::SignalAction SpreadAnalyzer::signal_from_z(double z, double entry_thr, double exit_thr) {
    if (spread_buffer_.size() < spread_window_)
        return SignalAction::NO_ACTION;

    if (z > entry_thr) {
        consecutive_exceed_ = (prev_sign_ > 0) ? consecutive_exceed_ + 1 : 1;
        prev_sign_ = 1;
    } else if (z < -entry_thr) {
        consecutive_exceed_ = (prev_sign_ < 0) ? consecutive_exceed_ + 1 : 1;
        prev_sign_ = -1;
    } else {
        consecutive_exceed_ = 0;
        prev_sign_ = 0;
    }

    if (consecutive_exceed_ >= required_consecutive_) {
        if (z > entry_thr) return SignalAction::ENTER_SHORT;
        if (z < -entry_thr) return SignalAction::ENTER_LONG;
    }

    if (last_signal_ == SignalAction::ENTER_SHORT && z < exit_thr)
        return SignalAction::EXIT;
    if (last_signal_ == SignalAction::ENTER_LONG && z > -exit_thr)
        return SignalAction::EXIT;

    return SignalAction::NO_ACTION;
}

void SpreadAnalyzer::add_spread(double spread) {
    update_volatility(spread);

    spread_buffer_.push_back(spread);
    if (spread_buffer_.size() > spread_window_)
        spread_buffer_.pop_front();

    double z = 0.0;
    if (spread_buffer_.size() >= 2)
        z = compute_z(spread);
    last_z_ = z;

    z_history_.push_back(z);
    if (z_history_.size() > z_window_)
        z_history_.pop_front();

    last_signal_ = signal_from_z(z, entry_threshold(), exit_threshold());
}

void SpreadAnalyzer::reset() {
    spread_buffer_.clear();
    z_history_.clear();
    spread_diff_buffer_.clear();
    consecutive_exceed_ = 0;
    prev_sign_ = 0;
    last_z_ = 0.0;
    ema_vol_ = 1.0;
    long_term_vol_ref_ = 1.0;
    vol_ref_set_ = false;
}

void SpreadAnalyzer::reset_signal_counters() {
    consecutive_exceed_ = 0;
    prev_sign_ = 0;
}
