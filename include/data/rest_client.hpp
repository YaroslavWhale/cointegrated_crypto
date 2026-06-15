#pragma once
#include "data/i_market_data_source.hpp"

class BinanceRestClient : public IHistoricalDataSource {
public:
    std::vector<double> fetch_klines(const std::string& symbol,
                                     const std::string& interval,
                                     int limit) override;
};
