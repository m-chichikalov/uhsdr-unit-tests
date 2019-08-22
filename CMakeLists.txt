cmake_minimum_required(VERSION 3.14)
project(uhsdr-unit-tests C CXX)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(lib/catch2)

set(CATCH_ALL_TESTS_SRCS
        tests/test.cpp
        )

add_executable(catch-all-tests ${CATCH_ALL_TESTS_SRCS})

target_include_directories(catch-all-tests PRIVATE
        .
        UHSDR/mchf-eclipse
        doubles
        doubles/dummy_includs
        )

target_link_libraries(catch-all-tests catch2)

add_custom_command(
        TARGET catch-all-tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/lib/catch2/libcatch2.dll ${CMAKE_CURRENT_BINARY_DIR}/.
)
