#include "utils/statistical_utils.h"
#include "domain/kalman_filter.h"
#include <cmath>
#include <limits>
#include <iostream>

OLSResult compute_ols(const std::vector<double>& x, const std::vector<double>& y) {
    size_t n = std::min(x.size(), y.size());
    if (n < 2) return {0.0, 1.0};

    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_xx = 0.0;
    for (size_t i = 0; i < n; ++i) {
        sum_x += x[i];
        sum_y += y[i];
        sum_xy += x[i] * y[i];
        sum_xx += x[i] * x[i];
    }
    double mean_x = sum_x / n;
    double mean_y = sum_y / n;
    double beta = (sum_xy - n * mean_x * mean_y) / (sum_xx - n * mean_x * mean_x);
    double alpha = mean_y - beta * mean_x;
    return {alpha, beta};
}

KalmanParams optimize_kalman_parameters(const std::vector<double>& log_prices1,
                                        const std::vector<double>& log_prices2,
                                        double alpha_init, double beta_init) {
    double best_logL = -std::numeric_limits<double>::infinity();
    KalmanParams best_params = {0.001, 0.0001, 0.0001};

    for (double r_exp = -5.0; r_exp <= -2.0; r_exp += 0.5) {
        for (double qa_exp = -7.0; qa_exp <= -3.0; qa_exp += 0.5) {
            for (double qb_exp = -7.0; qb_exp <= -3.0; qb_exp += 0.5) {
                double R = std::pow(10.0, r_exp);
                double Q_alpha = std::pow(10.0, qa_exp);
                double Q_beta = std::pow(10.0, qb_exp);

                KalmanFilter kf(R, Q_alpha, Q_beta, alpha_init, beta_init);
                double logL = 0.0;
                for (size_t i = 0; i < log_prices1.size(); ++i) {
                    kf.update(log_prices1[i], log_prices2[i]);
                    double innov = kf.get_innovation();
                    double S = kf.get_predicted_variance();
                    if (S <= 1e-12) S = 1e-12;
                    logL += -0.5 * (std::log(S) + (innov * innov) / S);
                }
                if (logL > best_logL) {
                    best_logL = logL;
                    best_params = {R, Q_alpha, Q_beta};
                }
            }
        }
    }

    std::cout << "[OPT] Best parameters: R=" << best_params.R
              << ", Q_alpha=" << best_params.Q_alpha
              << ", Q_beta=" << best_params.Q_beta
              << " (logL=" << best_logL << ")" << std::endl;
    return best_params;
}
