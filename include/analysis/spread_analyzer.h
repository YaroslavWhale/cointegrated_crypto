#pragma once
#include <deque>
#include <string>

class SpreadAnalyzer {
public:
    SpreadAnalyzer(size_t spread_window = 50, size_t z_window = 50,
                   double threshold_multiplier = 2.0, double min_threshold = 0.5);

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
    std::deque<double> spread_buffer_;
    size_t z_window_;
    std::deque<double> z_history_;
    double threshold_multiplier_;
    double min_threshold_;

    int consecutive_exceed_ = 0;
    int prev_sign_ = 0;
    static constexpr int required_consecutive_ = 1;
};
