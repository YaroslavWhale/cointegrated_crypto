#pragma once
#include <vector>

class CointegrationTest {
public:
    // Выполняет тест Энгла-Грейнджера.
    // price1, price2 – ряды цен (логарифмы или сами цены)
    // alpha, beta – оценки регрессии price1 = alpha + beta * price2
    // p_value – реальное приближённое p‑value (не всегда 0.99)
    // Возвращает true при p_value < 0.05
    static bool test(const std::vector<double>& price1,
                     const std::vector<double>& price2,
                     double& alpha, double& beta, double& p_value);
};
