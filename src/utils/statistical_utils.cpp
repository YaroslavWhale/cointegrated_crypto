#include "utils/statistical_utils.h"
#include "domain/kalman_filter.h"
#include <cmath>
#include <limits>
#include <iostream>
#include <algorithm>

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

static std::vector<double> generate_range_limited(double base, double min_val, double max_val, int steps) {
    std::vector<double> values;
    if (base <= 0.0) base = 1e-10;
    double log_base = std::log10(base);
    double log_min = std::log10(min_val);
    double log_max = std::log10(max_val);

    if (log_base < log_min) log_min = log_base - 0.5;
    if (log_base > log_max) log_max = log_base + 0.5;

    double step = (log_max - log_min) / (steps - 1);
    for (int i = 0; i < steps; ++i) {
        double val = std::pow(10.0, log_min + i * step);
        val = std::max(min_val, std::min(max_val, val));
        values.push_back(val);
    }
    return values;
}

KalmanParams optimize_kalman_parameters(const std::vector<double>& log_prices1,
                                        const std::vector<double>& log_prices2,
                                        double alpha_init, double beta_init) {
    size_t n = std::min(log_prices1.size(), log_prices2.size());
    if (n < 2) return {0.001, 0.0001, 0.0001};

    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_xx = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double x = log_prices2[i];
        double y = log_prices1[i];
        sum_x += x; sum_y += y;
        sum_xy += x * y; sum_xx += x * x;
    }
    double mean_x = sum_x / n;
    double mean_y = sum_y / n;
    double beta_ols = (sum_xy - n * mean_x * mean_y) / (sum_xx - n * mean_x * mean_x);
    double alpha_ols = mean_y - beta_ols * mean_x;

    double var_residuals = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double res = log_prices1[i] - (alpha_ols + beta_ols * log_prices2[i]);
        var_residuals += res * res;
    }
    var_residuals /= (n > 2) ? (n - 2) : n;
    double R_guess = std::max(var_residuals, 1e-10);

    int window = std::min(30, (int)n / 2);
    std::vector<double> alphas, betas;
    for (size_t i = 0; i + window <= n; i += window / 2) {
        double sx = 0, sy = 0, sxy = 0, sxx = 0;
        for (size_t j = i; j < i + window && j < n; ++j) {
            double x = log_prices2[j];
            double y = log_prices1[j];
            sx += x; sy += y; sxy += x * y; sxx += x * x;
        }
        double mx = sx / window;
        double my = sy / window;
        double b = (sxy - window * mx * my) / (sxx - window * mx * mx);
        double a = my - b * mx;
        alphas.push_back(a);
        betas.push_back(b);
    }

    double var_alpha = 0.0, var_beta = 0.0;
    if (alphas.size() > 1) {
        double ma = 0.0, mb = 0.0;
        for (size_t i = 0; i < alphas.size(); ++i) { ma += alphas[i]; mb += betas[i]; }
        ma /= alphas.size();
        mb /= betas.size();
        for (size_t i = 0; i < alphas.size(); ++i) {
            double da = alphas[i] - ma;
            double db = betas[i] - mb;
            var_alpha += da * da;
            var_beta += db * db;
        }
        var_alpha /= (alphas.size() - 1);
        var_beta /= (betas.size() - 1);
    }
    double Q_alpha_guess = std::max(var_alpha, 1e-12);
    double Q_beta_guess  = std::max(var_beta, 1e-12);

    // Границы, соответствующие KalmanFilter
    constexpr double R_min_opt = 1e-4;
    constexpr double R_max_opt = 1.0;
    constexpr double Q_min_opt = 1e-12;
    constexpr double Q_max_opt = 1e-10;

    auto evaluate = [&](double R, double Qa, double Qb) {
        KalmanFilter kf(R, Qa, Qb, alpha_init, beta_init);
        double logL = 0.0;
        for (size_t i = 0; i < n; ++i) {
            kf.update(log_prices1[i], log_prices2[i]);
            double innov = kf.get_innovation();
            double S = kf.get_predicted_variance();
            if (S <= 1e-12) S = 1e-12;
            logL += -0.5 * (std::log(S) + (innov * innov) / S);
        }
        return logL;
    };

    auto R_range  = generate_range_limited(R_guess, R_min_opt, R_max_opt, 8);
    auto Qa_range = generate_range_limited(Q_alpha_guess, Q_min_opt, Q_max_opt, 8);
    auto Qb_range = generate_range_limited(Q_beta_guess, Q_min_opt, Q_max_opt, 8);

    double best_logL = -std::numeric_limits<double>::infinity();
    KalmanParams best_params = {R_guess, Q_alpha_guess, Q_beta_guess};

    for (double R : R_range) {
        for (double Qa : Qa_range) {
            for (double Qb : Qb_range) {
                double logL = evaluate(R, Qa, Qb);
                if (logL > best_logL) {
                    best_logL = logL;
                    best_params = {R, Qa, Qb};
                }
            }
        }
    }

    auto fine_R  = generate_range_limited(best_params.R, R_min_opt, R_max_opt, 5);
    auto fine_Qa = generate_range_limited(best_params.Q_alpha, Q_min_opt, Q_max_opt, 5);
    auto fine_Qb = generate_range_limited(best_params.Q_beta, Q_min_opt, Q_max_opt, 5);

    for (double R : fine_R) {
        for (double Qa : fine_Qa) {
            for (double Qb : fine_Qb) {
                double logL = evaluate(R, Qa, Qb);
                if (logL > best_logL) {
                    best_logL = logL;
                    best_params = {R, Qa, Qb};
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
