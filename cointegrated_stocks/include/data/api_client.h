#pragma once
#include <string>
#include <vector>

// Загружает дневные цены закрытия с Binance.
// symbol – краткое имя (BTC, ETH, …)
// interval – интервал свечей (по умолчанию "1d")
// limit – количество свечей (по умолчанию 100)
std::vector<double> fetch_historical_prices(const std::string& symbol,
                                            const std::string& interval = "1d",
                                            int limit = 100);
