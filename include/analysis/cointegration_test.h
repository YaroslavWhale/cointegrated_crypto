#pragma once
#include <vector>

struct CointegrationResult {
    double alpha;
    double beta;
    double p_value;
    bool is_cointegrated;
};

class CointegrationTest {
public:
    static CointegrationResult test(const std::vector<double>& price1,
                                    const std::vector<double>& price2);
};
