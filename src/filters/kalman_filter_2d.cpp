#include "filters/kalman_filter_2d.h"
#include <algorithm>
#include <cmath>

KalmanFilter2D::KalmanFilter2D(double R, double Q_alpha, double Q_beta,
                               double init_alpha, double init_beta,
                               double init_P_alpha, double init_P_beta,
                               double target_beta)
    : R_(std::max(1e-4, std::min(R, 1.0)))
    , Q_{{ {std::max(1e-12, std::min(Q_alpha, 1e-10)), 0.0},
          {0.0, std::max(1e-12, std::min(Q_beta, 1e-10))} }}
    , x_{{init_alpha, init_beta}}
    , P_{{ {std::max(init_P_alpha, 1e-10), 0.0},
          {0.0, std::max(init_P_beta, 1e-10)} }}
    , target_beta_(target_beta)
{}

void KalmanFilter2D::update(double price1_log, double price2_log) {
    // prediction
    auto x_pred = x_;
    std::array<std::array<double, 2>, 2> P_pred;
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            P_pred[i][j] = P_[i][j] + Q_[i][j];

    double H0 = 1.0;
    double H1 = price2_log;
    double y = price1_log - (x_pred[0] + H1 * x_pred[1]);

    double HP0 = H0 * P_pred[0][0] + H1 * P_pred[1][0];
    double HP1 = H0 * P_pred[0][1] + H1 * P_pred[1][1];
    double S = HP0 * H0 + HP1 * H1 + R_;
    if (S < 1e-12) S = 1e-12;
    double invS = 1.0 / S;

    double K0 = (P_pred[0][0] * H0 + P_pred[0][1] * H1) * invS;
    double K1 = (P_pred[1][0] * H0 + P_pred[1][1] * H1) * invS;

    x_[0] = x_pred[0] + K0 * y;
    x_[1] = x_pred[1] + K1 * y;

    P_[0][0] = P_pred[0][0] - K0 * S * K0;
    P_[0][1] = P_pred[0][1] - K0 * S * K1;
    P_[1][0] = P_pred[1][0] - K1 * S * K0;
    P_[1][1] = P_pred[1][1] - K1 * S * K1;

    double avg = 0.5 * (P_[0][1] + P_[1][0]);
    P_[0][1] = P_[1][0] = avg;
    P_[0][0] = std::max(P_[0][0], 1e-10);
    P_[1][1] = std::max(P_[1][1], 1e-10);

    innovation_ = y;
    S_ = S;
}

void KalmanFilter2D::apply_pseudo_beta(double R_beta) {
    double z = target_beta_;
    double y_beta = z - x_[1];

    double S = P_[1][1] + R_beta;
    if (S < 1e-12) S = 1e-12;
    double invS = 1.0 / S;

    double K_alpha = P_[0][1] * invS;
    double K_beta  = P_[1][1] * invS;

    x_[0] += K_alpha * y_beta;
    x_[1] += K_beta  * y_beta;

    P_[0][0] -= K_alpha * S * K_alpha;
    P_[0][1] -= K_alpha * S * K_beta;
    P_[1][0] -= K_beta  * S * K_alpha;
    P_[1][1] -= K_beta  * S * K_beta;

    double avg = 0.5 * (P_[0][1] + P_[1][0]);
    P_[0][1] = P_[1][0] = avg;
    P_[0][0] = std::max(P_[0][0], 1e-10);
    P_[1][1] = std::max(P_[1][1], 1e-10);
}

double KalmanFilter2D::get_spread() const { return innovation_; }

void KalmanFilter2D::reset() {
    x_ = {0.0, 1.0};
    P_ = {{ {1e-4, 0.0}, {0.0, 1e-6} }};
}
