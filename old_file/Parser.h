#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "Ticker.h"

using json = nlohmann::json;

// Парсинг ответа Binance /klines и заполнение Ticker (цена закрытия)
void parse_historical_data(const std::string& json_str, Ticker& ticker) {
    try {
        json data = json::parse(json_str);

        if (!data.is_array()) {
            std::cerr << "Error: Expected array from Binance API" << std::endl;
            return;
        }

        ticker.clear_history();  // очищаем старую историю

        for (const auto& candle : data) {
            if (candle.size() < 5) continue;
            // цена закрытия – 5-й элемент (индекс 4)
            double close = std::stod(candle[4].get<std::string>());
            ticker.add_to_history(close);
        }

        // Устанавливаем текущую цену = последней цене в истории
        if (!ticker.get_history().empty()) {
            ticker.set_price(ticker.get_history().back());
        }

        std::cout << "Parsed " << ticker.get_history().size()
                  << " historical candles for " << ticker.get_symbol() << std::endl;

    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

#endif // PARSER_H
