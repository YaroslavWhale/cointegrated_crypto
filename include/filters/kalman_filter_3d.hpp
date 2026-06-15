#pragma once
#include "filters/i_state_estimator.hpp"
#include "filters/kalman_filter_base.hpp"

class KalmanFilter3D : public IStateEstimator {
public:
    KalmanFilter3D(double R,
                   double Q_alpha, double Q_beta, double Q_gamma,
                   double init_alpha = 0.0, double init_beta = 1.0, double init_gamma = 0.0,
                   double init_P_alpha = 1e-4, double init_P_beta = 1e-6, double init_P_gamma = 1e-8,
                   double target_beta = 1.0);

    void update(double price1_log, double price2_log) override;
    double get_spread() const override;
    void apply_pseudo_beta(double R_beta) override;
    void reset() override;

    double get_alpha() const { return kf_.state().data[0][0]; }
    double get_beta()  const { return kf_.state().data[1][0]; }
    double get_gamma() const { return kf_.state().data[2][0]; }
    double innovation() const { return kf_.innovation(); }
    double innovation_variance() const { return kf_.innovationVariance(); }

    double get_R() const { return R_; }
    double get_Q_alpha() const { return Q_.data[0][0]; }
    double get_Q_beta()  const { return Q_.data[1][1]; }
    double get_Q_gamma() const { return Q_.data[2][2]; }

private:
    double R_;
    Matrix<double, 3, 3> Q_;
    double target_beta_;
    KalmanFilter<3> kf_;
};
