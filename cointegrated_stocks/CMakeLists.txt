cmake_minimum_required(VERSION 3.16)

project(cointegrated_stocks LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(cpr REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(cointegrated_stocks main.cpp
    get_server.h
    Ticker.h
    TradingPair.h
    Parser.h
    get_signal.h
)

target_link_libraries(cointegrated_stocks
    PRIVATE
        cpr::cpr
        nlohmann_json::nlohmann_json
)

include(GNUInstallDirs)
install(TARGETS cointegrated_stocks
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
