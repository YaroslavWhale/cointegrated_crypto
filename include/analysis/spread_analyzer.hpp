#pragma once
#include <deque>
#include <cstddef>

class SpreadAnalyzer {
public:
    enum class SignalAction { NO_ACTION, ENTER_LONG, ENTER_SHORT, EXIT };

    SpreadAnalyzer(size_t spread_window,
                   size_t z_window,
                   double entry_multiplier,
                   double exit_multiplier,
                   double min_threshold,
                   size_t vol_window = 50,
                   double vol_scale_factor = 1.0);

    void add_spread(double spread);

    SignalAction current_signal() const { return last_signal_; }
    double entry_threshold() const;
    double exit_threshold() const;
    double z_score() const { return last_z_; }

    void reset();
    void reset_signal_counters();

private:
    size_t spread_window_;
    size_t z_window_;
    double entry_multiplier_;
    double exit_multiplier_;
    double min_threshold_;
    size_t vol_window_;
    double vol_scale_factor_;

    std::deque<double> spread_buffer_;
    std::deque<double> z_history_;
    std::deque<double> spread_diff_buffer_;

    double ema_vol_ = 1.0;
    double long_term_vol_ref_ = 1.0;
    bool vol_ref_set_ = false;

    double last_z_ = 0.0;
    int consecutive_exceed_ = 0;
    int prev_sign_ = 0;
    static constexpr int required_consecutive_ = 2;
    SignalAction last_signal_ = SignalAction::NO_ACTION;

    double compute_z(double spread) const;
    double compute_base_threshold(double multiplier) const;
    double volatility_scale() const;
    void update_volatility(double spread);
    SignalAction signal_from_z(double z, double entry_thr, double exit_thr);
};
