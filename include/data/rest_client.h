#pragma once
#include <string>
#include <vector>

class RestClient {
public:
    static std::vector<double> fetch_klines(const std::string& symbol,
                                            const std::string& interval,
                                            int limit);
};
