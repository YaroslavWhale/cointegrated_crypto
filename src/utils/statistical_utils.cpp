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

    auto generate_range = [](double base, double min_factor, double max_factor, double step_log) {
        std::vector<double> values;
        if (base <= 0) base = 1e-10;
        double log_base = std::log10(base);
        double log_min = log_base + std::log10(min_factor);
        double log_max = log_base + std::log10(max_factor);
        for (double lv = log_min; lv <= log_max + 1e-12; lv += step_log) {
            values.push_back(std::pow(10.0, lv));
        }
        return values;
    };

    std::vector<double> R_cand  = generate_range(R_guess, 0.01, 100.0, 0.5);
    std::vector<double> Qa_cand = generate_range(Q_alpha_guess, 0.01, 100.0, 0.5);
    std::vector<double> Qb_cand = generate_range(Q_beta_guess, 0.01, 100.0, 0.5);

    if (R_cand.size() < 3)
        R_cand = generate_range(R_guess, 0.0001, 10000.0, 1.0);
    if (Qa_cand.size() < 3)
        Qa_cand = generate_range(Q_alpha_guess, 0.0001, 10000.0, 1.0);
    if (Qb_cand.size() < 3)
        Qb_cand = generate_range(Q_beta_guess, 0.0001, 10000.0, 1.0);

    double best_logL = -std::numeric_limits<double>::infinity();
    KalmanParams best_params = {R_guess, Q_alpha_guess, Q_beta_guess};

    for (double R : R_cand) {
        for (double Q_alpha : Qa_cand) {
            for (double Q_beta : Qb_cand) {
                KalmanFilter kf(R, Q_alpha, Q_beta, alpha_init, beta_init);
                double logL = 0.0;
                for (size_t i = 0; i < n; ++i) {
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
