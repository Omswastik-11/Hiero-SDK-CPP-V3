cmake_minimum_required(VERSION 3.20)

project(hiero_v3_sdk_prototype VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(hiero_v3_sdk
    src/client/client.cpp
    src/runtime/executor.cpp
    src/transport/in_memory_transport.cpp
)

target_include_directories(hiero_v3_sdk
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(hiero_v3_sdk PUBLIC cxx_std_17)

if (MSVC)
    target_compile_options(hiero_v3_sdk PRIVATE /W4 /permissive-)
else()
    target_compile_options(hiero_v3_sdk PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(transfer_and_balance
    examples/transfer_and_balance.cpp
)

target_link_libraries(transfer_and_balance PRIVATE hiero_v3_sdk)

include(CTest)
if (BUILD_TESTING)
    add_executable(minimal_v3_tests
        tests/minimal_v3_tests.cpp
    )
    target_link_libraries(minimal_v3_tests PRIVATE hiero_v3_sdk)
    add_test(NAME minimal_v3_tests COMMAND minimal_v3_tests)
endif()
