cmake_minimum_required(VERSION 3.15)

project(FraudDetectionEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(FraudDetectionEngine src/main.cpp)

target_include_directories(FraudDetectionEngine PRIVATE
    "C:/Program Files/PostgreSQL/19/include"
)

target_link_libraries(FraudDetectionEngine PRIVATE
    "C:/Program Files/PostgreSQL/19/lib/libpq.lib"
)