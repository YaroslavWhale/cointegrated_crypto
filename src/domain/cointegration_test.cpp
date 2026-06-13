#include "domain/cointegration_test.h"
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <vector>

namespace {

// OLS: y = a + b*x
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

double adf_test(const std::vector<double>& y, int max_lags = 5) {
    size_t n = y.size();
    if (n < 10) return 0.0;

    double best_aic = 1e30;
    double best_gamma_t = 0.0;

    for (int p = 0; p <= max_lags; ++p) {
        size_t effective_n = n - p - 1;
        if (effective_n < 10) continue;

        std::vector<std::vector<double>> Xmat(effective_n, std::vector<double>(2 + p, 0.0));
        std::vector<double> Y(effective_n);

        for (size_t i = 0; i < effective_n; ++i) {
            size_t idx = p + i;
            Y[i] = y[idx+1] - y[idx];
            Xmat[i][0] = 1.0;
            Xmat[i][1] = y[idx];
            for (int j = 0; j < p; ++j) {
                Xmat[i][2+j] = y[idx - j] - y[idx - j - 1];
            }
        }

        // OLS для ADF регрессии
        int nvars = 2 + p;
        std::vector<double> beta(nvars, 0.0);
        std::vector<std::vector<double>> XtX(nvars, std::vector<double>(nvars, 0.0));
        std::vector<double> XtY(nvars, 0.0);
        for (size_t i = 0; i < effective_n; ++i) {
            for (int j = 0; j < nvars; ++j) {
                double xj = Xmat[i][j];
                XtY[j] += xj * Y[i];
                for (int k = 0; k < nvars; ++k) {
                    XtX[j][k] += xj * Xmat[i][k];
                }
            }
        }

        std::vector<std::vector<double>> A(nvars, std::vector<double>(nvars+1, 0.0));
        for (int i = 0; i < nvars; ++i) {
            for (int j = 0; j < nvars; ++j) A[i][j] = XtX[i][j];
            A[i][nvars] = XtY[i];
        }
        for (int i = 0; i < nvars; ++i) {
            int max_row = i;
            for (int r = i+1; r < nvars; ++r) if (fabs(A[r][i]) > fabs(A[max_row][i])) max_row = r;
            std::swap(A[i], A[max_row]);
            double piv = A[i][i];
            if (fabs(piv) < 1e-12) continue;
            for (int j = i; j <= nvars; ++j) A[i][j] /= piv;
            for (int r = 0; r < nvars; ++r) {
                if (r == i) continue;
                double factor = A[r][i];
                for (int j = i; j <= nvars; ++j) A[r][j] -= factor * A[i][j];
            }
        }
        for (int i = 0; i < nvars; ++i) beta[i] = A[i][nvars];

        double gamma = beta[1];
        double rss = 0.0;
        for (size_t i = 0; i < effective_n; ++i) {
            double pred = 0.0;
            for (int j = 0; j < nvars; ++j) pred += beta[j] * Xmat[i][j];
            double e = Y[i] - pred;
            rss += e*e;
        }
        double sigma2 = rss / (effective_n - nvars);
        std::vector<std::vector<double>> invXtX(nvars, std::vector<double>(nvars, 0.0));
        for (int i = 0; i < nvars; ++i) invXtX[i][i] = 1.0;
        std::vector<std::vector<double>> XtX_copy = XtX;
        for (int i = 0; i < nvars; ++i) {
            double piv = XtX_copy[i][i];
            if (fabs(piv) < 1e-12) { invXtX.clear(); break; }
            for (int j = 0; j < nvars; ++j) {
                XtX_copy[i][j] /= piv;
                invXtX[i][j] /= piv;
            }
            for (int r = 0; r < nvars; ++r) {
                if (r == i) continue;
                double factor = XtX_copy[r][i];
                for (int j = 0; j < nvars; ++j) {
                    XtX_copy[r][j] -= factor * XtX_copy[i][j];
                    invXtX[r][j] -= factor * invXtX[i][j];
                }
            }
        }
        if (invXtX.empty()) continue;
        double se_gamma = sqrt(sigma2 * invXtX[1][1]);
        double t_gamma = gamma / se_gamma;

        double aic = effective_n * log(rss/effective_n) + 2*nvars;
        if (aic < best_aic) {
            best_aic = aic;
            best_gamma_t = t_gamma;
        }
    }
    return best_gamma_t;
}

double p_value_eg(double t_stat, size_t n) {
    const double c01 = -3.90;
    const double c05 = -3.34;
    const double c10 = -3.04;

    if (t_stat <= c01) return 0.01;
    if (t_stat <= c05) return 0.01 + 0.04 * (t_stat - c01) / (c05 - c01);
    if (t_stat <= c10) return 0.05 + 0.05 * (t_stat - c05) / (c10 - c05);
    double p = 0.10 + 0.90 * (1.0 - std::exp(-(t_stat - c10) / 0.5));
    return std::min(p, 0.99);
}

}

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

    double mean_res = std::accumulate(residuals.begin(), residuals.end(), 0.0) / residuals.size();
    double var_res = 0.0;
    for (double r : residuals) var_res += (r - mean_res) * (r - mean_res);
    var_res /= residuals.size();
    std::cout << "[COINT] OLS: " << price1.size() << " points, "
              << "alpha=" << a << ", beta=" << b
              << ", mean(resid)=" << mean_res
              << ", std(resid)=" << std::sqrt(var_res) << std::endl;

    double t_stat = adf_test(residuals);
    p_value = p_value_eg(t_stat, residuals.size());
    std::cout << "[COINT] ADF t-stat = " << t_stat << ", p-value = " << p_value << std::endl;

    return p_value < 0.05;
}
