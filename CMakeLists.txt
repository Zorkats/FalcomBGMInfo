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

include(CTest)
if(BUILD_TESTING)
    add_executable(FalcomBgmPathUtilsTests tests/bgm_path_utils_tests.cpp)
    target_compile_features(FalcomBgmPathUtilsTests PRIVATE cxx_std_17)
    target_include_directories(FalcomBgmPathUtilsTests PRIVATE src/include)
    add_test(NAME FalcomBgmPathUtilsTests COMMAND FalcomBgmPathUtilsTests)
    add_executable(
        FalcomOwnerThreadSubclassTests
        tests/owner_thread_subclass_tests.cpp
    )
    target_compile_features(
        FalcomOwnerThreadSubclassTests
        PRIVATE
        cxx_std_17
    )
    target_include_directories(
        FalcomOwnerThreadSubclassTests
        PRIVATE
        src/include
    )
    target_link_libraries(
        FalcomOwnerThreadSubclassTests
        PRIVATE
        comctl32
    )
    add_test(
        NAME FalcomOwnerThreadSubclassTests
        COMMAND FalcomOwnerThreadSubclassTests
    )
    add_test(
        NAME FalcomNoGlobalUser32Hooks
        COMMAND ${CMAKE_COMMAND}
            -DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/verify_no_user32_hooks.cmake
    )
    add_test(
        NAME FalcomRuntimeLifecycleInvariants
        COMMAND ${CMAKE_COMMAND}
            -DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/verify_runtime_lifecycle.cmake
    )
endif()
