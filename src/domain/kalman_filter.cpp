#include "domain/kalman_filter.h"
#include <algorithm>
#include <cmath>

KalmanFilter::KalmanFilter(double R, double Q_alpha, double Q_beta,
                           double init_alpha, double init_beta,
                           double init_P_alpha, double init_P_beta)
    : R_(R), R_adaptive_(R), R_base_(R)
    , Q_{{ {Q_alpha, 0.0}, {0.0, Q_beta} }}
    , Q_base_(Q_)
    , x_{{init_alpha, init_beta}}
    , P_{{ {init_P_alpha, 0.0}, {0.0, init_P_beta} }}
    , last_innovation_(0.0), last_S_(0.0)
{
}

void KalmanFilter::reset_adaptive() {
    R_adaptive_ = R_base_;
    R_ = R_base_;
    Q_ = Q_base_;
}

void KalmanFilter::update(double price1, double price2) {
    std::array<double, 2> x_pred = x_;
    std::array<std::array<double, 2>, 2> P_pred;
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            P_pred[i][j] = P_[i][j] + Q_[i][j];

    const double H0 = 1.0;
    const double H1 = price2;
    double y = price1 - (x_pred[0] + H1 * x_pred[1]);

    double HP0 = H0 * P_pred[0][0] + H1 * P_pred[1][0];
    double HP1 = H0 * P_pred[0][1] + H1 * P_pred[1][1];
    double S = HP0 * H0 + HP1 * H1 + R_;
    if (S < 1e-12) S = 1e-12;
    double invS = 1.0 / S;

    double K0 = (P_pred[0][0] * H0 + P_pred[0][1] * H1) * invS;
    double K1 = (P_pred[1][0] * H0 + P_pred[1][1] * H1) * invS;

    x_[0] = x_pred[0] + K0 * y;
    x_[1] = x_pred[1] + K1 * y;

    double KSK0 = K0 * S * K0;
    double KSK1 = K1 * S * K1;
    double KSK01 = K0 * S * K1;
    P_[0][0] = P_pred[0][0] - KSK0;
    P_[0][1] = P_pred[0][1] - KSK01;
    P_[1][0] = P_pred[1][0] - KSK01;
    P_[1][1] = P_pred[1][1] - KSK1;

    double P01_avg = 0.5 * (P_[0][1] + P_[1][0]);
    P_[0][1] = P_[1][0] = P01_avg;
    P_[0][0] = std::max(P_[0][0], 1e-10);
    P_[1][1] = std::max(P_[1][1], 1e-10);

    last_innovation_ = y;
    last_S_ = S;

    //Адаптация
    double innov_sq = y * y;
    R_adaptive_ = lambda_R_ * R_adaptive_ + (1.0 - lambda_R_) * innov_sq;
    R_ = std::max(R_min_, std::min(R_max_, R_adaptive_));

    if (last_S_ > 1e-12) {
        double norm_innov = innov_sq / last_S_;
        if (norm_innov > threshold_innov_) {
            Q_[0][0] = std::min(Q_max_, Q_[0][0] * Q_increase_factor_);
            Q_[1][1] = std::min(Q_max_, Q_[1][1] * Q_increase_factor_);
        } else {
            Q_[0][0] = std::max(Q_base_[0][0], Q_[0][0] * Q_decrease_factor_);
            Q_[1][1] = std::max(Q_base_[1][1], Q_[1][1] * Q_decrease_factor_);
        }
    }
}

double KalmanFilter::get_spread() const { return last_innovation_; }
double KalmanFilter::get_alpha() const { return x_[0]; }
double KalmanFilter::get_beta()  const { return x_[1]; }
double KalmanFilter::get_alpha_uncertainty() const { return P_[0][0]; }
double KalmanFilter::get_beta_uncertainty()  const { return P_[1][1]; }

void KalmanFilter::reset(double alpha, double beta, double P_alpha, double P_beta) {
    x_[0] = alpha; x_[1] = beta;
    P_[0][0] = std::max(P_alpha, 1e-10);
    P_[1][1] = std::max(P_beta, 1e-10);
    P_[0][1] = P_[1][0] = 0.0;
    // При сбросе сохраняем текущие адаптивные параметры как новые базовые
    R_base_ = R_adaptive_;
    Q_base_ = Q_;
    R_ = R_base_;
    R_adaptive_ = R_base_;
}
