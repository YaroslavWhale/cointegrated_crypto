cmake_minimum_required(VERSION 3.16)

project(cointegrated_crypto LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(cpr REQUIRED)

find_package(nlohmann_json CONFIG REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/app/trading_engine.cpp
    src/data/api_client.cpp
    src/domain/kalman_filter.cpp
    src/domain/spread_analyzer.cpp
)

set(HEADERS
    include/app/trading_engine.h
    include/data/api_client.h
    include/domain/kalman_filter.h
    include/domain/spread_analyzer.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        cpr::cpr
        nlohmann_json::nlohmann_json
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
