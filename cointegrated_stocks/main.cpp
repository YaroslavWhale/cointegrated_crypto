#include <iostream>
#include <string>
#include <algorithm>
#include "api_client.h"
#include "trading_pair.h"
#include "signal_generator.h"

int main(int argc, char* argv[]) {
    std::string sym1 = "BTC";
    std::string sym2 = "ETH";

    if (argc >= 3) {
        sym1 = argv[1];
        sym2 = argv[2];
    }

    std::string interval = "1d";
    int limit = 100;

    // 1. Получаем исторические цены
    std::vector<double> prices1 = fetch_historical_prices(sym1, interval, limit);
    std::vector<double> prices2 = fetch_historical_prices(sym2, interval, limit);

    if (prices1.empty() || prices2.empty()) {
        std::cerr << "Failed to fetch data for one of the symbols.\n";
        return 1;
    }

    // 2. Синхронизируем длину (берём минимум)
    size_t n = std::min(prices1.size(), prices2.size());
    prices1.resize(n);
    prices2.resize(n);

    if (n < 50) {
        std::cerr << "Not enough historical data (need at least 50 candles).\n";
        return 1;
    }

    // 3. Создаём TradingPair и загружаем историю
    TradingPair pair(60, 30);   // окно регрессии = 60, окно спреда = 30
    pair.load_history(prices1, prices2);

    // 4. Печатаем результаты
    std::cout << "Hedge ratio (β) = " << pair.get_hedge_ratio() << std::endl;
    std::cout << "Current spread: " << pair.get_current_spread() << std::endl;
    std::cout << "Mean spread: " << pair.get_mean_spread() << std::endl;
    std::cout << "StdDev spread: " << pair.get_stddev_spread() << std::endl;
    std::cout << "Z-score: " << pair.get_z_score() << std::endl;
    std::cout << "\n>>> SIGNAL: " << generate_signal(pair, sym1, sym2) << std::endl;

  /*  // 5. (Демонстрация обновления) добавим последнюю цену ещё раз
    double last1 = prices1.back();
    double last2 = prices2.back();
    pair.add_prices(last1, last2);
    pair.update_beta();
    std::cout << "\nAfter adding the same last price again:\n";
    std::cout << "Z-score now: " << pair.get_z_score() << std::endl;
    std::cout << "Signal: " << generate_signal(pair, sym1, sym2) << std::endl;
*/
    return 0;
}
