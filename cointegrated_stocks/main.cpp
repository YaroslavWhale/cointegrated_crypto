#include <iostream>
#include <string>
#include <vector>
#include "get_server.h"
#include "Parser.h"
#include "Ticker.h"
#include "TradingPair.h"

int main(int argc, char* argv[]) {
    // 1. Определяем тикеры (например, BTC и ETH)
    std::string sym1 = "BTC";
    std::string sym2 = "ETH";

    if (argc >= 3) {
        sym1 = argv[1];
        sym2 = argv[2];
    }

    // 2. Создаём объекты для получения исторических данных
    server_get api1, api2;
    api1.set_symbol(sym1);
    api2.set_symbol(sym2);

    // Настройки интервала и количества свечей
    std::string interval = "1d";    // дневные свечи
    int limit = 100;                // последние 100 дней
    api1.set_interval(interval);
    api1.set_limit(limit);
    api2.set_interval(interval);
    api2.set_limit(limit);

    // 3. Получаем JSON от Binance
    std::string json1 = api1.get_historical_data();
    std::string json2 = api2.get_historical_data();

    if (json1.empty() || json2.empty()) {
        std::cerr << "Failed to fetch data for one of the symbols.\n";
        return 1;
    }

    // 4. Парсим JSON в объекты Ticker
    Ticker ticker1, ticker2;
    ticker1.set_symbol(sym1);
    ticker2.set_symbol(sym2);

    parse_historical_data(json1, ticker1);
    parse_historical_data(json2, ticker2);

    // 5. Проверяем, что истории синхронизированы по длине (берём минимум)
    size_t n1 = ticker1.get_history().size();
    size_t n2 = ticker2.get_history().size();
    size_t n = std::min(n1, n2);
    if (n < 50) {
        std::cerr << "Not enough historical data (need at least 50).\n";
        return 1;
    }

    // 6. Создаём торговую пару
    TradingPair pair(sym1, sym2, 60, 30);   // окно регрессии = 60, окно спреда = 30

    // Заполняем историческими ценами (пока без обновления β – сначала просто передаём цены)
    for (size_t i = 0; i < n; ++i) {
        pair.add_prices(ticker1.get_history()[i], ticker2.get_history()[i]);
    }

    // 7. Один раз вычисляем β по последним regression_window точкам
    pair.update_beta();
    std::cout << "Hedge ratio (β) = " << pair.get_hedge_ratio() << std::endl;

    // 8. Выводим последние значения спреда и Z‑score
    std::cout << "Current spread: " << pair.get_current_spread() << std::endl;
    std::cout << "Mean spread: " << pair.get_mean_spread() << std::endl;
    std::cout << "StdDev spread: " << pair.get_stddev_spread() << std::endl;
    std::cout << "Z-score: " << pair.get_z_score() << std::endl;

    // 9. Генерируем сигнал
    std::cout << "\n>>> SIGNAL: " << pair.get_signal() << std::endl;

    // 10. (Опционально) можно смоделировать добавление новой цены и обновление Z‑score
    //     Например, берём последнюю цену и добавляем её снова
    double last1 = ticker1.get_history().back();
    double last2 = ticker2.get_history().back();
    pair.add_prices(last1, last2);
    pair.update_beta();                // пересчитываем β с учётом новых данных
    std::cout << "\nAfter adding the same last price again:\n";
    std::cout << "Z-score now: " << pair.get_z_score() << std::endl;
    std::cout << "Signal: " << pair.get_signal() << std::endl;

    return 0;
}
