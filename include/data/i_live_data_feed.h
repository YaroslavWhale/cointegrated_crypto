#pragma once
#include "core/types.h"
#include <string>

class ILiveDataFeed {
public:
    virtual ~ILiveDataFeed() = default;
    virtual void set_callback(PairPriceCallback cb) = 0;
    virtual void subscribe(const std::string& symbol1, const std::string& symbol2) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
};
