#pragma once
#include <deque>
#include <string>
#include <vector>

class SpreadAnalyzer {
public:
    SpreadAnalyzer(size_t spread_window, size_t z_window,
                   double threshold_multiplier, double min_threshold);

    std::string add_spread(double spread);

    double z_score() const;

    double current_threshold() const;

    void reset();
    void reset_signal_counters();

private:
    double compute_z(double spread) const;
    double compute_threshold() const;

    std::string signal_from_z(double z, double threshold) const;

    size_t spread_window_;
    size_t z_window_;
    double threshold_multiplier_;
    double min_threshold_;

    std::deque<double> spread_buffer_;
    std::deque<double> z_history_;

    int consecutive_exceed_ = 0;
    int prev_sign_ = 0;
    int required_consecutive_ = 2;

    double last_z_ = 0.0;
};
