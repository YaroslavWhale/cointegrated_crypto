#pragma once
#include <deque>
#include <string>

class SpreadAnalyzer {
public:
    SpreadAnalyzer(const std::string& sym1, const std::string& sym2,
                   size_t window_size = 50);

    std::string add_spread(double spread);

    double get_z_score() const;

    void reset();

private:
    std::string sym1_;
    std::string sym2_;
    size_t window_size_;
    std::deque<double> spread_buffer_;

    double last_z_score_ = 0.0;
    int consecutive_signals_ = 0;
    static constexpr int REQUIRED_CONSECUTIVE = 3;
    static constexpr double HYSTERESIS = 0.3;

    double compute_z_score(double current_spread) const;
    std::string signal_from_z_filtered(double z);
};
