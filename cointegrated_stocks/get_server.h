#ifndef SERVER_GET_H
#define SERVER_GET_H

#include <iostream>
#include <string>
#include <cpr/cpr.h>
#include <map>

class server_get {
private:
    // Базовый URL Binance API
    const std::string BASE_URL = "https://api.binance.com/api/v3/";

    // Параметры запроса
    std::string ticker_symbol;   // Пользовательский символ (BTC, ETH и т.д.)
    std::string interval;        // Интервал свечи (1m, 5m, 1h, 1d и т.д.)
    int limit;                   // Количество свечей (по умолчанию 10)

    // Словарь: пользовательский символ -> пара для Binance
    std::map<std::string, std::string> symbol_to_pair = {
        {"BTC", "BTCUSDT"},
        {"ETH", "ETHUSDT"},
        {"BNB", "BNBUSDT"},
        {"SOL", "SOLUSDT"},
        {"XRP", "XRPUSDT"},
        {"ADA", "ADAUSDT"},
        {"DOGE", "DOGEUSDT"},
        {"LTC", "LTCUSDT"},
        {"DOT", "DOTUSDT"},
        {"MATIC", "MATICUSDT"}
    };

    // Словарь: секунды -> интервал Binance
    std::map<int, std::string> seconds_to_interval = {
        {60, "1m"},      // 1 минута
        {300, "5m"},     // 5 минут
        {900, "15m"},    // 15 минут
        {1800, "30m"},   // 30 минут
        {3600, "1h"},    // 1 час
        {7200, "2h"},    // 2 часа
        {14400, "4h"},   // 4 часа
        {28800, "8h"},   // 8 часов
        {43200, "12h"},  // 12 часов
        {86400, "1d"},   // 1 день
        {604800, "1w"}   // 1 неделя
    };

    // Вспомогательный метод: преобразует символ в верхний регистр
    void to_upper(std::string& str) {
        for(char &c : str) {
            c = toupper(c);
        }
    }

    // Вспомогательный метод: проверяет, поддерживается ли символ
    bool is_symbol_supported(const std::string& symbol) {
        return symbol_to_pair.find(symbol) != symbol_to_pair.end();
    }

    // Вспомогательный метод: получает пару для Binance
    std::string get_pair(const std::string& symbol) {
        if (is_symbol_supported(symbol)) {
            return symbol_to_pair[symbol];
        }
        return "";
    }

public:
    // Конструктор по умолчанию (дневные свечи, 10 штук)
    server_get() : interval("1d"), limit(10) {}

    // Конструктор с параметрами
    server_get(const std::string& intrvl, int lim)
        : interval(intrvl), limit(lim) {}

    // Установка символа (BTC, ETH и т.д.)
    void set_symbol(const std::string& symbol) {
        ticker_symbol = symbol;
        to_upper(ticker_symbol);

        if (!is_symbol_supported(ticker_symbol)) {
            std::cout << "Warning: '" << ticker_symbol << "' not in supported symbols list" << std::endl;
            std::cout << "Supported symbols: ";
            for (const auto& pair : symbol_to_pair) {
                std::cout << pair.first << " ";
            }
            std::cout << std::endl;
        }
    }

    // Установка интервала строкой ("1m", "5m", "1h", "1d" и т.д.)
    void set_interval(const std::string& intrvl) {
        interval = intrvl;
    }

    // Установка интервала в секундах (60, 300, 3600, 86400 и т.д.)
    void set_interval(int seconds) {
        if (seconds_to_interval.find(seconds) != seconds_to_interval.end()) {
            interval = seconds_to_interval[seconds];
        } else {
            std::cout << "Warning: " << seconds << " seconds not supported. Using default '1d'" << std::endl;
            interval = "1d";
        }
    }

    // Установка лимита (количество свечей)
    void set_limit(int lim) {
        limit = lim;
    }

    void cin_symbol() {
        std::string input;

        // Ввод символа
        std::cout << "Enter ticker symbol (BTC, ETH, SOL, etc.): ";
        std::cin >> input;
        set_symbol(input);

        // Очищаем буфер после cin >>
        std::cin.ignore();

        // Ввод интервала (опционально)
        std::cout << "Enter interval (1m, 5m, 15m, 1h, 4h, 1d, 1w) or press Enter for default (1d): ";
        std::string interv_input;
        std::getline(std::cin, interv_input);
        if (!interv_input.empty()) {
            interval = interv_input;
        }

        // Ввод лимита (опционально)
        std::cout << "Enter limit (number of candles, 1-1000) or press Enter for default (10): ";
        std::string limit_input;
        std::getline(std::cin, limit_input);
        if (!limit_input.empty()) {
            limit = std::stoi(limit_input);
        }
    }

    // URL для исторических данных (OHLC свечи)
    std::string get_historical_url() {
        std::string pair = get_pair(ticker_symbol);
        if (pair.empty()) {
            return "";
        }

        return BASE_URL + "klines?symbol=" + pair +
               "&interval=" + interval +
               "&limit=" + std::to_string(limit);
    }

    // URL для текущей цены (24-часовая статистика)
    std::string get_current_price_url() {
        std::string pair = get_pair(ticker_symbol);
        if (pair.empty()) {
            return "";
        }

        return BASE_URL + "ticker/24hr?symbol=" + pair;
    }

    // URL для последней цены (простой тикер)
    std::string get_last_price_url() {
        std::string pair = get_pair(ticker_symbol);
        if (pair.empty()) {
            return "";
        }

        return BASE_URL + "ticker/price?symbol=" + pair;
    }

    // Получение исторических данных (OHLC свечи)
    std::string get_historical_data() {
        std::string url = get_historical_url();
        if (url.empty()) {
            std::cerr << "Error: Symbol '" << ticker_symbol << "' is not supported" << std::endl;
            return "";
        }

        std::cout << "Requesting historical data: " << url << std::endl;

        cpr::Response r = cpr::Get(cpr::Url{url});

        if (r.status_code != 200) {
            std::cerr << "HTTP error: " << r.status_code << std::endl;
            return "";
        }

        if (r.text.empty()) {
            std::cerr << "Error: Empty response" << std::endl;
            return "";
        }

        return r.text;
    }

    // Получение текущей цены (24-часовая статистика)
    std::string get_current_price() {
        std::string url = get_current_price_url();
        if (url.empty()) {
            std::cerr << "Error: Symbol '" << ticker_symbol << "' is not supported" << std::endl;
            return "";
        }

        std::cout << "Requesting current price: " << url << std::endl;

        cpr::Response r = cpr::Get(cpr::Url{url});

        if (r.status_code != 200) {
            std::cerr << "HTTP error: " << r.status_code << std::endl;
            return "";
        }

        return r.text;
    }

    // Получение последней цены (простой тикер)
    std::string get_last_price() {
        std::string url = get_last_price_url();
        if (url.empty()) {
            std::cerr << "Error: Symbol '" << ticker_symbol << "' is not supported" << std::endl;
            return "";
        }

        std::cout << "Requesting last price: " << url << std::endl;

        cpr::Response r = cpr::Get(cpr::Url{url});

        if (r.status_code != 200) {
            std::cerr << "HTTP error: " << r.status_code << std::endl;
            return "";
        }

        return r.text;
    }

    // Метод для обратной совместимости с вашим старым кодом
    std::string get_responce() {
        return get_historical_data();
    }

    // Вывод списка поддерживаемых символов
    void print_supported_symbols() {
        std::cout << "Supported symbols:" << std::endl;
        for (const auto& pair : symbol_to_pair) {
            std::cout << "  " << pair.first << " -> " << pair.second << std::endl;
        }
    }

    void print_settings() {
        std::cout << "Current settings:" << std::endl;
        std::cout << "  Symbol: " << (ticker_symbol.empty() ? "[not set]" : ticker_symbol) << std::endl;
        std::cout << "  Interval: " << interval << std::endl;
        std::cout << "  Limit: " << limit << std::endl;
    }

    std::string get_symbol() const { return ticker_symbol; }
    std::string get_interval() const { return interval; }
    int get_limit() const { return limit; }
};

#endif // SERVER_GET_H
