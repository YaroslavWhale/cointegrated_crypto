#pragma once
#include <vector>

struct OLSResult {
    double alpha;
    double beta;
};

struct KalmanParams {
    double R;
    double Q_alpha;
    double Q_beta;
};

struct KalmanParams3D {
    double R;
    double Q_alpha;
    double Q_beta;
    double Q_gamma;
};

OLSResult compute_ols(const std::vector<double>& x, const std::vector<double>& y);

KalmanParams optimize_kalman_2d(const std::vector<double>& log_p1,
                                const std::vector<double>& log_p2,
                                double init_alpha, double init_beta);

KalmanParams3D optimize_kalman_3d(const std::vector<double>& log_p1,
                                  const std::vector<double>& log_p2,
                                  double init_alpha, double init_beta,
                                  double init_gamma = 0.0);
