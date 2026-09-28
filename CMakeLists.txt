cmake_minimum_required(VERSION 3.16)
project(msxdisa LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(msxdisa_core src/z80_decoder.cpp src/analyzer.cpp src/zma_writer.cpp)
target_include_directories(msxdisa_core PUBLIC include)

add_executable(msxdisa src/main.cpp)
target_link_libraries(msxdisa PRIVATE msxdisa_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(msxdisa_tests tests/test_decoder.cpp)
    target_link_libraries(msxdisa_tests PRIVATE msxdisa_core)
    add_test(NAME decoder COMMAND msxdisa_tests)

    add_executable(msxdisa_analyzer_tests tests/test_analyzer.cpp)
    target_link_libraries(msxdisa_analyzer_tests PRIVATE msxdisa_core)
    add_test(NAME analyzer COMMAND msxdisa_analyzer_tests)
endif()