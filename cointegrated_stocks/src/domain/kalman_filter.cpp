#include "domain/kalman_filter.h"

KalmanFilter::KalmanFilter(double R, double Q_alpha, double Q_beta,
                           double init_alpha, double init_beta,
                           double init_P_alpha, double init_P_beta)
    : R_(R)
    , Q_{{ {Q_alpha, 0.0}, {0.0, Q_beta} }}
    , x_{{init_alpha, init_beta}}
    , P_{{ {init_P_alpha, 0.0}, {0.0, init_P_beta} }}
    , last_innovation_(0.0) {}

void KalmanFilter::update(double price1, double price2) {
    // Прогноз
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

    last_innovation_ = y;
}

double KalmanFilter::get_spread() const { return last_innovation_; }
double KalmanFilter::get_alpha() const { return x_[0]; }
double KalmanFilter::get_beta()  const { return x_[1]; }
double KalmanFilter::get_alpha_uncertainty() const { return P_[0][0]; }
double KalmanFilter::get_beta_uncertainty()  const { return P_[1][1]; }

void KalmanFilter::reset(double alpha, double beta, double P_alpha, double P_beta) {
    x_[0] = alpha; x_[1] = beta;
    P_[0][0] = P_alpha; P_[1][1] = P_beta;
    P_[0][1] = P_[1][0] = 0.0;
}
