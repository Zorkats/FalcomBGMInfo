cmake_minimum_required(VERSION 3.20)
project(FalcomModernBGMInfo)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()


# --- vcpkg Package Finding ---
find_package(yaml-cpp CONFIG REQUIRED)
find_package(minhook CONFIG REQUIRED)
find_package(directx-headers CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)


# --- Define Your DLL Target ---
add_library(FalcomModernBGMInfo SHARED
        main_dx11.cpp
        universal_proxy.def
        universal_proxy.h
        # --- ImGui Source Files ---
        # We must compile ALL ImGui source files, not just the backends
        # This will resolve all LNK2019 errors.
        "../include/imgui/imgui.cpp"
        "../include/imgui/imgui_draw.cpp"
        "../include/imgui/imgui_tables.cpp"
        "../include/imgui/imgui_widgets.cpp"

        # The backends you already had
        "../include/imgui/backends/imgui_impl_dx11.cpp"
        "../include/imgui/backends/imgui_impl_win32.cpp"
)
target_include_directories(FalcomModernBGMInfo PRIVATE
        ../include
        "../include/imgui"
)
# --- Link All Libraries ---
target_link_libraries(FalcomModernBGMInfo PRIVATE
        # vcpkg-managed libraries
        yaml-cpp::yaml-cpp
        minhook::minhook
        Microsoft::DirectX-Headers
        Microsoft::DirectXTK

        # Windows system libraries
        shlwapi
        d3d11
)