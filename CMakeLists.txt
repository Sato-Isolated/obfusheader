cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
project(obfusheader_v2_tests LANGUAGES CXX)

option(OBFUSHEADER_BUILD_TESTS "Build obfusheader tests" ON)

if(MSVC)
    string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

add_library(obfusheader INTERFACE)
target_include_directories(obfusheader INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(obfusheader INTERFACE cxx_std_20)

if(OBFUSHEADER_BUILD_TESTS)
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    function(obfusheader_apply_strict_client_compile target_name)
        if(MSVC)
            target_compile_options(${target_name} PRIVATE /permissive- /Zc:preprocessor)
        endif()
    endfunction()

    function(obfusheader_set_msvc_runtime target_name runtime_library)
        if(MSVC)
            set_property(TARGET ${target_name} PROPERTY MSVC_RUNTIME_LIBRARY "${runtime_library}")
        endif()
    endfunction()

    add_test(
        NAME obfusheader_public_surface
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_public_surface.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_executable(obfusheader_v2_tests tests/v2_tests.cpp)
    target_link_libraries(obfusheader_v2_tests PRIVATE obfusheader)
    set_target_properties(obfusheader_v2_tests PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)
    add_test(NAME obfusheader_v2_runtime COMMAND obfusheader_v2_tests)

    add_executable(obfusheader_v2_tests_cpp23 tests/v2_tests.cpp)
    target_link_libraries(obfusheader_v2_tests_cpp23 PRIVATE obfusheader)
    set_target_properties(obfusheader_v2_tests_cpp23 PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
    add_test(NAME obfusheader_v2_runtime_cpp23 COMMAND obfusheader_v2_tests_cpp23)

    add_executable(obfusheader_strong_preset_tests tests/strong_preset_tests.cpp)
    target_link_libraries(obfusheader_strong_preset_tests PRIVATE obfusheader)
    set_target_properties(obfusheader_strong_preset_tests PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)
    add_test(NAME obfusheader_strong_preset_runtime COMMAND obfusheader_strong_preset_tests)

    add_executable(obfusheader_anti_analysis_tests tests/anti_analysis_tests.cpp)
    target_link_libraries(obfusheader_anti_analysis_tests PRIVATE obfusheader)
    set_target_properties(obfusheader_anti_analysis_tests PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)
    add_test(NAME obfusheader_anti_analysis_runtime COMMAND obfusheader_anti_analysis_tests)

    add_executable(obfusheader_syntax_stability_md tests/syntax_stability_tests.cpp)
    target_link_libraries(obfusheader_syntax_stability_md PRIVATE obfusheader)
    set_target_properties(obfusheader_syntax_stability_md PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)
    obfusheader_apply_strict_client_compile(obfusheader_syntax_stability_md)
    obfusheader_set_msvc_runtime(obfusheader_syntax_stability_md "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    add_test(NAME obfusheader_syntax_stability_md_runtime COMMAND obfusheader_syntax_stability_md)

    add_executable(obfusheader_syntax_stability_mt tests/syntax_stability_tests.cpp)
    target_link_libraries(obfusheader_syntax_stability_mt PRIVATE obfusheader)
    set_target_properties(obfusheader_syntax_stability_mt PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)
    obfusheader_apply_strict_client_compile(obfusheader_syntax_stability_mt)
    obfusheader_set_msvc_runtime(obfusheader_syntax_stability_mt "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_test(NAME obfusheader_syntax_stability_mt_runtime COMMAND obfusheader_syntax_stability_mt)

    add_executable(obfusheader_binary_fixture tests/binary_fixture.cpp)
    target_link_libraries(obfusheader_binary_fixture PRIVATE obfusheader)
    if(UNIX AND NOT APPLE)
        target_link_libraries(obfusheader_binary_fixture PRIVATE dl)
        target_link_libraries(obfusheader_v2_tests PRIVATE dl)
        target_link_libraries(obfusheader_v2_tests_cpp23 PRIVATE dl)
        target_link_libraries(obfusheader_strong_preset_tests PRIVATE dl)
        target_link_libraries(obfusheader_anti_analysis_tests PRIVATE dl)
    endif()
    set_target_properties(obfusheader_binary_fixture PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)
    add_test(
        NAME obfusheader_binary_scan
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/scan_binary.py $<TARGET_FILE:obfusheader_binary_fixture> needle-v2-binary-scan-secret wide-binary-scan-secret u8-binary-scan-secret u16-binary-scan-secret u32-binary-scan-secret str-eq-binary-scan-secret hex:cdab3412 hex:0123456789abcdeffedcba9876543210
    )
    add_test(
        NAME obfusheader_runtime_binary_scan
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/scan_binary.py $<TARGET_FILE:obfusheader_v2_tests> missing-module-for-test missing-symbol-for-test missing-void-symbol-for-test
    )

    if(MSVC)
        add_executable(obfusheader_no_crt_fixture tests/no_crt_fixture.cpp)
        target_link_libraries(obfusheader_no_crt_fixture PRIVATE obfusheader kernel32)
        target_compile_options(obfusheader_no_crt_fixture PRIVATE /permissive- /Zc:preprocessor /GS- /EHsc- /GR-)
        target_link_options(obfusheader_no_crt_fixture PRIVATE /NODEFAULTLIB /ENTRY:entry /SUBSYSTEM:CONSOLE)
        set_target_properties(obfusheader_no_crt_fixture PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED YES
            MSVC_RUNTIME_LIBRARY ""
        )
        add_test(NAME obfusheader_no_crt_runtime COMMAND obfusheader_no_crt_fixture)
    endif()
endif()
