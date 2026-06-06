#pragma once
#include <string>

void run_live_strategy(const std::string& sym1,
                       const std::string& sym2,
                       int spread_window = 50,
                       int coint_check_minutes = 60);
