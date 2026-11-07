# HardwareMonitorBall

**极简硬件监视器** —— 一个常驻桌面的圆形悬浮球，实时显示 CPU 温度 / SSD 温度 / 内存占用 / CPU 占用。

纯 Win32 + GDI，约 2200 行 C++（18 个 `.cpp` + 17 个头文件，按层分在 `src\` 下），**不依赖任何第三方 GUI 库，也不需要管理员权限**。

![HardwareMonitorBall](assets/HardwareMonitor.png)

---

> **当前版本：v1.1**（2026-11-06）—— 工程结构按标准布局重组：`src/` 分层（`app / core / theme / metrics / ui / platform / util`）、
> 成品进 `bin\`、`build/` 只放中间产物、`tests/` 走 CTest；exe 内嵌信息与 `CopyMath` / `Palette` 对齐。

---

## 目录

- [功能特性](#功能特性)
- [第一部分：如何使用](#第一部分如何使用)
  - [1. 运行程序](#1-运行程序)
  - [2. 界面与操作](#2-界面与操作)
  - [3. 四种指标的数据源与精度](#3-四种指标的数据源与精度)
  - [4. 设置保存在哪](#4-设置保存在哪)
- [第二部分：如何从源码编译](#第二部分如何从源码编译)
  - [环境准备](#环境准备)
  - [方式一：双击 build.bat（推荐）](#方式一双击-buildbat推荐)
  - [方式二：手动执行命令](#方式二手动执行命令)
  - [构建系统做了什么](#构建系统做了什么)
  - [单元测试](#单元测试)
- [项目结构](#项目结构)
- [源码架构说明](docs/ARCHITECTURE.md)
- [工作原理](#工作原理)
- [常见问题](#常见问题)
- [已知限制](#已知限制)
- [更新历史](#更新历史)
- [关于 AI 使用](#关于-ai-使用)

---

## 功能特性

- **四种指标一键切换**：CPU 温度 / SSD 温度 / 内存占用 / CPU 占用，单击球或滚轮循环
- **两种形态**：悬浮球（点一下就看）与展开面板（四个指标并排看，点卡片可设为球上默认显示）
- **环形仪表**：球外圈按数值填充，CPU 温度 ≥75 °C 转黄、≥90 °C 转红（SSD 是 55 / 70）
- **可拖拽、可置顶、可隐藏到托盘**；位置与开关状态写注册表，下次启动原样回来
- **跟随系统明暗主题**：读 `AppsUseLightTheme`，深浅两套配色
- **高 DPI 自适应**：Per-Monitor V2，175% 缩放下字不糊、球不扁
- **普通用户权限**：三个数据源都不需要驱动、不需要提权（HWiNFO 那类工具要管理员）
- **单一 exe**：不需要安装，`bin\HardwareMonitorBall.exe` 拷走就能用

---

# 第一部分：如何使用

## 1. 运行程序

直接双击 `bin\HardwareMonitorBall.exe` 即可，无需安装、无需管理员权限。

程序启动后**默认停靠在屏幕右侧**（工作区右上角往下 180 px），第一次运行会从托盘弹一条气泡提示告诉你操作方式。

- **单实例**：重复双击不会开出第二个球，而是把已经在跑的那个唤回前台（靠命名 Mutex `Local\HWMonBallSingleton`）。
- **不进任务栏、不进 Alt-Tab**（`WS_EX_TOOLWINDOW`），只在托盘留一个图标。
- **球外区域点击穿透**：窗口用 `SetWindowRgn` 裁成圆形，点在圆外会直接落到下面的窗口上。

> `bin\` 里只有 exe 一个文件。程序**不读写任何配置文件**，所有状态都在注册表当前用户下（见第 4 节），所以整个 `bin\` 拷到哪都能跑。

## 2. 界面与操作

```
        ╭─────────╮
        │  47 °C  │        ← 中央：数值 + 单位
        │   CPU   │        ← 下方：指标小字
        ╰─────────╯
        外圈：按数值填充的环形仪表（隐式 25~105 °C 映射）
```

| 操作 | 效果 |
|---|---|
| 左键单击球 | 循环切换指标 |
| 左键双击球 | 展开面板 |
| 左键双击面板标题栏 | 收起为球 |
| 滚轮（球上） | 切换指标（上/下 = 前/后） |
| 右键 | 上下文菜单：切换指标 / 展开收起 / 置顶 / 隐藏 / 退出 |
| 拖拽 | 移动窗口（位移超过 4 px 才算拖拽，手抖不会误移动） |
| `Esc` | 面板形态下收起；球形态下隐藏到托盘 |
| 托盘图标 | 双击显隐；右键菜单 |

**展开面板**是一张 258 × 291（逻辑像素）的圆角卡片，2 × 2 四张指标卡片：

- 卡片右上角的小圆点 = **当前球上默认显示的指标**；点任意卡片可以改；
- 内存卡片的副文本是「已用 / 总量 GB」，CPU 占用卡片是「N 逻辑核心」；
- 每张卡片底部有一条进度条，比例口径与环形仪表一致。

## 3. 四种指标的数据源与精度

| 指标 | 数据源 | 说明 |
|---|---|---|
| CPU 温度 | PDH 计数器 `\Thermal Zone Information(*)\Temperature` | ACPI 热区，单位开尔文。**整数分辨率（1 °C）** |
| SSD 温度 | `IOCTL_STORAGE_QUERY_PROPERTY` / `StorageDeviceTemperatureProperty` | 多传感器取最高值；以 `dwDesiredAccess = 0` 打开设备，普通权限可读 |
| CPU 占用 | `GetSystemTimes` 差分 | kernel 时间已含 idle，所以 `total = kernel + user` |
| 内存 | `GlobalMemoryStatusEx` | 已用 = Total − Available |

**CPU 温度准不准？** 做过与 HWiNFO64 v8.54 的实测对比（同一时刻：读 PDH → 截 HWiNFO → 截球，
负载用 8 进程跑满 40 秒，共 6 个工况点）：

| 工况 | 本程序数据源 ACPI (°C) | 球面显示 | HWiNFO CPU (Tctl/Tdie) | 偏差 |
|---|---|---|---|---|
| 空闲 | 47.52 | 47 | 48.6 | −1.1 |
| 满载 +8 s | 73.18 | 73 | 73.4 | −0.2 |
| 满载 +18 s | 77.85 | 78 | 78.5 | −0.7 |
| 满载 +28 s | 81.18 | 81 | 81.6 | −0.4 |
| 冷却 +20 s | 54.85 | 55 | 55.9 | −1.0 |
| 冷却 +40 s | 50.85 | 51 | 51.4 | −0.5 |

偏差 **−1.1 ~ −0.2 °C**，满载到 81 °C 时依然对得上，没有"负载越高偏得越多"的失真。
SSD 温度与 HWiNFO 的「磁盘温度」**完全一致**（同机 35 °C）。

偏差来源是三件事叠加，都不是算法错误：

1. **数据源本身低一点**：ACPI 热区报的温度比 CPU 内部 DTS 略低；
2. **1 °C 量化**：ACPI 计数器是整数开尔文，连续采样实测会跳 ±1~2 K（EC 固件的量化行为）；
3. **显示取整**：球面只显示整数，`46.85 °C` 显示成 `47`。

> `High Precision Temperature` 并不是更高精度：它恒等于 `Temperature + 0.2 K`，步长同样是 1 K。
> 换它不会让分辨率变好，只能消掉 `-273.15` 留下的 `.85` 尾巴。

## 4. 设置保存在哪

全部在 **`HKEY_CURRENT_USER\Software\HWMonBall`**，不碰 `HKLM`，所以不需要管理员权限：

| 值名 | 类型 | 含义 |
|---|---|---|
| `x` / `y` | DWORD | 悬浮球窗口左上角（屏幕坐标） |
| `px` / `py` | DWORD | 展开面板左上角 |
| `pvalid` | DWORD | 面板位置是否已被真正定位过（防止 `(0,0)` 冒充有效位置） |
| `metric` | DWORD | 球上默认显示的指标（0 = CPU 温度 … 3 = CPU 占用） |
| `topmost` | DWORD | 是否置顶 |
| `hidden` | DWORD | 是否隐藏到托盘 |
| `tipShown` | DWORD | 首次运行的气泡提示是否弹过 |

想恢复初始状态（位置回到右上角、提示泡重新弹一次）：

```bat
reg delete "HKCU\Software\HWMonBall" /f
```

---

# 第二部分：如何从源码编译

工程是标准 C++ 布局：根目录放 `CMakeLists.txt` + `src/` + `tests/` + `assets/` + `docs/`，
编译中间文件全在 `build/`，成品输出到 `bin\`。

## 环境准备

| 需要 | 用途 | 说明 |
|---|---|---|
| **MinGW-w64** | `g++.exe` 编译、`windres.exe` 嵌图标 | 确认 MinGW-w64 安装目录的 `bin\` 下有 `g++.exe`、`windres.exe`、`mingw32-make.exe`（在 `PATH` 上即可） |
| **CMake ≥ 3.20** | 构建 | 装官方安装包即可；本机若没有，可在 `.build.local.bat` 设 `CMAKE` 指向任意一份，`build.bat` 会自动找 |
| 无其它依赖 | — | PDH / IOCTL / GDI 都是系统 API，不需要 SDK 额外组件 |

## 方式一：双击 build.bat（推荐）

根目录的 `build.bat` 是个薄封装：找 cmake → 挑工具链 → 配置 → 编译 → 跑测试。脚本内部 `cd /d "%~dp0"`，从哪儿双击都行。

工具链的选择规则是：**PATH 里有 MSVC（`cl.exe`）就用 MSVC，否则用 MinGW-w64（在 `PATH` 上，或在 `.build.local.bat` 指定）**。

```
[INFO] Using MinGW toolchain from <你的 MinGW bin 目录>.
[1/3] configure ...
[2/3] build ...
[3/3] test ...
1/1 Test #1: metric_map .......................   Passed    0.36 sec
100% tests passed out of 1
Build OK: bin\HardwareMonitorBall.exe
```

> `build.bat` 刻意只用 **ASCII + CRLF**：`.bat` 里写中文在 GBK 控制台下会乱码，
> 纯 LF 换行则会让 `^` 续行解析出错（这两条 `.editorconfig` 里也标了）。
>
> `ctest` 不写死名字而是从 cmake 所在目录拼路径——CMake 走 venv 那份时，`ctest` 并不在 `PATH` 上。

## 方式二：手动执行命令

在项目根目录（`CMakeLists.txt` 那一层）执行：

```bat
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

> 装了 Ninja 的话也可以 `-G Ninja`；Git Bash 里同样的命令照用。

## 构建系统做了什么

| 配置 | 作用 |
|---|---|
| `project(... VERSION 1.1.0 LANGUAGES CXX RC)` | 同时启用 C++ 与 RC（资源）编译 |
| `add_executable(... WIN32 ...)` | `WIN32` = GUI 子系统（`-mwindows`），运行不弹黑框 |
| `target_include_directories(...)` | 每个模块目录一个 `-I`，源码里照旧写 `#include "app.h"` |
| `CMAKE_RC_FLAGS = -c 65001 -I<项目根>` | `app.rc` 是 UTF-8（版本信息有中文），且要能按相对路径找到 `assets/HardwareMonitor.ico` |
| `target_compile_definitions(... UNICODE NOMINMAX WIN32_LEAN_AND_MEAN ...)` | 宽字符 API、关掉 `<windows.h>` 的 min/max 宏 |
| `-static -static-libgcc -static-libstdc++ -mwindows -municode` | 静态链运行库；入口是 `wWinMain`，**`-municode` 不能省**，少了它链接器会去找 `WinMain` 并报 undefined reference |
| `RUNTIME_OUTPUT_DIRECTORY = bin` | 成品落在 `bin\`，中间文件全在 `build/` |
| `OBJECT_DEPENDS app.rc → assets/HardwareMonitor.ico` | 改了图标必须重跑 `windres`，挂进依赖图免得忘了 |
| `enable_testing()` + `tests/` | CTest 跑 `test_metric_map` |

## 单元测试

`src/core/` 这一层是纯 C++（不 `#include <windows.h>`），所以能直接编成控制台程序跑，不开窗口、秒级结束：

```bat
ctest --test-dir build --output-on-failure
:: 或者直接跑，看逐条结果
build\tests\test_metric_map.exe
```

`tests/test_metric_map.cpp` 现在 **32 条断言**，覆盖：

| 组 | 内容 |
|---|---|
| 温度比例 | 25 °C → 0、65 °C → 0.5、105 °C → 1；低于 25 钳到 0、高于 105 钳到 1；`-1`（没采到）返回 −1 |
| 百分比比例 | 0 / 50 / 100 的线性映射，>100 钳到 1 |
| 档位边界 | CPU 温度 74/75/89/90、SSD 55/70、内存 60/61/85/86 —— 逐点核对"正常 / 警告 / 危险" |
| 四舍五入 | 74.4 → 74、74.5 / 74.6 → 75（与球面显示同口径） |
| 数值文本 | 负值 → `--`、`47.5` → `48`、`100` → `100` |
| 联动 | `74.6` 显示成 `75`，配色就按 75 判成"警告" —— 数字与颜色必须同口径 |

没有引第三方测试框架（一个 `CHECK` 宏 + 计数），以后想换 Catch2 / GoogleTest，只改 `tests/` 即可，顶层 CMake 不用动。

---

## 项目结构

```text
HardwareMonitorBall_v1.1/
├── CMakeLists.txt             构建入口（版本号、源文件清单、输出到 bin\）
├── build.bat                  Windows 一键编译：找 cmake → 挑工具链 → 配置 → 编译 → 测试
├── README.md
├── .gitignore                 build/、*.o、*.exe
├── .clang-format              代码风格（4 空格 / 100 列 / 大括号不换行）
├── .editorconfig              缩进与换行统一（.bat 强制 CRLF）
│
├── bin\
│   └── HardwareMonitorBall.exe    成品（双击运行，整个 bin\ 拷走即用）
│
├── assets/                    构建输入
│   ├── HardwareMonitor.ico    程序图标（嵌进 exe）
│   └── HardwareMonitor.png    README 预览图
│
├── src/
│   ├── main.cpp               入口：单实例 / 主题 / DPI / 注册窗口类 / 消息循环
│   │
│   ├── app/                   应用层：全局状态、窗口过程、交互
│   │   ├── app.h                  常量、指标枚举、全局状态的 extern 声明
│   │   ├── app_core.cpp           全局状态定义 + AppInit / AppShutdown
│   │   ├── window.cpp             WndProc、拖拽、命令分发、形态切换
│   │   ├── app.rc                 资源：图标 + 清单 + 版本信息
│   │   ├── resource.h             资源 ID
│   │   └── app.manifest           Per-Monitor V2、asInvoker
│   │
│   ├── core/                  纯逻辑层（零 Win32，可被单测直接链接）
│   │   └── metric_map.h / .cpp    比例映射 / 档位判定 / 数值格式化
│   │
│   ├── theme/                 调色板与 DPI
│   │   └── theme.h / .cpp         深浅两套配色、g_scale、PX()、Mix()
│   │
│   ├── metrics/               数据采集
│   │   ├── cpu_temp.h / .cpp      PDH 热区计数器
│   │   ├── disk_temp.h / .cpp     IOCTL 磁盘温度
│   │   ├── system_load.h / .cpp   GetSystemTimes / GlobalMemoryStatusEx
│   │   └── metrics.h / .cpp       采样调度 + 指标呈现（值 / 文本 / 比例 / 配色）
│   │
│   ├── ui/                    绘制层
│   │   ├── gfx.h / .cpp           圆角、圆、圆环、圆弧
│   │   ├── text.h / .cpp          9 个字体句柄 + 四种文本排版
│   │   ├── layout.h / .cpp        尺寸换算、卡片/按钮矩形、工作区收敛、异形区域
│   │   └── paint.h / .cpp         双缓冲 + 悬浮球 + 面板
│   │
│   ├── platform/              Win32 平台层
│   │   ├── config.h / .cpp        注册表读写
│   │   ├── tray.h / .cpp          托盘图标与托盘菜单
│   │   ├── menu.h / .cpp          右键菜单
│   │   └── tooltip.h / .cpp       悬停提示泡
│   │
│   └── util/
│       └── util.h / .cpp          Fmt / AnsiToWide / TrimW / CopyTip
│
├── tests/
│   ├── CMakeLists.txt
│   └── test_metric_map.cpp    32 条断言（CTest）
│
├── docs/
│   └── ARCHITECTURE.md        分层图、依赖方向、关键坑位、温度实测记录
│
└── build/                     **CMake 生成**：编译中间文件（不入库，可随时删）
```

| 位置 | 作用 |
|---|---|
| `CMakeLists.txt` | 顶层构建脚本：源文件清单、`-I` 路径、链接库、测试入口 |
| `build.bat` | 一键编译 + 跑测试（等价于手动那三条命令） |
| `bin\` | 成品：`HardwareMonitorBall.exe` |
| `assets/` | 只放**构建输入**：图标与 README 图 |
| `src/app/` | 入口之外的"应用层"：全局状态、窗口过程、交互 |
| `src/core/` | 纯逻辑，零 Win32，是唯一有单测的一层 |
| `src/metrics/` | 四个数据源 + 采样调度，只产数字不认识颜色 |
| `src/ui/` | 绘制原语、字体、布局、球与面板 |
| `src/platform/` | 注册表、托盘、菜单、提示泡 |
| `tests/` | CTest 单元测试（只链 `core/`） |
| `docs/ARCHITECTURE.md` | 改代码前先看这份 |
| `build/` | CMake 生成物，`.gitignore` 已忽略 |

依赖方向只有一条：**`app` → `ui` / `platform` / `metrics` / `theme` → `core` / `util`**，
`core` 不 include `<windows.h>`；全局变量**只在 `app/app_core.cpp` 定义一次**。
细节见 [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)。

## 工作原理

```
   每 1 秒（WM_TIMER）
        │
        ├─► CpuTempRead()      PDH \Thermal Zone Information(*)\Temperature  → K − 273.15
        ├─► CpuUsageRead()     GetSystemTimes 差分
        └─► RamRead()          GlobalMemoryStatusEx
        │
        │   每 5 个周期（IOCTL 慢）
        └─► ReadDiskTemperature()   IOCTL StorageDeviceTemperatureProperty
        │
        ▼
    g_m（Metrics 结构：四个原始值）
        │
        ▼
   MetricValue / MetricText / MetricRatio / MetricAccent
        │         │              │                │
        │         │              │                └─► core::level_by_threshold → 青 / 黄 / 红
        │         │              └─► core::temp_ratio / percent_ratio → 环形仪表填充比例
        │         └─► core::format_value → "--" 或整数文本
        ▼
    PaintBall / PaintPanel（内存 DC）
        │
        ▼
    BitBlt 到窗口（双缓冲，无闪烁）
```

**为什么球和卡片都用同一套 `Metric*` 函数**：数值、文本、比例、配色只算一次，
球面显示 `47` 时环的填充和卡片上的颜色必然一致，不会出现"数字变了颜色没变"。

**形态切换**只是换窗口尺寸 + 换异形区域（椭圆 ↔ 圆角矩形）+ 换绘制函数，
由 `g_panel` 一个布尔量决定，不销毁重建窗口。

## 常见问题

**Q：球不见了怎么办？**
两种可能：① 被隐藏到托盘了 —— 双击托盘图标；② 已经退出 —— 重新双击 `bin\HardwareMonitorBall.exe`。
如果窗口被拖到屏幕外，程序在拖拽结束、启动读配置、DPI 变化时都会把位置收敛回"最近显示器"的工作区，一般不会丢。

**Q：显示 `--` 是什么意思？**
该指标这次没采到。CPU 温度在极少数机器上没有 ACPI 热区计数器；SSD 温度需要盘支持
`StorageDeviceTemperatureProperty`（部分 SATA / USB 硬盘盒不支持）。程序会在下一个采样周期重试。

**Q：温度偶尔跳 1~2 度，是读数不稳吗？**
是**数据源**的特性。ACPI 热区计数器实测在相邻采样间会跳 ±1~2 K（整数开尔文 + EC 固件量化），
程序只是如实转述。想要更平滑可以在 `metrics.cpp` 里做三点中位数，但响应会变慢。

**Q：为什么我的 CPU 温度比 HWiNFO 低十几度？**
本程序读的是 **ACPI 热区**，HWiNFO 的 `CPU (Tctl/Tdie)` 读的是 CPU 内部 DTS。
两者的关系取决于主板 EC 固件：本机（HONOR / Ryzen 7 8845HS）实测只差约 1 °C，
但不少机器上 ACPI 热区会明显低于核心温度。要拿核心温度必须用内核驱动（需要管理员），本程序刻意不这么做。

**Q：为什么不需要管理员权限？**
三个数据源都对普通用户开放：PDH 热区计数器、`GetSystemTimes`、`GlobalMemoryStatusEx` 不用说；
磁盘温度用 `dwDesiredAccess = 0` 打开 `\\.\PhysicalDriveN` 也能发 IOCTL（`GENERIC_READ` 反而会被拒绝）。

**Q：换一台电脑，之前的设置还在吗？**
不在。状态存在 `HKCU\Software\HWMonBall`，属于当前用户账户；换机器只有 `bin\` 里的 exe 跟着走。
新机器上第一次运行会落在默认位置（屏幕右侧、工作区顶部往下 180 px）。

**Q：为什么第一次启动会弹托盘气泡，以后不弹了？**
`tipShown` 记在注册表里。想再看一次就删掉这个值或整个键（见第 4 节的 `reg delete`）。

---

## 已知限制

- **CPU 温度是 ACPI 热区，不是 CPU 内部 DTS**：分辨率 1 °C，且完全依赖主板 EC 固件把 CPU 温度映射到热区。
- **SSD 温度需要盘支持** `StorageDeviceTemperatureProperty`，不支持的盘显示 `--`。
- **一次只看一块盘**：扫描 `PhysicalDrive0..5`，取第一个能上报温度的那块；多盘机器不会循环显示。
- **托盘气泡提示只在第一次启动弹**（`tipShown`）。
- **不做历史曲线**：只有当前值和环形比例，没有折线图与峰值记录。

---

## 更新历史

### v1.1 — 2026-11-06

**重构（目录结构）**

- 1486 行的单文件 `main.cpp` 拆成 **18 个 `.cpp` + 17 个头文件**，按层分目录：
  `app`（全局状态 / 窗口过程 / 交互）、`core`（纯逻辑，零 Win32）、`theme`（配色与 DPI）、
  `metrics`（四个数据源 + 采样调度）、`ui`（绘制原语 / 字体 / 布局 / 球与面板）、
  `platform`（注册表 / 托盘 / 菜单 / 提示泡）、`util`（格式化与编码工具）。
- 依赖方向固定为 **`app` → `ui` / `platform` / `metrics` / `theme` → `core` / `util`**，
  全局变量只在 `app/app_core.cpp` 定义一次，`app.h` 只做 extern 声明。
- 抽出 `core/metric_map`：比例映射、档位判定、四舍五入、数值格式化集中到一处，
  并且**不再 `#include <windows.h>`**，所以能被单测直接链接。

**工程化（对齐标准 C++ 布局）**

- 构建系统换成 **CMake**：`CMakeLists.txt` 是唯一构建入口，`build.bat` 退化成几行封装；
  链接选项（含 `-municode`）、`-I` 路径、RC 编码（`-c 65001`）都写进 CMake。
- 新增 **`tests/`**（CTest + 32 条断言）与 **`docs/ARCHITECTURE.md`**（分层图、依赖方向、坑位清单、温度实测）。
- 补齐 **`.gitignore`** / **`.clang-format`** / **`.editorconfig`**；图标与预览图统一进 `assets/`。
- **成品收进 `bin\`**：`RUNTIME_OUTPUT_DIRECTORY` 指向 `bin`，根目录不再直接躺 exe。

**改进**

- **SSD 掉盘恢复更快**：缓存盘读失败时**当轮**就退回全盘扫描（原实现要等下一轮，中间有一轮显示 `--`）。
- **改指标统一走 `SetMetric()`**：原实现双击展开时直接改 `g_metric`，导致回到球形态后提示泡文本仍是旧的；
  现在提示泡、持久化、重绘一次做齐。

**其它**

- 版本号 → **1.1.0.0**（`app.rc` 的 `FILEVERSION` 与 `app.h` 的 `kAppVersion` 同步）；文件夹名按 2 位版本约定为 `HardwareMonitorBall_v1.1`。
- exe 内嵌信息与 `CopyMath` / `Palette` 对齐：`CompanyName` / `ProductName` = `HardwareMonitorBall`，
  `FileDescription` = 「极简硬件监视器」。

### v1.0 — 初始版本

- 圆形悬浮球 + 展开面板两种形态，四种指标（CPU 温度 / SSD 温度 / 内存 / CPU 占用）。
- 环形仪表、按数值分档配色、明暗主题跟随、Per-Monitor V2 高 DPI。
- 拖拽、置顶、隐藏到托盘、右键菜单、单实例。
- 位置与开关状态写注册表 `HKCU\Software\HWMonBall`。
- 手写 `build.bat`（`windres` + 单条 `g++` 命令）。

## 关于 AI 使用

> 本项目由 AI 在人类作者实质性指导下生成，按 MIT 许可证授权。
>
> AI 生成的代码可能存在人类未察觉的疏漏，请自行评估 AI 生成代码的质量与安全性，本项目不提供任何形式的担保。
