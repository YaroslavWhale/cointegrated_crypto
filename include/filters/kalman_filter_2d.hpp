#pragma once
#include "filters/i_state_estimator.hpp"
#include "filters/kalman_filter_base.hpp"

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

    double get_alpha() const { return kf_.state().data[0][0]; }
    double get_beta()  const { return kf_.state().data[1][0]; }
    double innovation() const;
    double innovation_variance() const;

    double get_R() const;
    double get_Q_alpha() const;
    double get_Q_beta() const;

private:
    double R_;
    Matrix<double, 2, 2> Q_;
    double target_beta_;
    KalmanFilter<2> kf_;
};
