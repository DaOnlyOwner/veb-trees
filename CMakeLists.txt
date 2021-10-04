cmake_minimum_required(VERSION 3.8)
project(drop_in_libs LANGUAGES CXX C)

function(stp tname)
    set_target_properties(${tname}
        PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
    )
endfunction()

add_executable(veb_test "veb_test.cpp" "veb.h")
stp(veb_test)