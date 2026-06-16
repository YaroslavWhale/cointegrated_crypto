#pragma once
#include "filters/i_state_estimator.hpp"

class IPseudoBetaEstimator : public IStateEstimator {
public:
    virtual void apply_pseudo_beta(double R_beta) = 0;
};
