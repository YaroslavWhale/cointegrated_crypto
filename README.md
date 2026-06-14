# Парный трейдинг с адаптивным фильтром Калмана

**Предупреждение:** ПРОЕКТ НАХОДИТСЯ В СТАДИИ РАЗРАБОТКИ И **НЕ ПРИГОДЕН ДЛЯ РЕАЛЬНОЙ ТОРГОВЛИ**. Используйте только для образовательных и исследовательских целей.

## Описание

Стратегия реализует **парный трейдинг** с динамической оценкой спреда с помощью **адаптивного фильтра Калмана**. Фильтр Калмана позволяет коэффициентам hedge ratio (β) и α (сдвиг) эволюционировать во времени как случайное блуждание. Для повышения стабильности β используется **псевдоизмерение** (`apply_pseudo_beta`) — дополнительное наблюдение, подтягивающее оценку β к целевому значению (по умолчанию 1.0) с заданной дисперсией `R_beta`. Это помогает избежать дрейфа β при слабой коинтеграции.

### Как работает стратегия

1. **Начальная настройка**  
   - Загружаются исторические минутные данные (обычно 100 свечей) через REST API Binance(cpr).
   - Вычисляются логарифмы цен: `log(P1)` и `log(P2)`.
   - **OLS** даёт начальные оценки `α_init` и `β_init` для фильтра Калмана.

2. **Фильтр Калмана в реальном времени**  
   - Состояние: `x = [α, β]`.
   - Модель измерения:  
     `price1 = α + β * price2 + ε`, где `ε ~ N(0, R)`.  
   - Модель процесса (случайное блуждание):  
     `x_t = x_{t-1} + η`, где `η ~ N(0, Q)`.
   *Уравнения обновления:*
   - Предсказание:  
     `x_pred = x_prev`,  
     `P_pred = P_prev + Q`.  
   - Инновация (спред):  
     `y = price1 - (α_pred + β_pred * price2)`.  
   - Дисперсия инновации:  
     `S = H * P_pred * H^T + R`, где `H = [1, price2]`.  
   - Коэффициент Калмана:  
     `K = P_pred * H^T / S`.  
   - Коррекция:  
     `x_new = x_pred + K * y`,  
     `P_new = (I - K*H) * P_pred`.  
   - **Спред** – это инновация `y`.

3. **Генерация торговых сигналов**  
   Текущий спред формирует z-оценку:
   - z = (spread - μ) / σ, где μ и σ вычисляются по скользящему окну спреда (размер --window).
   - Адаптивный порог входа:
   - Ведётся история последних Z-оценок (размер окна --z-window).
   - Вычисляется скользящее стандартное отклонение Z (σ_z).
   - Порог = threshold_multiplier × σ_z (но не ниже min_threshold, чтобы избежать слишком частых сигналов).
   - Сигнал SELL (z > +current_threshold) и BUY (z < -current_threshold) подаются только после N последовательных подтверждений (чтобы избежать ложных срабатываний(в коде это явно задано static constexpr int REQUIRED_CONSECUTIVE = 1 в z_signal.h можно изменить).

4. **Поток данных**  
   - **WebSocket** подключение к Binance: подписка на минутные свечи (`1m`).  
   - При получении закрытой свечи обновляется фильтр Калмана и z-оценка.  
   - При возникновении сигнала исполняется сделка(в симуляторе).

## Зависимости

- CMake ≥ 3.16
- Компилятор с поддержкой C++17 (gcc, clang)
- Boost (system, thread)
- OpenSSL
- nlohmann_json
- websocketpp
- cpr (HTTP-клиент)

### **Установка зависимостей**

#### Ubuntu / Debian
```bash
sudo apt install cmake g++ libboost-system-dev libboost-thread-dev libssl-dev nlohmann-json3-dev libwebsocketpp-dev libcurl4-openssl-dev
```
cpr из исходников:

```
git clone https://github.com/libcpr/cpr.git && cd cpr && mkdir build && cd build && cmake .. -DCPR_USE_SYSTEM_CURL=ON && make && sudo make install
```
#### Fedora / RHEL / CentOS

```bash
sudo dnf install cmake gcc-c++ boost-devel openssl-devel nlohmann-json-devel websocketpp-devel libcurl-devel
```
cpr — из исходников (см. выше)

#### Arch Linux

```bash
sudo pacman -S cmake gcc boost openssl nlohmann-json websocketpp curl
```
cpr из AUR: yay -S cpr или собрать из исходников

## Сборка и запуск

```bash
git clone https://github.com/ahsoka25/cointegrated_crypto.git
cd cointegrated_crypto
mkdir build && cd build
cmake ..
make
```
Исполняемый файл cointegrated_crypto появится в /build.

```bash
./cointegrated_crypto --sym1 BTC --sym2 ETH --window 150 --pseudo_beta_R 0.5
```
Параметры:
- --sym1	Тикер первого актива	(по умолчанию BTC)
- --sym2	Тикер второго актива	(по умолчанию ETH)
- --window	Размер окна для расчёта z-оценки (по умолчанию 150)
- --pseudo_beta_R Дисперсия шума псевдоизмерения β (по умолчанию 0.5)
- --quote Валюта котировки (USDT для крипты, USD для акций)
- 
# Архитектура проекта

```text
cointegrated_crypto/
├── CMakeLists.txt
│
├── include/
│   ├── core/                     # Базовые типы и структуры данных
│   │   ├── instrument.h          #   Структура Instrument (symbol, base, quote)
│   │   └── types.h               #   Callback-типы (PairPriceCallback)
│   │
│   ├── data/                     # Слой доступа к данным
│   │   ├── i_market_data_source.h #   Интерфейс исторических данных
│   │   ├── i_live_data_feed.h     #   Интерфейс live-потока (WebSocket)
│   │   ├── rest_client.h          #   Binance REST API
│   │   └── websocket_feed.h       #   Binance WebSocket
│   │
│   ├── filters/                  # Математические модели оценки состояния
│   │   ├── i_state_estimator.h    #   Интерфейс фильтра (update, get_spread, ...)
│   │   ├── kalman_filter_2d.h     #   Калман 2D (α, β) с псевдоизмерением β
│   │   └── kalman_filter_3d.h     #   Заглушка для будущего фильтра 3D (α, β, γ)
│   │
│   ├── analysis/                 # Анализ спреда и генерация сигналов
│   │   ├── spread_analyzer.h      #   Вычисление Z-score, адаптивный порог
│   │   └── cointegration_test.h   #   Тест Энгла-Грейнджера (пока не активен)
│   │
│   ├── portfolio/                # Управление капиталом и позицией
│   │   ├── i_portfolio.h          #   Интерфейс портфеля
│   │   └── simple_portfolio.h     #   Симулятор портфеля с фиксированным плечом
│   │
│   ├── strategy/                 # Торговая логика
│   │   └── pair_trading_strategy.h #  Основная стратегия (связывает фильтр, анализатор, портфель)
│   │
│   ├── application/              # Конфигурация и запуск приложения
│   │   └── strategy_app.h         #  StrategyApplication + AppConfig
│   │
│   └── utils/                    # Вспомогательные функции
│       └── statistical_utils.h    #  OLS, оптимизация параметров Калмана
│
└── src/
    ├── main.cpp
    ├── application/
    │   └── strategy_app.cpp
    ├── analysis/
    │   ├── cointegration_test.cpp
    │   └── spread_analyzer.cpp
    ├── data/
    │   ├── rest_client.cpp
    │   └── websocket_feed.cpp
    ├── filters/
    │   └── kalman_filter_2d.cpp
    ├── portfolio/
    │   └── simple_portfolio.cpp
    ├── strategy/
    │   └── pair_trading_strategy.cpp
    └── utils/
        └── statistical_utils.cpp
```
