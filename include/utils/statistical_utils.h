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

OLSResult compute_ols(const std::vector<double>& x, const std::vector<double>& y);

KalmanParams optimize_kalman_parameters(const std::vector<double>& log_prices1,
                                        const std::vector<double>& log_prices2,
                                        double alpha_init, double beta_init);
