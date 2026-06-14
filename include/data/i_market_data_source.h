#pragma once
#include <vector>
#include <string>

class IHistoricalDataSource {
public:
    virtual ~IHistoricalDataSource() = default;
    virtual std::vector<double> fetch_klines(const std::string& symbol,
                                             const std::string& interval,
                                             int limit) = 0;
};
