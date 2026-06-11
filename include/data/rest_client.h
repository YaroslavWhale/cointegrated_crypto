#pragma once
#include <vector>
#include <string>

class RestClient {
public:
    static std::vector<double> fetch_klines(const std::string& symbol,
                                            const std::string& interval,
                                            int limit);
};
//int coint_check_minutes = 60);
