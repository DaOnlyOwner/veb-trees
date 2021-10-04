cmake_minimum_required(VERSION 3.8)
project(veb_trees LANGUAGES CXX C)

function(stp tname)
    set_target_properties(${tname}
        PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
    )
endfunction()

add_executable(veb_trees "veb_test.cpp" "veb.h")
stp(veb_trees)