cmake_minimum_required(VERSION 3.16)
project(cointegrated_crypto LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)

# Для CPR (если не установлен, замените на find_package(CURL))
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

set(SOURCES
    src/main.cpp
    src/app/trading_engine.cpp
    src/domain/kalman_filter.cpp
    src/domain/spread_analyzer.cpp
    src/domain/cointegration_test.cpp
    src/data/websocket_client.cpp
    src/data/rest_client.cpp
    src/simulation/portfolio_simulator.cpp
)

set(HEADERS
    include/app/trading_engine.h
    include/domain/kalman_filter.h
    include/domain/spread_analyzer.h
    include/domain/cointegration_test.h
    include/data/websocket_client.h
    include/data/rest_client.h
    include/simulation/portfolio_simulator.h
)

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
