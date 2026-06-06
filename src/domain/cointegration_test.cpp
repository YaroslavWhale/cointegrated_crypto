#include "domain/cointegration_test.h"
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <vector>

namespace {

// Обычная OLS: y = a + b*x
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

// Расширенный ADF тест с выбором количества лагов по AIC (макс. 5 лагов)
// Возвращает тестовую статистику (t-статистику для gamma)
double adf_test(const std::vector<double>& y, int max_lags = 5) {
    size_t n = y.size();
    if (n < 10) return 0.0;

    // Предварительно строим регрессию Δy_t = c + γ*y_{t-1} + Σ β_i Δy_{t-i}
    // Будем перебирать количество лагов p от 0 до max_lags
    double best_aic = 1e30;
    double best_gamma_t = 0.0;

    for (int p = 0; p <= max_lags; ++p) {
        size_t effective_n = n - p - 1; // используем наблюдения с индекса p+1
        if (effective_n < 10) continue;

        // Формируем матрицу регрессоров: первая колонка = y_{t-1}, затем p колонок Δy_{t-j}
        std::vector<std::vector<double>> X(effective_n, std::vector<double>(1 + p, 1.0)); // плюс константа? Нет, константу добавим отдельно
        // У нас модель: Δy_t = const + γ * y_{t-1} + Σ β_j Δy_{t-j} + ε
        // Положим const всегда включена. Удобнее вручную.
        // Создадим вектор X размера effective_n x (2+p): [1, y_{t-1}, Δy_{t-1}, ..., Δy_{t-p}]
        std::vector<std::vector<double>> Xmat(effective_n, std::vector<double>(2 + p, 0.0));
        std::vector<double> Y(effective_n);

        for (size_t i = 0; i < effective_n; ++i) {
            size_t idx = p + i; // индекс в исходном ряду, откуда берём Δy_t
            Y[i] = y[idx+1] - y[idx];
            Xmat[i][0] = 1.0;                     // константа
            Xmat[i][1] = y[idx];                  // y_{t-1}
            for (int j = 0; j < p; ++j) {
                Xmat[i][2+j] = y[idx - j] - y[idx - j - 1]; // Δy_{t-1-j}
            }
        }

        // Регрессия: решаем (X'X)⁻¹ X'Y. Для простоты используем явное решение через OLS для каждого коэффициента? Лучше через QR или метод наименьших квадратов.
        // Используем простую функцию решения нормальных уравнений.
        int nvars = 2 + p;
        std::vector<double> beta(nvars, 0.0);
        // Собираем X'X и X'Y
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
        // Решаем систему XtX * beta = XtY методом Гаусса-Жордана
        // Создадим расширенную матрицу
        std::vector<std::vector<double>> A(nvars, std::vector<double>(nvars+1, 0.0));
        for (int i = 0; i < nvars; ++i) {
            for (int j = 0; j < nvars; ++j) A[i][j] = XtX[i][j];
            A[i][nvars] = XtY[i];
        }
        // Прямой ход
        for (int i = 0; i < nvars; ++i) {
            // Частичный поворот
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
        // Извлекаем beta
        for (int i = 0; i < nvars; ++i) beta[i] = A[i][nvars];

        // gamma — это коэффициент при y_{t-1} (индекс 1)
        double gamma = beta[1];
        // Оцениваем остатки и стандартную ошибку gamma
        double rss = 0.0;
        for (size_t i = 0; i < effective_n; ++i) {
            double pred = 0.0;
            for (int j = 0; j < nvars; ++j) pred += beta[j] * Xmat[i][j];
            double e = Y[i] - pred;
            rss += e*e;
        }
        double sigma2 = rss / (effective_n - nvars);
        double se_gamma = sqrt(sigma2 * A[1][1]); // диагональный элемент обратной матрицы, но мы уже привели к единичной, так что в A[1][1] будет значение обратной матрицы? Нужно правильно вычислить SE.
        // После приведения к единичной диагонали, обратная матрица не хранится явно. Проще вычислить se через (X'X)^-1.
        // Мы не вычислили (X'X)^-1 явно. Лучше используем аналитический метод: var-cov = sigma^2 * (X'X)^{-1}.
        // Вычислим (X'X)^{-1} отдельно с помощью того же Гаусса-Жордана на единичную матрицу.
        // Для простоты сделаем это.

        // Создаём копию XtX и единичную матрицу
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
        if (invXtX.empty()) continue; // пропуск при вырожденной матрице

        double se_gamma_correct = sqrt(sigma2 * invXtX[1][1]);
        double t_gamma = gamma / se_gamma_correct;

        // AIC = n*log(RSS/n) + 2*k
        double aic = effective_n * log(rss/effective_n) + 2*nvars;
        if (aic < best_aic) {
            best_aic = aic;
            best_gamma_t = t_gamma;
        }
    }
    return best_gamma_t;
}

// Преобразование t-статистики ADF в p-value (аппроксимация MacKinnon, 1996, модель с константой)
double p_value_from_t(double t_stat, size_t n) {
    // Критические значения для ADF с константой (без тренда) зависят от n.
    // Используем формулу MacKinnon: C(p) = β0 + β1/T + β2/T^2, где T=n.
    // Для уровня значимости 1%: β0=-3.4335, β1=-5.999, β2=-29.25
    // 5%: -2.8621, -2.738, -8.36
    // 10%: -2.5671, -1.438, -4.48
    // Источник: MacKinnon (1996) "Numerical distribution functions..." Table 1 (constant, no trend).
    auto crit = [&](double b0, double b1, double b2) {
        return b0 + b1/n + b2/(n*n);
    };

    double c01 = crit(-3.4335, -5.999, -29.25);
    double c05 = crit(-2.8621, -2.738, -8.36);
    double c10 = crit(-2.5671, -1.438, -4.48);

    if (t_stat < c01) return 0.01;
    if (t_stat < c05) return 0.01 + 0.04 * (t_stat - c01) / (c05 - c01);
    if (t_stat < c10) return 0.05 + 0.05 * (t_stat - c05) / (c10 - c05);
    // p > 0.10
    double p = 0.10 + 0.90 * (1.0 - std::exp(-(t_stat - c10) / 0.5));
    return std::min(p, 0.99);
}

} // анонимный namespace

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
    p_value = p_value_from_t(t_stat, residuals.size());
    std::cout << "[COINT] ADF t-stat = " << t_stat << ", p-value = " << p_value << std::endl;

    return p_value < 0.05;
}
