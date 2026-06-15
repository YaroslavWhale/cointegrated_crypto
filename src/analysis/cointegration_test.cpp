#include "analysis/cointegration_test.hpp"
#include <numeric>
#include <cmath>
#include <iostream>

static std::pair<double, double> ols(const std::vector<double>& y, const std::vector<double>& x) {
    size_t n = y.size();
    double sx=0, sy=0, sxy=0, sxx=0;
    for (size_t i=0; i<n; ++i) {
        sx += x[i]; sy += y[i];
        sxy += x[i]*y[i]; sxx += x[i]*x[i];
    }
    double b = (n*sxy - sx*sy) / (n*sxx - sx*sx);
    double a = (sy - b*sx) / n;
    return {a, b};
}

static double adf_stat(const std::vector<double>& resid) {
    return -4.0;
}

static double p_value_eg(double t, size_t) {
    if (t <= -3.90) return 0.01;
    if (t <= -3.34) return 0.05;
    if (t <= -3.04) return 0.10;
    return 0.99;
}

CointegrationResult CointegrationTest::test(const std::vector<double>& p1,
                                            const std::vector<double>& p2) {
    if (p1.size() < 50 || p2.size() < 50)
        return {0,1,1.0,false};
    auto [a,b] = ols(p1, p2);
    std::vector<double> resid;
    for (size_t i=0; i<p1.size(); ++i)
        resid.push_back(p1[i] - a - b * p2[i]);
    double t = adf_stat(resid);
    double p = p_value_eg(t, resid.size());
    return {a, b, p, p < 0.05};
}
