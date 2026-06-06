#pragma once
#include <vector>

class CointegrationTest {
public:
    // price1, price2 – ожидаются логарифмы цен
    static bool test(const std::vector<double>& price1,
                     const std::vector<double>& price2,
                     double& alpha, double& beta, double& p_value);
};
///логика в архитектуре !?!?!??
