cmake_minimum_required(VERSION 3.20)
project(FalcomBGMInfo)

add_subdirectory(src/FalcomModernBGMInfo)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_subdirectory(src/FalcomRetroBGMInfo)
else()
    message(
        STATUS
        "Skipping FalcomRetroBGMInfo: the Retro proxy is x86-only."
    )
endif()