# ============================================================================
#  tests — 单元测试（CTest 驱动）
#
#  这里只测 core/ 这一层：比例映射、档位判定、数值格式化，全是纯函数，
#  不碰 Win32、不碰全局状态、不需要窗口，所以能直接链一个控制台程序跑。
#  以后要加 Catch2 / GoogleTest，往 tests/ 里放即可，顶层 CMakeLists 不用改。
# ============================================================================
add_executable(test_metric_map
    test_metric_map.cpp
    ${CMAKE_SOURCE_DIR}/src/core/metric_map.cpp
)

target_include_directories(test_metric_map PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
target_compile_features(test_metric_map PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(test_metric_map PRIVATE /utf-8 /W4)
else()
    target_compile_options(test_metric_map PRIVATE -Wall -Wextra -finput-charset=UTF-8)
    if(MINGW)
        target_link_options(test_metric_map PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endif()

add_test(NAME metric_map COMMAND test_metric_map)
