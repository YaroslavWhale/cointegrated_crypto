#include "filters/kalman_filter_2d.hpp"
#include <algorithm>
#include <cmath>

static Matrix<double, 2, 2> make_Q(double Q_alpha, double Q_beta) {
    Matrix<double, 2, 2> Q;
    Q.zero();
    Q.data[0][0] = std::max(1e-12, std::min(Q_alpha, 1e-10));
    Q.data[1][1] = std::max(1e-12, std::min(Q_beta,  1e-10));
    return Q;
}

static KalmanFilter<2>::StateVec make_x0(double init_alpha, double init_beta) {
    KalmanFilter<2>::StateVec x0;
    x0.data[0][0] = init_alpha;
    x0.data[1][0] = init_beta;
    return x0;
}

static KalmanFilter<2>::CovMat make_P0(double init_P_alpha, double init_P_beta) {
    KalmanFilter<2>::CovMat P0;
    P0.zero();
    P0.data[0][0] = std::max(init_P_alpha, 1e-10);
    P0.data[1][1] = std::max(init_P_beta,  1e-10);
    return P0;
}

KalmanFilter2D::KalmanFilter2D(double R, double Q_alpha, double Q_beta,
                               double init_alpha, double init_beta,
                               double init_P_alpha, double init_P_beta,
                               double target_beta)
    : R_(std::max(1e-4, std::min(R, 1.0)))
    , Q_(make_Q(Q_alpha, Q_beta))
    , target_beta_(target_beta)
    , kf_(Q_, R_, make_x0(init_alpha, init_beta), make_P0(init_P_alpha, init_P_beta))
{}

void KalmanFilter2D::update(double price1_log, double price2_log) {
    Vector<double, 2> H;
    H.data[0][0] = 1.0;
    H.data[1][0] = price2_log;
    kf_.update(H, price1_log);
}

double KalmanFilter2D::get_spread() const {
    return kf_.innovation();
}

void KalmanFilter2D::apply_pseudo_beta(double R_beta) {
    kf_.pseudoUpdate(1, target_beta_, R_beta);
}

void KalmanFilter2D::reset() {
    KalmanFilter<2>::StateVec x0;
    x0.data[0][0] = 0.0;
    x0.data[1][0] = 1.0;
    KalmanFilter<2>::CovMat P0;
    P0.zero();
    P0.data[0][0] = 1e-4;
    P0.data[1][1] = 1e-6;
    kf_.reset(x0, P0);
}

double KalmanFilter2D::get_R() const { return R_; }
double KalmanFilter2D::get_Q_alpha() const { return Q_.data[0][0]; }
double KalmanFilter2D::get_Q_beta() const { return Q_.data[1][1]; }
double KalmanFilter2D::innovation() const { return kf_.innovation(); }
double KalmanFilter2D::innovation_variance() const { return kf_.innovationVariance(); }
