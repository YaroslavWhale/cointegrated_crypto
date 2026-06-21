# Парный трейдинг (2D Калман)

**Предупреждение:** ПРОЕКТ НАХОДИТСЯ В СТАДИИ РАЗРАБОТКИ И **НЕ ПРИГОДЕН ДЛЯ РЕАЛЬНОЙ ТОРГОВЛИ**. Используйте только для образовательных и исследовательских целей.

## Описание

Стратегия реализует парный трейдинг с динамической оценкой спреда с помощью фильтра Калмана с двумя состояниями (α, β). Коэффициенты эволюционируют как случайное блуждание.  
Генерация торговых сигналов использует **адаптивные пороги**, которые автоматически подстраиваются под текущую волатильность спреда.

## Как работает стратегия

### 1. Начальная настройка
- Загружаются исторические данные через REST API Binance.
- Вычисляются логарифмы цен: `log(P1)` и `log(P2)`.
- OLS даёт начальные оценки `α_init` и `β_init`.
- Выполняется оптимизация гиперпараметров (R, Q_α, Q_β) по логарифму правдоподобия.
- Создаётся 2D-фильтр Калмана и прогревается на истории.

### 2. Фильтр Калмана (2D)
- **Состояние:** `x = [α, β]^T`
- **Модель измерения:** `price1 = α + β * price2 + ε`,  `ε ~ N(0, R)`
- **Модель процесса (случайное блуждание):** `x_t = x_{t-1} + η`, `η ~ N(0, Q)`, `Q = diag(Q_α, Q_β)`
- **Инновация (спред):** `y = price1 - (α_pred + β_pred * price2)`
- Уравнения обновления стандартные для линейного фильтра Калмана.

### 3. Генерация торговых сигналов (динамические пороги)

- **Z-оценка:** `z = (spread - μ) / σ`, где μ и σ вычисляются по скользящему окну спреда.
- **Адаптивный базовый порог:**  
  `base_threshold = max(min_threshold, multiplier × σ_z)`  
  где `σ_z` – скользящее стандартное отклонение Z-оценок.
- **Волатильностное масштабирование:**  
  Пороги дополнительно умножаются на коэффициент масштабирования, зависящий от отношения текущей волатильности спреда к долгосрочной.
  scale = 1 + vol_scale_factor × (sqrt(ema_vol / long_term_vol) - 1)
  entry_threshold = base_entry × scale
  exit_threshold = base_exit × scale

  - **Раздельные уровни входа и выхода:**
  - Вход: `|z| > entry_threshold` (множитель обычно 2.0).
  - Выход: `|z| < exit_threshold` (множитель обычно 0.75).
  - Требуется подтверждение сигнала `required_consecutive_` раз подряд (по умолчанию 1).
  - **Торговые сигналы (типобезопасный enum):**
  - `ENTER_LONG` – покупка первого, продажа второго.
  - `ENTER_SHORT` – продажа первого, покупка второго.
  - `EXIT` – закрытие позиции при возврате Z внутрь exit_threshold.

### 4. Поток данных
- **REST API:** загрузка истории для инициализации и прогрева.
- **WebSocket Binance:** подписка на закрытые свечи с заданным интервалом.
- На каждом новом баре обновляется фильтр, Z-оценка и сигнал.

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
./cointegrated_crypto --sym1 BTC --sym2 ETH --window 150 -interval 1m
```
Параметры:
- --sym1	Тикер первого актива	(по умолчанию BTC)
- --sym2	Тикер второго актива	(по умолчанию ETH)
- --window Размер окна для расчёта z-оценки (по умолчанию 150)
- --interval Интервал свечей (по умолчанию 1m)

# Архитектура проекта

```text
cointegrated_crypto/
├── CMakeLists.txt
│
├── include/
│   ├── core/                     // Базовые типы и структуры данных
│   │   ├── instrument.hpp        //   Структура Instrument (symbol, base, quote)
│   │   └── types.hpp             //   Callback-типы (PairPriceCallback)
│   │
│   ├── data/                     // Слой доступа к данным
│   │   ├── i_market_data_source.hpp //   Интерфейс исторических данных
│   │   ├── i_live_data_feed.hpp    //   Интерфейс live-потока (WebSocket)
│   │   ├── rest_client.hpp         //   Binance REST API
│   │   └── websocket_feed.hpp      //   Binance WebSocket
│   │
│   ├── math/                    // Математические утилиты
│   │   └── matrix.hpp            //   Шаблонная матрица и вектор (для Калмана)
│   │
│   ├── filters/                  // Математические модели оценки состояния
│   │   ├── i_state_estimator.hpp      //   Интерфейс фильтра (update, get_spread, ...)
│   │   ├── i_pseudo_beta_estimator.hpp //  (не спользуется в стратегии просто есть :) )
│   │   ├── kalman_filter_base.hpp     //   Базовый шаблонный фильтр Калмана
│   │   ├── kalman_filter_2d.hpp       //   2D-фильтр (α, β)
│   │   └── kalman_filter_3d.hpp       //   3D-фильтр (α, β, γ) (не используется)
│   │
│   ├── analysis/                 // Анализ спреда и генерация сигналов
│   │   ├── spread_analyzer.hpp    //   Вычисление Z-score, адаптивный порог, счётчик подтверждений
│   │   └── cointegration_test.hpp //   Тест Энгла-Грейнджера (не спользуется в стратегии просто есть :) )
│   │
│   ├── portfolio/                // Управление капиталом и позицией
│   │   ├── i_portfolio.hpp        //   Интерфейс портфеля
│   │   └── simple_portfolio.hpp   //   Симулятор портфеля с фиксированным плечом
│   │
│   ├── strategy/                 // Торговая логика
│   │   └── pair_trading_strategy.hpp //  Основная стратегия (связывает фильтр, анализатор, портфель)
│   │
│   ├── application/              // Конфигурация и запуск приложения
│   │   └── strategy_app.hpp      //  StrategyApplication + AppConfig
│   │
│   └── utils/                    // Вспомогательные функции
│       └── statistical_utils.hpp //  OLS, оптимизация параметров Калмана
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
