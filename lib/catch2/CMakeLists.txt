#
# Create a library target for the Catch header-only test framework.
#
add_library(catch2  SHARED
        catch.hpp
        catch_main.cpp)

set_target_properties(catch2 PROPERTIES PUBLIC_HEADER .)

target_include_directories(catch2
        PUBLIC
        .
        )

target_compile_options(catch2
        PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Werror -Wall -Wextra>
        $<$<CXX_COMPILER_ID:MSVC>:
        /W4>
        )