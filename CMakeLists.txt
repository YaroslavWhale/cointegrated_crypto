cmake_minimum_required(VERSION 3.16)
project(cointegrated_crypto LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(websocketpp REQUIRED)
find_package(OpenSSL REQUIRED)

set(SOURCES
    src/main.cpp
    src/app/trading_engine.cpp
    src/domain/kalman_filter.cpp
    src/domain/spread_analyzer.cpp
    src/data/websocket_client.cpp
)

set(HEADERS
    include/app/trading_engine.h
    include/domain/kalman_filter.h
    include/domain/spread_analyzer.h
    include/data/websocket_client.h
    include/websocketpp/config/asio_tls_client.hpp #локальный TLS-конфиг
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        nlohmann_json::nlohmann_json
        Boost::system
        Boost::thread
        websocketpp::websocketpp
        OpenSSL::SSL
        OpenSSL::Crypto
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
