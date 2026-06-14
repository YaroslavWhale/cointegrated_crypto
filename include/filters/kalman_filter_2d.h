#pragma once
#include "filters/i_state_estimator.h"
#include <array>

struct KalmanParams {
    double R;
    double Q_alpha;
    double Q_beta;
};

class KalmanFilter2D : public IStateEstimator {
public:
    KalmanFilter2D(double R, double Q_alpha, double Q_beta,
                   double init_alpha = 0.0, double init_beta = 1.0,
                   double init_P_alpha = 1e-4, double init_P_beta = 1e-6,
                   double target_beta = 1.0);

    void update(double price1_log, double price2_log) override;
    double get_spread() const override;
    void apply_pseudo_beta(double R_beta) override;
    void reset() override;

    double get_alpha() const { return x_[0]; }
    double get_beta()  const { return x_[1]; }
    double innovation() const { return innovation_; }
    double innovation_variance() const { return S_; }

    double get_R() const { return R_; }
    double get_Q_alpha() const { return Q_[0][0]; }
    double get_Q_beta()  const { return Q_[1][1]; }

private:
    double R_;
    std::array<std::array<double, 2>, 2> Q_;
    std::array<double, 2> x_;
    std::array<std::array<double, 2>, 2> P_;
    double target_beta_;
    double innovation_ = 0.0;
    double S_ = 0.0;
};
