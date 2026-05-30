#pragma once
#include <string>
#include <vector>

// Запускает коинтеграционную стратегию на исторических данных
void run_strategy(const std::string& sym1,
                  const std::string& sym2,
                  int spread_window = 50,
                  const std::string& interval = "1d",
                  int limit = 100);
