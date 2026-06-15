#include "utils/statistical_utils.hpp"
#include "filters/kalman_filter_2d.hpp"
#include <cmath>
#include <limits>
#include <iostream>
#include <algorithm>

OLSResult compute_ols(const std::vector<double>& x, const std::vector<double>& y) {
    size_t n = std::min(x.size(), y.size());
    double sx=0, sy=0, sxy=0, sxx=0;
    for (size_t i=0; i<n; ++i) {
        sx+=x[i]; sy+=y[i]; sxy+=x[i]*y[i]; sxx+=x[i]*x[i];
    }
    double b = (n*sxy - sx*sy) / (n*sxx - sx*sx);
    double a = (sy - b*sx) / n;
    return {a, b};
}

static std::vector<double> log_range(double base, double minv, double maxv, int steps) {
    double lb = std::log10(std::max(base, 1e-12));
    double lmin = std::log10(minv), lmax = std::log10(maxv);
    if (lb < lmin) lmin = lb - 0.5;
    if (lb > lmax) lmax = lb + 0.5;
    double step = (lmax - lmin) / (steps-1);
    std::vector<double> vals;
    for (int i=0; i<steps; ++i)
        vals.push_back(std::max(minv, std::min(maxv, std::pow(10.0, lmin + i*step))));
    return vals;
}

KalmanParams optimize_kalman_2d(const std::vector<double>& log_p1,
                                const std::vector<double>& log_p2,
                                double init_alpha, double init_beta) {
    size_t n = log_p1.size();
    double var_res = 0.0;
    for (size_t i=0; i<n; ++i) {
        double res = log_p1[i] - (init_alpha + init_beta * log_p2[i]);
        var_res += res*res;
    }
    var_res /= (n-2);
    double R_guess = std::max(var_res, 1e-10);

    int w = std::min(30, (int)n/2);
    std::vector<double> alphas, betas;
    for (size_t i=0; i+w <= n; i += w/2) {
        auto o = compute_ols(std::vector<double>(log_p2.begin()+i, log_p2.begin()+i+w),
                             std::vector<double>(log_p1.begin()+i, log_p1.begin()+i+w));
        alphas.push_back(o.alpha);
        betas.push_back(o.beta);
    }
    double var_a=0, var_b=0;
    if (alphas.size()>1) {
        double ma=0, mb=0;
        for (size_t i=0;i<alphas.size();++i){ma+=alphas[i]; mb+=betas[i];}
        ma/=alphas.size(); mb/=betas.size();
        for (size_t i=0;i<alphas.size();++i){
            var_a += (alphas[i]-ma)*(alphas[i]-ma);
            var_b += (betas[i]-mb)*(betas[i]-mb);
        }
        var_a/=(alphas.size()-1); var_b/=(betas.size()-1);
    }
    double Qa = std::max(1e-12, var_a);
    double Qb = std::max(1e-12, var_b);

    auto evaluate = [&](double R, double qa, double qb) {
        KalmanFilter2D kf(R, qa, qb, init_alpha, init_beta);
        double logL = 0.0;
        for (size_t i=0; i<n; ++i) {
            kf.update(log_p1[i], log_p2[i]);
            double innov = kf.innovation();
            double S = kf.innovation_variance();
            if (S <= 1e-12) S = 1e-12;
            logL += -0.5 * (std::log(S) + innov*innov/S);
        }
        return logL;
    };

    auto Rs = log_range(R_guess, 1e-4, 1.0, 6);
    auto Qas = log_range(Qa, 1e-12, 1e-10, 6);
    auto Qbs = log_range(Qb, 1e-12, 1e-10, 6);

    double bestL = -1e30;
    KalmanParams best = {R_guess, Qa, Qb};
    for (double r : Rs) for (double qa : Qas) for (double qb : Qbs) {
                double L = evaluate(r, qa, qb);
                if (L > bestL) {
                    bestL = L;
                    best = {r, qa, qb};
                }
            }
    std::cout << "[OPT] Best: R=" << best.R << " Q_alpha=" << best.Q_alpha
              << " Q_beta=" << best.Q_beta << " (logL=" << bestL << ")\n";
    return best;
}
