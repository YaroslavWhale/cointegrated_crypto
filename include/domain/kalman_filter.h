#pragma once
#include <array>

// Модель price1 = α + β * price2 с динамической оценкой через фильтр Калмана
class KalmanFilter {
public:
    // R – дисперсия шума измерения
    // Q_alpha, Q_beta – диагональ ковариации шума процесса
    KalmanFilter(double R, double Q_alpha, double Q_beta,
                 double init_alpha = 0.0, double init_beta = 1.0,
                 double init_P_alpha = 100.0, double init_P_beta = 100.0);

    // Обновить фильтр новой парой цен
    void update(double price1, double price2);

    // Текущий спред (инновация)
    double get_spread() const;
    double get_alpha() const;
    double get_beta() const;

    // Неопределённости оценок (для отладки)
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
