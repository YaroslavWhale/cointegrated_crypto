cmake_minimum_required(VERSION 3.16)
project(cointegrated_crypto LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Зависимости
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)

find_package(cpr CONFIG)
if(NOT cpr_FOUND)
    message(FATAL_ERROR "cpr not found. Install CPR: https://github.com/libcpr/cpr")
endif()

find_path(WEBSOCKETPP_INCLUDE_DIR
    NAMES websocketpp/config/asio_client.hpp
    PATHS /usr/include /usr/local/include
)
if(NOT WEBSOCKETPP_INCLUDE_DIR)
    message(FATAL_ERROR "websocketpp not found. Install: sudo apt install libwebsocketpp-dev")
endif()

# ====== Явный список ВСЕХ исходников ======
set(SOURCES
    src/main.cpp
    src/application/strategy_app.cpp
    src/analysis/cointegration_test.cpp
    src/analysis/spread_analyzer.cpp
    src/data/rest_client.cpp
    src/data/websocket_feed.cpp
    src/filters/kalman_filter_2d.cpp
    src/portfolio/simple_portfolio.cpp
    src/strategy/pair_trading_strategy.cpp
    src/utils/statistical_utils.cpp
)

# ====== Явный список ВСЕХ заголовков (для отображения в Qt) ======
set(HEADERS
    include/core/instrument.h
    include/core/types.h
    include/data/i_market_data_source.h
    include/data/i_live_data_feed.h
    include/data/rest_client.h
    include/data/websocket_feed.h
    include/filters/i_state_estimator.h
    include/filters/kalman_filter_2d.h
    include/filters/kalman_filter_3d.h
    include/analysis/spread_analyzer.h
    include/analysis/cointegration_test.h
    include/portfolio/i_portfolio.h
    include/portfolio/simple_portfolio.h
    include/strategy/pair_trading_strategy.h
    include/application/strategy_app.h
    include/utils/statistical_utils.h
)

# Группировка в виртуальные папки для Qt Creator (как в старом проекте)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Sources" FILES ${SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Headers" FILES ${HEADERS})

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${WEBSOCKETPP_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        nlohmann_json::nlohmann_json
        Boost::system
        Boost::thread
        OpenSSL::SSL
        OpenSSL::Crypto
        cpr::cpr
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
