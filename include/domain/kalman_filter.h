#pragma once
#include <array>

class KalmanFilter {
public:
    KalmanFilter(double R, double Q_alpha, double Q_beta,
                 double init_alpha = 0.0, double init_beta = 1.0,
                 double init_P_alpha = 100.0, double init_P_beta = 100.0);

    void update(double price1, double price2);

    double get_spread() const;
    double get_alpha() const;
    double get_beta() const;
    double get_alpha_uncertainty() const;
    double get_beta_uncertainty() const;

    void reset(double alpha, double beta, double P_alpha = 100.0, double P_beta = 100.0);

private:
    double R_;
    std::array<std::array<double, 2>, 2> Q_;
    std::array<double, 2> x_;
    std::array<std::array<double, 2>, 2> P_;
    double last_innovation_;
};
