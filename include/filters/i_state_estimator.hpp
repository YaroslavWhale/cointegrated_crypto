#pragma once

class IStateEstimator {
public:
    virtual ~IStateEstimator() = default;
    virtual void update(double price1_log, double price2_log) = 0;
    virtual double get_spread() const = 0;
    virtual void reset() = 0;

    virtual double innovation() const = 0;
    virtual double innovation_variance() const = 0;
};
