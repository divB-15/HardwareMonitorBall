# ============================================================================
#  HardwareMonitorBall — CMake 构建入口
#
#  用法（Windows + MinGW-w64）：
#      cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
#      cmake --build build --parallel
#      ctest --test-dir build --output-on-failure
#  或者直接双击根目录的 build.bat（等价于上面三条）。
#
#  ⚠ 版本号要同步改两处：
#      1) 本文件 project(... VERSION 1.1.0 ...)
#      2) src/app/app.rc 的 FILEVERSION / PRODUCTVERSION / FileVersion / ProductVersion
#     另外 src/app/app.h 的 kAppVersion（窗口标题与托盘提示用）也是同一版本。
#    改完重编，在「属性 → 详细信息」核对。
# ============================================================================
cmake_minimum_required(VERSION 3.20)
project(HardwareMonitorBall VERSION 1.1.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "HardwareMonitorBall 是纯 Win32 程序（PDH / IOCTL / GDI），只支持 Windows。")
endif()

# app.rc 是 UTF-8（版本信息里有中文），必须显式告诉 windres；
# -I 让它能按"相对项目根"的路径找到 assets/HardwareMonitor.ico。
set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -c 65001 -I\"${CMAKE_CURRENT_SOURCE_DIR}\"")

# ---------------------------------------------------------------------------
#  主程序：18 个 .cpp + 1 个资源脚本
#  WIN32 = 链接成 GUI 子系统程序（-mwindows），不弹控制台窗口。
# ---------------------------------------------------------------------------
add_executable(HardwareMonitorBall WIN32
    src/main.cpp

    src/app/app_core.cpp
    src/app/window.cpp

    src/core/metric_map.cpp

    src/theme/theme.cpp

    src/metrics/cpu_temp.cpp
    src/metrics/disk_temp.cpp
    src/metrics/system_load.cpp
    src/metrics/metrics.cpp

    src/ui/gfx.cpp
    src/ui/text.cpp
    src/ui/layout.cpp
    src/ui/paint.cpp

    src/platform/config.cpp
    src/platform/tray.cpp
    src/platform/menu.cpp
    src/platform/tooltip.cpp

    src/util/util.cpp

    src/app/app.rc
)

# 每个模块一个目录 → 一个 -I，源码里照旧写 #include "app.h"
target_include_directories(HardwareMonitorBall PRIVATE
    src/app
    src/core
    src/theme
    src/metrics
    src/ui
    src/platform
    src/util
)

target_compile_definitions(HardwareMonitorBall PRIVATE
    UNICODE _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _WIN32_WINNT=0x0601
    WINVER=0x0601
)

# pdh = CPU 温度计数器；comctl32 = 提示泡（TOOLTIPS_CLASS）；
# shell32 = 托盘图标；advapi32 = 注册表读写。
target_link_libraries(HardwareMonitorBall PRIVATE
    user32 gdi32 pdh comctl32 shell32 advapi32
)

if(MSVC)
    target_compile_options(HardwareMonitorBall PRIVATE /utf-8 /W4 /permissive-)
else()
    # 源码是 UTF-8（大量中文注释与宽字符串字面量），显式声明输入编码
    target_compile_options(HardwareMonitorBall PRIVATE -Wall -Wextra -finput-charset=UTF-8)
    if(MINGW)
        # 静态链接运行库：分发的 exe 不依赖 libgcc/libstdc++ DLL。
        # -municode 不能省：入口是 wWinMain（宽字符版），少了它链接器会去找
        # WinMain 并报 undefined reference。
        target_link_options(HardwareMonitorBall PRIVATE
            -static -static-libgcc -static-libstdc++ -mwindows -municode)
    endif()
endif()

# 成品统一输出到 bin\：根目录保持纯工程（源码 / 构建脚本 / 文档），
# 编译中间文件仍然全在 build\。全工程只有 bin\ 这一份 exe。
set_target_properties(HardwareMonitorBall PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    # 多配置生成器（MSVC）默认还会再套一层 Release\；显式钉死，
    # 免得 bin\ 里又冒出 bin\Release\HardwareMonitorBall.exe。
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    OUTPUT_NAME "HardwareMonitorBall")

# 图标是资源编译器的输入：改了 .ico 必须重跑 windres。
# 用 OBJECT_DEPENDS 把 .ico 挂进依赖图，免得改了图标忘了重编。
set_property(SOURCE src/app/app.rc APPEND PROPERTY
             OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/HardwareMonitor.ico")

# ---------------------------------------------------------------------------
#  测试（CTest）：只覆盖 core/ 这个纯逻辑层（不依赖 Win32 与窗口）
#  跑法： ctest --test-dir build --output-on-failure
# ---------------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
