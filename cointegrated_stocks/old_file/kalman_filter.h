#ifndef KALMAN_FILTER_H
#define KALMAN_FILTER_H

#include <array>
#include <limits>

// Простая реализация фильтра Калмана для модели:
// price1 = α + β * price2 + noise
// Состояние: x = [α, β]ᵀ
// Модель процесса: x_t = x_{t-1} + w, w ~ N(0, Q)
// Измерение: z_t = H_t * x_t + v, v ~ N(0, R), где H_t = [1, price2_t]
class KalmanFilter {
public:
    // Конструктор с возможностью задания начальных условий
    KalmanFilter(double R, double Q_alpha, double Q_beta,
                 double init_alpha = 0.0, double init_beta = 1.0,
                 double init_P_alpha = 100.0, double init_P_beta = 100.0)
        : R_(R)
        , Q_{{Q_alpha, 0.0}, {0.0, Q_beta}}
        , x_{{init_alpha, init_beta}}
        , P_{{init_P_alpha, 0.0}, {0.0, init_P_beta}} {}

    // Обновление фильтра новой парой цен (price1, price2)
    void update(double price1, double price2) {
        // ----- Прогноз (априорные оценки) -----
        // Так как F = I, прогноз состояния равен предыдущему
        std::array<double, 2> x_pred = x_;
        // Прогноз ковариации: P_pred = P + Q
        std::array<std::array<double, 2>, 2> P_pred;
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                P_pred[i][j] = P_[i][j] + Q_[i][j];
            }
        }

        // ----- Матрица измерения H = [1, price2] -----
        double H0 = 1.0;
        double H1 = price2;

        // ----- Инновация -----
        double predicted_price1 = x_pred[0] + H1 * x_pred[1];
        double y = price1 - predicted_price1;

        // ----- Ковариация инновации S = H * P_pred * H^T + R -----
        // Вычисляем H * P_pred (вектор-строка 1x2)
        double HP0 = H0 * P_pred[0][0] + H1 * P_pred[1][0];
        double HP1 = H0 * P_pred[0][1] + H1 * P_pred[1][1];
        double S = HP0 * H0 + HP1 * H1 + R_;

        // Защита от деления на ноль
        if (S < 1e-12) S = 1e-12;
        double invS = 1.0 / S;

        // ----- Коэффициент Калмана K = P_pred * H^T * inv(S) -----
        // Сначала вычисляем P_pred * H^T (вектор 2x1)
        double K0 = (P_pred[0][0] * H0 + P_pred[0][1] * H1) * invS;
        double K1 = (P_pred[1][0] * H0 + P_pred[1][1] * H1) * invS;

        // ----- Обновление состояния -----
        x_[0] = x_pred[0] + K0 * y;
        x_[1] = x_pred[1] + K1 * y;

        // ----- Обновление ковариации (симметричная форма) -----
        // P = P_pred - K * S * K^T
        double KSK0 = K0 * S * K0;
        double KSK1 = K1 * S * K1;
        double KSK01 = K0 * S * K1;

        P_[0][0] = P_pred[0][0] - KSK0;
        P_[0][1] = P_pred[0][1] - KSK01;
        P_[1][0] = P_pred[1][0] - KSK01;  // симметрично
        P_[1][1] = P_pred[1][1] - KSK1;
    }

    // Текущие оценки коэффициентов
    double get_alpha() const { return x_[0]; }
    double get_beta()  const { return x_[1]; }

    // Дисперсии оценок (неопределённость)
    double get_alpha_variance() const { return P_[0][0]; }
    double get_beta_variance()  const { return P_[1][1]; }

    // Спред (невязка) для заданных цен
    double get_spread(double price1, double price2) const {
        return price1 - (x_[0] + x_[1] * price2);
    }

    // Опционально: сброс состояния
    void reset(double alpha, double beta,
               double P_alpha = 100.0, double P_beta = 100.0) {
        x_[0] = alpha;
        x_[1] = beta;
        P_[0][0] = P_alpha;
        P_[1][1] = P_beta;
        P_[0][1] = P_[1][0] = 0.0;
    }

private:
    double R_;                                    // дисперсия шума измерения
    std::array<std::array<double, 2>, 2> Q_;      // ковариация шума процесса
    std::array<double, 2> x_;                    // состояние [α, β]
    std::array<std::array<double, 2>, 2> P_;      // ковариационная матрица ошибки
};

#endif // KALMAN_FILTER_H
