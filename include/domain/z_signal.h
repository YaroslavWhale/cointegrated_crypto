#pragma once
#include <deque>
#include <string>

class SpreadAnalyzer {
public:
    SpreadAnalyzer(const std::string& sym1, const std::string& sym2,
                   size_t spread_window_size = 50,
                   size_t z_window_size = 50,
                   double threshold_multiplier = 2.0,
                   double min_threshold = 0.5);

    std::string add_spread(double spread);
    double get_z_score() const;
    double get_current_threshold() const;
    void reset();
    void reset_signal_counters();

private:
    std::string sym1_;
    std::string sym2_;

    size_t spread_window_size_;
    std::deque<double> spread_buffer_;

    size_t z_window_size_;
    std::deque<double> z_history_;
    double threshold_multiplier_;
    double min_threshold_;

    int consecutive_exceed_ = 0;
    int prev_z_sign_ = 0;
    static constexpr int REQUIRED_CONSECUTIVE = 1;

    double compute_z_for_spread(double spread) const;
    double compute_current_threshold() const;
    std::string signal_from_z(double z) const;
};
