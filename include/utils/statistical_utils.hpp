#pragma once
#include <vector>

struct KalmanParams {
    double R;
    double Q_alpha;
    double Q_beta;
};

struct OLSResult {
    double alpha;
    double beta;
};

OLSResult compute_ols(const std::vector<double>& x, const std::vector<double>& y);
KalmanParams optimize_kalman_2d(const std::vector<double>& log_p1,
                                const std::vector<double>& log_p2,
                                double init_alpha, double init_beta);
