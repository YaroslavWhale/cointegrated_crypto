#pragma once
#include "math/matrix.hpp"
#include <algorithm>

template<size_t Dim>
class KalmanFilter {
public:
    using StateVec = Vector<double, Dim>;
    using CovMat   = Matrix<double, Dim, Dim>;

    KalmanFilter(const CovMat& Q, double R,
                 const StateVec& x0, const CovMat& P0)
        : Q_(Q), R_(R), x_(x0), P_(P0) {}

    void predict() {
        P_ = P_ + Q_;
    }

    void update(const Vector<double, Dim>& H, double z) {
        predict();
        double y = z - (H.transpose() * x_).data[0][0];

        auto HP = H.transpose() * P_;
        double S = (HP * H).data[0][0] + R_;
        if (S < 1e-12) S = 1e-12;
        double invS = 1.0 / S;

        auto K = P_ * H * invS;
        x_ = x_ + K * y;

        auto KH = K * H.transpose();
        for (size_t i = 0; i < Dim; ++i)
            KH.data[i][i] = 1.0 - KH.data[i][i];
        P_ = KH * P_;

        for (size_t i = 0; i < Dim; ++i)
            for (size_t j = i+1; j < Dim; ++j) {
                double avg = 0.5 * (P_.data[i][j] + P_.data[j][i]);
                P_.data[i][j] = P_.data[j][i] = avg;
            }
        for (size_t i = 0; i < Dim; ++i)
            P_.data[i][i] = std::max(P_.data[i][i], 1e-10);

        innovation_ = y;
        S_ = S;
    }

    void pseudoUpdate(size_t stateIndex, double target, double R_pseudo) {
        Vector<double, Dim> H_pseudo;
        H_pseudo.zero();
        H_pseudo.data[stateIndex][0] = 1.0;

        double z = target;
        double y = z - (H_pseudo.transpose() * x_).data[0][0];

        auto HP = H_pseudo.transpose() * P_;
        double S = (HP * H_pseudo).data[0][0] + R_pseudo;
        if (S < 1e-12) S = 1e-12;
        double invS = 1.0 / S;

        auto K = P_ * H_pseudo * invS;
        x_ = x_ + K * y;
        auto KH = K * H_pseudo.transpose();
        for (size_t i = 0; i < Dim; ++i)
            KH.data[i][i] = 1.0 - KH.data[i][i];
        P_ = KH * P_;

        for (size_t i = 0; i < Dim; ++i)
            for (size_t j = i+1; j < Dim; ++j) {
                double avg = 0.5 * (P_.data[i][j] + P_.data[j][i]);
                P_.data[i][j] = P_.data[j][i] = avg;
            }
        for (size_t i = 0; i < Dim; ++i)
            P_.data[i][i] = std::max(P_.data[i][i], 1e-10);
    }

    const StateVec& state() const { return x_; }
    double innovation() const { return innovation_; }
    double innovationVariance() const { return S_; }

    void reset(const StateVec& x0, const CovMat& P0) {
        x_ = x0;
        P_ = P0;
    }

private:
    CovMat Q_;
    double R_;
    StateVec x_;
    CovMat P_;
    double innovation_ = 0.0;
    double S_ = 0.0;
};
