cmake_minimum_required(VERSION 3.20)
project(FalcomModernBGMInfo)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

# =========================================================
# ARCHITECTURE DETECTION & CONFIGURATION
# =========================================================
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Configuring for x64 (64-bit)")

    # 1. Select x64 Source Files
    set(PROXY_SOURCES
            universal_proxy_x64.cpp
            universal_proxy_x64.h
    )

    # 2. Select x64 Definition File
    set(PROXY_DEF universal_proxy_x64.def)

else()
    message(STATUS "Configuring for x86 (32-bit)")


    # 1. Select x86 Source Files
    set(PROXY_SOURCES
            universal_proxy_x86.cpp
            universal_proxy_x86.h
    )

    # 2. Select x86 Definition File
    set(PROXY_DEF universal_proxy_x86.def)
endif()

# --- vcpkg Package Finding ---
find_package(yaml-cpp CONFIG REQUIRED)
find_package(minhook CONFIG REQUIRED)
find_package(directx-headers CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)

# --- Define Your DLL Target ---
add_library(FalcomModernBGMInfo SHARED
        main_dx11.cpp
        ${PROXY_SOURCES}  # <--- Only adds the files for the current architecture

        # --- ImGui Source Files ---
        "../include/imgui/imgui.cpp"
        "../include/imgui/imgui_draw.cpp"
        "../include/imgui/imgui_tables.cpp"
        "../include/imgui/imgui_widgets.cpp"

        "../include/imgui/backends/imgui_impl_dx11.cpp"
        "../include/imgui/backends/imgui_impl_win32.cpp"
)

# --- Link the specific Definition File ---
target_sources(FalcomModernBGMInfo PRIVATE ${PROXY_DEF})

target_include_directories(FalcomModernBGMInfo PRIVATE
        ../include
        "../include/imgui"
)

# --- Link All Libraries ---
target_link_libraries(FalcomModernBGMInfo PRIVATE
        yaml-cpp::yaml-cpp
        minhook::minhook
        Microsoft::DirectX-Headers
        Microsoft::DirectXTK

        shlwapi
        d3d11
        winmm
)