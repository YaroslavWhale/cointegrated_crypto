# Парный трейдинг фильтром Калмана с проверкой на коинтеграцию тестом Энгла‑Грейнджера

**предупреждение:** Проект находится в стадии разработки и **не пригоден для реальной торговли**.  

## Описание

Стратегия парного трейдинга

- Проверка долгосрочной связи через **тест Энгла–Грейнджера** на логарифмах цен (α, β - метод наименьших квадратов OLS log(P1) = α + β·log(P2); остатки ε проверяются расширенным ADF-тестом; p-value < 0.05 - ряды коинтегрированы).  
- Динамическая оценка параметров спреда с помощью **фильтра Калмана** ([α, β] меняется как случайное блуждание).  
- Вычисление **z-score**(оценки) спреда - сигнал.  
- **На старте:** исторические часовые свечи загружаются через REST API (cpr) для первичного теста коинтеграции.  
- **В реальном времени:** минутные данные поступают через WebSocket и используются для обновления Калмана и расчёта спреда, а также агрегируются в часовые и сохраняются в буфер для периодической перепроверки коинтеграции (раз в N минут).

## Зависимости

- CMake ≥ 3.16
- Компилятор с поддержкой C++17 (gcc, clang)
- Boost (system, thread)
- OpenSSL
- nlohmann_json
- websocketpp
- cpr (HTTP-клиент на основе libcurl)

### Зависимостей

**Ubuntu / Debian**
```bash
sudo apt install cmake g++ libboost-system-dev libboost-thread-dev libssl-dev nlohmann-json3-dev libwebsocketpp-dev libcurl4-openssl-dev
```
cpr отсутствует в репозиториях, установите его из исходников:
```bash
git clone https://github.com/libcpr/cpr.git && cd cpr && mkdir build && cd build && cmake .. -DCPR_USE_SYSTEM_CURL=ON && make && sudo make install
```

**Fedora / RHEL / CentOS**

```bash
sudo dnf install cmake gcc-c++ boost-devel openssl-devel nlohmann-json-devel websocketpp-devel libcurl-devel
```
(cpr отсутствует в репозиториях, установите аналогично).

**Arch Linux**

```bash
sudo pacman -S cmake gcc boost openssl nlohmann-json websocketpp curl
```
cpr доступен в AUR: yay -S cpr (или соберите из исходников).

**Компиляция и сборка**
```bash
git clone https://github.com/ahsoka25/cointegrated_crypto.git
cd cointegrated_crypto
mkdir build && cd build
cmake ..
make
```

Исполняемый файл cointegrated_crypto появится в каталоге build/.

**Запуск**
```bash
./cointegrated_crypto --sym1 BTC --sym2 ETH --window 50 --coint_check 60
```

**Параметры:**
```
--sym1	Тикер первого актива
--sym2	Тикер второго актива
--window	Размер окна для расчёта z-оценки (по умолчанию 50)
--coint_check	Периодичность перепроверки коинтеграции (в минутах. по умолчанию 60)
```

**Архитектура**

```text
cointegrated_crypto/
│
├── CMakeLists.txt
│
├── include/
│   ├── app/
│   │   └── trading_engine.h
│   ├── domain/
│   │   ├── kalman_filter.h
│   │   ├── spread_analyzer.h
│   │   └── cointegration_test.h
│   └── data/
│       ├── websocket_client.h
│       └── rest_client.h
│
 ── src/
    ├── main.cpp
    ├── app/
    │   └── trading_engine.cpp
    ├── domain/
    │   ├── kalman_filter.cpp
    │   ├── spread_analyzer.cpp
    │   └── cointegration_test.cpp
    └── data/
        ├── websocket_client.cpp
        └── rest_client.cpp
```
