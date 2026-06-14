#pragma once
#include "filters/i_state_estimator.h"

class KalmanFilter3D : public IStateEstimator {
public:
    void update(double, double) override {}
    double get_spread() const override { return 0.0; }
    void apply_pseudo_beta(double) override {}
    void reset() override {}
};
