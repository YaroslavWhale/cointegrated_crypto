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
    double get_innovation() const { return last_innovation_; }
    double get_predicted_variance() const { return last_S_; }

    void reset(double alpha, double beta, double P_alpha = 100.0, double P_beta = 100.0);

    void reset_adaptive();

private:
    double R_;
    double R_adaptive_;
    double R_base_;

    std::array<std::array<double, 2>, 2> Q_;
    std::array<std::array<double, 2>, 2> Q_base_;

    std::array<double, 2> x_;
    std::array<std::array<double, 2>, 2> P_;

    double last_innovation_;
    double last_S_;

    static constexpr double lambda_R_ = 0.95;
    static constexpr double threshold_innov_ = 3.0;
    static constexpr double Q_increase_factor_ = 1.1;
    static constexpr double Q_decrease_factor_ = 0.99;
    static constexpr double Q_min_ = 1e-8;
    static constexpr double Q_max_ = 1e-1;
    static constexpr double R_min_ = 1e-8;
    static constexpr double R_max_ = 1.0;
};
