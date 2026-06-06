#include "domain/cointegration_test.h"
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <iostream>
#include <algorithm>

namespace {

std::pair<double, double> ols(const std::vector<double>& y,
                              const std::vector<double>& x) {
    size_t n = y.size();
    if (n != x.size() || n < 2) throw std::runtime_error("Invalid input size");

    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    for (size_t i = 0; i < n; ++i) {
        sum_x += x[i];
        sum_y += y[i];
        sum_xy += x[i] * y[i];
        sum_x2 += x[i] * x[i];
    }

    double denom = n * sum_x2 - sum_x * sum_x;
    if (std::abs(denom) < 1e-12) return {0.0, 1.0};

    double b = (n * sum_xy - sum_x * sum_y) / denom;
    double a = (sum_y - b * sum_x) / n;
    return {a, b};
}

double adf_pvalue(const std::vector<double>& residuals) {
    size_t n = residuals.size();
    if (n < 10) return 1.0;

    double sum_lag = 0.0, sum_diff = 0.0;
    double sum_lag2 = 0.0, sum_lag_diff = 0.0;

    for (size_t t = 1; t < n; ++t) {
        double lag = residuals[t-1];
        double diff = residuals[t] - lag;  // Δy_t
        sum_lag += lag;
        sum_diff += diff;
        sum_lag2 += lag * lag;
        sum_lag_diff += lag * diff;
    }

    size_t m = n - 1;
    double denom = m * sum_lag2 - sum_lag * sum_lag;
    if (std::abs(denom) < 1e-12) return 1.0;

    double gamma = (m * sum_lag_diff - sum_lag * sum_diff) / denom;

    // Остаточная дисперсия с учётом intercept (mean_diff) и slope (gamma)
    double mean_diff = sum_diff / m;
    double mean_lag = sum_lag / m;
    double sum_sq_err = 0.0;
    for (size_t t = 1; t < n; ++t) {
        double lag = residuals[t-1];
        double diff = residuals[t] - lag;
        double predicted = gamma * (lag - mean_lag) + mean_diff;
        double error = diff - predicted;
        sum_sq_err += error * error;
    }

    double sigma2 = sum_sq_err / (m - 2);              // две степени свободы (intercept, slope)
    double se_gamma = std::sqrt(sigma2 / (sum_lag2 - sum_lag * sum_lag / m));
    double t_stat = gamma / se_gamma;

    // Критические значения ADF (константа, без тренда) – MacKinnon (1996)
    static const double crit_01 = -3.43;
    static const double crit_05 = -2.86;
    static const double crit_10 = -2.57;

    if (t_stat < crit_01) return 0.01;
    if (t_stat < crit_05) return 0.01 + 0.04 * (t_stat - crit_01) / (crit_05 - crit_01);
    if (t_stat < crit_10) return 0.05 + 0.05 * (t_stat - crit_05) / (crit_10 - crit_05);

    // t_stat > crit_10 => p > 0.10, плавно приближаем к 1
    double p = 0.10 + 0.90 * (1.0 - std::exp(-(t_stat - crit_10) / 0.5));
    return std::min(p, 0.99);
}

}

// Публичный метод
bool CointegrationTest::test(const std::vector<double>& price1,
                             const std::vector<double>& price2,
                             double& alpha, double& beta, double& p_value) {
    if (price1.size() < 50 || price2.size() < 50) {
        std::cerr << "[COINT] Not enough data: "
                  << price1.size() << " and " << price2.size() << "\n";
        return false;
    }

    auto [a, b] = ols(price1, price2);
    alpha = a;
    beta = b;

    std::vector<double> residuals;
    residuals.reserve(price1.size());
    for (size_t i = 0; i < price1.size(); ++i)
        residuals.push_back(price1[i] - (a + b * price2[i]));

    // Диагностика остатков
    double mean_res = std::accumulate(residuals.begin(), residuals.end(), 0.0) / residuals.size();
    double var_res = 0.0;
    for (double r : residuals) var_res += (r - mean_res) * (r - mean_res);
    var_res /= residuals.size();

    std::cout << "[COINT] OLS: " << price1.size() << " points, "
              << "alpha=" << a << ", beta=" << b
              << ", mean(resid)=" << mean_res
              << ", std(resid)=" << std::sqrt(var_res) << std::endl;

    p_value = adf_pvalue(residuals);
    std::cout << "[COINT] ADF p-value = " << p_value << std::endl;

    return p_value < 0.05;
}
