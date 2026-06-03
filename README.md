# Парный трейдинг криптовалюты фильтром Калмана
---
##  Зависимости

- **CMake** ≥ 3.16
- **C++17** компилятор (gcc, clang)
- **Boost** (system, thread)
- **OpenSSL**
- **nlohmann_json**
- **websocketpp**
---
##  Установка зависимостей

### Ubuntu / Debian
```bash
sudo apt install cmake g++ libboost-system-dev libboost-thread-dev libssl-dev nlohmann-json3-dev libwebsocketpp-dev
```

### Fedora / RHEL / CentOS
```bash
sudo dnf install cmake gcc-c++ boost-devel openssl-devel nlohmann-json-devel websocketpp-devel
```

### Arch Linux
```bash
sudo pacman -S cmake gcc boost openssl nlohmann-json websocketpp
```

## Компиляция и сборка
```bash
git clone https://github.com/ahsoka25/cointegrated_crypto.git
cd cointegrated_crypto
mkdir build && cd build
cmake ..
make
```

После успешной сборки исполняемый файл cointegrated_crypto появится в каталоге build/.

### Запуск
```bash
./cointegrated_crypto --sym1 BTC --sym2 ETH --window 50
```

### Параметры:
--sym1, --sym2 – тикеры из списка: BTC, ETH, BNB, SOL, XRP.
--window – размер окна для расчёта z‑оценки (по умолчанию 50).

## Структура проекта
```
cointegrated_crypto/
├── CMakeLists.txt
├── include/  //Заголовочные файлы
│ ├── kalman_filter.h
│ ├── spread_analyzer.h
│ ├── websocket_client.h
│ └── trading_engine.h
├── src/
│ ├── main.cpp
│ ├── domain/
│ │ ├── kalman_filter.cpp  //Фильтр Калмана (смещение + коэффициент)
│ │ └── spread_analyzer.cpp  //Буфер спреда, z-score, сигналы
│ ├── app/
│ │ └── trading_engine.cpp  //Основная логика стратегии
│ └── data/
│ └── websocket_client.cpp //WebSocket клиент для Binance
└── README.md
```
