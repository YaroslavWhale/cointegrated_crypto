# Трейд коинтегрированными активами (3D Калман)

**Предупреждение:** ПРОЕКТ НАХОДИТСЯ В СТАДИИ РАЗРАБОТКИ И **НЕ ПРИГОДЕН ДЛЯ РЕАЛЬНОЙ ТОРГОВЛИ**. Используйте только для образовательных и исследовательских целей.

## Описание

Стратегия реализует **парный трейдинг** с динамической оценкой спреда с помощью **фильтра Калмана** с тремя состояниями (`α`, `β`, `γ`). Все три коэффициента эволюционируют во времени как случайное блуждание (модель процесса `x_t = x_{t-1} + η`, где `η ~ N(0, Q)`).

### Работает стратегия

1. **Начальная настройка**  
   - Загружаются исторические минутные данные через REST API Binance (cpr).
   - Вычисляются логарифмы цен: `log(P1)` и `log(P2)`.
   - **OLS** даёт начальные оценки `α_init` и `β_init` для фильтра Калмана, `γ_init` инициализируется нулём.
   - Выполняется **оптимизация гиперпараметров** фильтра (`R`, `Q_α`, `Q_β`, `Q_γ`) по логарифму правдоподобия на исторических данных.

2. **Фильтр Калмана в реальном времени (3D)**  
   - Состояние: `x = [α, β, γ]^T`.
   - Модель измерения:  
     `price1 = α + β * price2 + γ * price2^2 + ε`, где `ε ~ N(0, R)`.  
   - Модель процесса (случайное блуждание):  
     `x_t = x_{t-1} + η`, где `η ~ N(0, Q)`, `Q = diag(Q_α, Q_β, Q_γ)`.
   - **Уравнения обновления:**
     - Предсказание:  
       `x_pred = x_prev`,  
       `P_pred = P_prev + Q`.  
     - Инновация (спред):  
       `y = price1 - (α_pred + β_pred * price2 + γ_pred * price2^2)`.  
     - Дисперсия инновации:  
       `S = H * P_pred * H^T + R`, где `H = [1, price2, price2^2]`.  
     - Коэффициент Калмана:  
       `K = P_pred * H^T / S`.  
     - Коррекция:  
       `x_new = x_pred + K * y`,  
       `P_new = (I - K*H) * P_pred`.  
   - **Спред** – это инновация `y`.

3. **Генерация торговых сигналов**  
   - Текущий спред в z-оценку:  
     `z = (spread - μ) / σ`, где μ и σ вычисляются по скользящему окну спреда (размер `--window`).  
   - Адаптивный порог входа:  
     - Ведётся история последних Z-оценок (размер окна равен `--window`).  
     - Вычисляется скользящее стандартное отклонение Z (`σ_z`).  
     - Порог = `threshold_multiplier × σ_z` (но не ниже `min_threshold`).  
   - Сигналы `SELL_SYM1` (z > +threshold) и `BUY_SYM1` (z < -threshold) подаются только после `REQUIRED_CONSECUTIVE` последовательных подтверждений (задаётся в `spread_analyzer.hpp`, по умолчанию 1).  
   - Закрытие позиции происходит при возврате z к нулю (|z| < 0.5).

5. **Поток данных**  
   - **WebSocket** подключение к Binance: подписка на минутные свечи (`1m`).  
   - При получении новой закрытой свечи обновляется фильтр Калмана и z-оценка.  
   - При возникновении сигнала исполняется сделка (в симуляторе).

## Зависимости
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
./cointegrated_crypto --sym1 BTC --sym2 ETH --window 150
```
Параметры:
- --sym1	Тикер первого актива	(по умолчанию BTC)
- --sym2	Тикер второго актива	(по умолчанию ETH)
- --window	Размер окна для расчёта z-оценки (по умолчанию 150)
- --pseudo_beta_R (не используется в 3D) Дисперсия шума псевдоизмерения (заглушка)
- --quote Валюта котировки (USDT для крипты, USD для акций)

# Архитектура проекта

```text
cointegrated_crypto/
├── CMakeLists.txt
│
├── include/
│   ├── core/                     # Базовые типы и структуры данных
│   │   ├── instrument.hpp        #   Структура Instrument (symbol, base, quote)
│   │   └── types.hpp             #   Callback-типы (PairPriceCallback)
│   │
│   ├── data/                     # Слой доступа к данным
│   │   ├── i_market_data_source.hpp #   Интерфейс исторических данных
│   │   ├── i_live_data_feed.hpp    #   Интерфейс live-потока (WebSocket)
│   │   ├── rest_client.hpp         #   Binance REST API
│   │   └── websocket_feed.hpp      #   Binance WebSocket
│   │
│   ├── math/                    # Математические утилиты
│   │   └── matrix.hpp            #   Шаблонная матрица и вектор (для Калмана)
│   │
│   ├── filters/                  # Математические модели оценки состояния
│   │   ├── i_state_estimator.hpp      #   Интерфейс фильтра (update, get_spread, ...)
│   │   ├── i_pseudo_beta_estimator.hpp #   Интерфейс для псевдоизмерения β (для 2D)
│   │   ├── kalman_filter_base.hpp     #   Базовый шаблонный фильтр Калмана
│   │   ├── kalman_filter_2d.hpp       #   2D-фильтр (α, β) – устаревший, сохранён для совместимости
│   │   └── kalman_filter_3d.hpp       #   3D-фильтр (α, β, γ) с квадратичным членом
│   │
│   ├── analysis/                 # Анализ спреда и генерация сигналов
│   │   ├── spread_analyzer.hpp    #   Вычисление Z-score, адаптивный порог, счётчик подтверждений
│   │   └── cointegration_test.hpp #   Тест Энгла-Грейнджера (пока не активен)
│   │
│   ├── portfolio/                # Управление капиталом и позицией
│   │   ├── i_portfolio.hpp        #   Интерфейс портфеля
│   │   └── simple_portfolio.hpp   #   Симулятор портфеля с фиксированным плечом
│   │
│   ├── strategy/                 # Торговая логика
│   │   └── pair_trading_strategy.hpp #  Основная стратегия (связывает фильтр, анализатор, портфель)
│   │
│   ├── application/              # Конфигурация и запуск приложения
│   │   └── strategy_app.hpp      #  StrategyApplication + AppConfig
│   │
│   └── utils/                    # Вспомогательные функции
│       └── statistical_utils.hpp #  OLS, оптимизация параметров Калмана (2D и 3D)
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
    │   ├── kalman_filter_2d.cpp
    │   └── kalman_filter_3d.cpp
    ├── portfolio/
    │   └── simple_portfolio.cpp
    ├── strategy/
    │   └── pair_trading_strategy.cpp
    └── utils/
        └── statistical_utils.cpp
```
