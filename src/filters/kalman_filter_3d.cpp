#include "filters/kalman_filter_3d.hpp"
#include <algorithm>
#include <cmath>

static Matrix<double, 3, 3> make_Q(double Q_alpha, double Q_beta, double Q_gamma) {
    Matrix<double, 3, 3> Q;
    Q.zero();
    Q.data[0][0] = std::max(1e-12, std::min(Q_alpha, 1e-10));
    Q.data[1][1] = std::max(1e-12, std::min(Q_beta,  1e-10));
    Q.data[2][2] = std::max(1e-12, std::min(Q_gamma, 1e-10));
    return Q;
}

static KalmanFilter<3>::StateVec make_x0(double init_alpha, double init_beta, double init_gamma) {
    KalmanFilter<3>::StateVec x0;
    x0.data[0][0] = init_alpha;
    x0.data[1][0] = init_beta;
    x0.data[2][0] = init_gamma;
    return x0;
}

static KalmanFilter<3>::CovMat make_P0(double init_P_alpha, double init_P_beta, double init_P_gamma) {
    KalmanFilter<3>::CovMat P0;
    P0.zero();
    P0.data[0][0] = std::max(init_P_alpha, 1e-10);
    P0.data[1][1] = std::max(init_P_beta,  1e-10);
    P0.data[2][2] = std::max(init_P_gamma, 1e-10);
    return P0;
}

KalmanFilter3D::KalmanFilter3D(double R,
                               double Q_alpha, double Q_beta, double Q_gamma,
                               double init_alpha, double init_beta, double init_gamma,
                               double init_P_alpha, double init_P_beta, double init_P_gamma)
    : R_(std::max(1e-4, std::min(R, 1.0)))
    , Q_(make_Q(Q_alpha, Q_beta, Q_gamma))
    , kf_(Q_, R_, make_x0(init_alpha, init_beta, init_gamma),
          make_P0(init_P_alpha, init_P_beta, init_P_gamma))
{}

void KalmanFilter3D::update(double price1_log, double price2_log) {
    Vector<double, 3> H;
    H.data[0][0] = 1.0;
    H.data[1][0] = price2_log;
    H.data[2][0] = price2_log * price2_log;
    kf_.update(H, price1_log);
}

double KalmanFilter3D::get_spread() const {
    return kf_.innovation();
}

void KalmanFilter3D::reset() {
    KalmanFilter<3>::StateVec x0;
    x0.data[0][0] = 0.0;
    x0.data[1][0] = 1.0;
    x0.data[2][0] = 0.0;
    KalmanFilter<3>::CovMat P0;
    P0.zero();
    P0.data[0][0] = 1e-4;
    P0.data[1][1] = 1e-6;
    P0.data[2][2] = 1e-8;
    kf_.reset(x0, P0);
}
