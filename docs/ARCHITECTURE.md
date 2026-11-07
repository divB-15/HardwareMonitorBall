# 架构说明

HardwareMonitorBall 是一个常驻桌面的硬件监视悬浮球。规模不大（约 2200 行 C++），
但把它拆成单向依赖的几层，好处是：改绘制不用碰采集，改采集不用碰窗口过程。

---

## 一、分层与依赖方向

```
        ┌──────────────────────────────────────────────┐
        │  src/main.cpp     入口：单实例 / 主题 / 注册窗口 │
        └───────────────────┬──────────────────────────┘
                            │
        ┌───────────────────▼──────────────────────────┐
        │  app/    全局状态 · 窗口过程 · 交互 · 生命周期  │
        └───┬───────────┬───────────┬──────────┬───────┘
            │           │           │          │
   ┌────────▼──┐ ┌──────▼─────┐ ┌───▼──────┐ ┌─▼─────────┐
   │  ui/      │ │ platform/  │ │ metrics/ │ │ theme/    │
   │ 绘制      │ │ 配置/托盘  │ │ 采集     │ │ 调色板/DPI│
   └────┬──────┘ └─────┬──────┘ └────┬─────┘ └────┬──────┘
        │              │             │            │
        └──────────────┴──────┬──────┴────────────┘
                              │
                    ┌─────────▼─────────┐
                    │  core/  纯逻辑     │  ← 零 Win32，可被单测直接链接
                    │  util/  小工具     │
                    └───────────────────┘
```

**规则：箭头只能向下。** 下层不知道上层的存在：

- `core/` 不 include `<windows.h>`，只有数值映射与格式化——所以 `tests/` 能单独链它；
- `metrics/` 只产出数字，不认识颜色和矩形；
- `ui/` 只消费数字与主题色，不自己采集；
- `platform/` 封装 Win32 资源（注册表、托盘、菜单、提示泡），不知道业务含义；
- `app/` 是唯一可以"什么都知道"的一层，也是唯一持有全局状态的一层；
- 全局变量**只在 `app/app_core.cpp` 定义一次**，`app.h` 提供 extern 声明。

---

## 二、目录职责

### `src/app/`

| 文件 | 职责 |
|---|---|
| `app.h` | 应用级常量（`kAppTitle`、尺寸、时序）、指标枚举、`MetricDef`、菜单 ID、**所有全局状态的 extern 声明**、对外函数声明 |
| `app_core.cpp` | 全局状态的**唯一定义处**；`AppInit()` 建字体/开数据源/装托盘/进悬浮球形态，`AppShutdown()` 逆序收尾 |
| `window.cpp` | `WndProc`、鼠标键盘交互、拖拽、`ApplyMode()` 形态切换、`OnCommand()` 命令分发 |
| `app.rc` | 图标 + 应用清单 + 版本信息 |
| `resource.h` | 资源 ID（只有一个图标 ID） |
| `app.manifest` | Per-Monitor V2 DPI 感知、`asInvoker`（不需要管理员）、Common-Controls v6 |

### `src/core/` — 纯逻辑

`metric_map.{h,cpp}`：比例映射（温度 25~105 °C 归一化）、档位判定、四舍五入、数值文本。
**这是全工程唯一有单元测试的地方**，也是"屏幕上的数字和颜色必须同口径"这条规则的落点：
`74.6 °C` 显示成 `75`，配色就必须按 75 判定，否则数字变黄了环还是青的。

### `src/theme/`

调色板（深/浅两套）、`g_scale`（DPI 系数）、`PX()` 换算、`Mix()` 插值、
`LoadSystemPalette()` 读系统主题。**所有布局常量都写成 96 dpi 下的逻辑像素**，
进 GDI 之前必须过 `PX()`——除本文件外任何地方都不该自己乘 dpi。

### `src/metrics/`

| 文件 | 数据源 | 权限 |
|---|---|---|
| `cpu_temp.cpp` | PDH `\Thermal Zone Information(*)\Temperature` | 普通用户 |
| `disk_temp.cpp` | `IOCTL_STORAGE_QUERY_PROPERTY`（属性 52 / 51） | 普通用户（`dwDesiredAccess = 0`） |
| `system_load.cpp` | `GetSystemTimes` 差分 / `GlobalMemoryStatusEx` | 普通用户 |
| `metrics.cpp` | 采样调度 + 指标呈现（值 / 文本 / 比例 / 强调色） | — |

`metrics.cpp` 是"数据 → 界面"的翻译层：`ui/` 拿到的已经是能直接画的东西。

### `src/ui/`

| 文件 | 职责 |
|---|---|
| `gfx.cpp` | 图形基元：圆角矩形、圆、圆环、圆弧（全部临时创建 GDI 对象、用完即删） |
| `text.cpp` | 9 个字体句柄的创建/销毁，以及左对齐/居中/矩形居中/数值+单位四种排版 |
| `layout.cpp` | 尺寸换算、卡片与按钮矩形、工作区收敛、面板初始位置、异形区域 |
| `paint.cpp` | 双缓冲 + 悬浮球绘制 + 面板绘制 |

### `src/platform/`

`config.cpp`（注册表读写）、`tray.cpp`（托盘图标与托盘菜单）、`menu.cpp`（右键菜单）、
`tooltip.cpp`（悬停提示泡）。

### `src/util/`

`Fmt()`（宽字符格式化）、`AnsiToWide()`、`TrimW()`、`CopyTip()`（定长缓冲区安全拷贝）。

---

## 三、关键设计决策

### 1. 异形窗口 + 球外点击穿透

`WS_POPUP` 无边框 + `SetWindowRgn(椭圆)`。区域外的鼠标事件直接落到下层窗口，
不需要自己算"点到球里没有"。`SetWindowRgn` 成功后**区域归系统所有**，不能再 `DeleteObject`。

### 2. 双缓冲

球/面板都画到内存 DC，最后一次性 `BitBlt`。异形窗口如果直接画到窗口 DC，拖动时必然闪。
`WM_ERASEBKGND` 直接 `return 1`（背景全部自己画）。

### 3. 拖拽必须用屏幕坐标

`WM_MOUSEMOVE` 里窗口跟着光标移动会**改变客户区坐标**，用客户区坐标差算位移会形成正反馈，
窗口越拖越快飞走。正确做法：记下按下时的「屏幕光标位置 + 窗口矩形」，
之后每次都用屏幕坐标差推算新位置。

### 4. 面板位置要带 pvalid 标志

注册表里存了 `px/py`，但进程如果从未展开过面板，存下的是 `(0,0)`——
没有 `pvalid` 标志就会在下次启动时冒充有效位置，把面板钉在屏幕左上角。

### 5. 交互全部走客户区消息

没有子控件、没有自绘按钮的命中测试脚本；按钮就是几个 `RECT`，
`WM_MOUSEMOVE` 里比对矩形决定要不要重绘悬停态，`WM_LBUTTONUP` 里比对矩形决定点了什么。

### 6. 单实例靠命名 Mutex

`Local\HWMonBallSingleton`：第二次启动不会开新窗口，而是给已运行的实例发 `IDM_SHOW` 把它唤回。

---

## 四、坑位清单（改代码前先看）

### CPU 温度

- **计数器单位是开尔文**，要减 273.15。
- **中文系统上本地化路径找不到**（`\热能区域(*)\温度`）：必须用 `PdhAddEnglishCounterW`
  （动态从 `pdh.dll` 取，见 `cpu_temp.cpp`），失败再回退 `PdhAddCounterW`。
- **`PDH_STATUS` 是 signed long，`PDH_MORE_DATA` 是无符号常量**，比较前必须 `(DWORD)` 强转。
- **`CStatus == 0` 是 VALID_DATA，不是"没数据"**。
- **`GetProcAddress` 转函数指针要绕一层 `void*`**，直接强转会触发 `-Wcast-function-type`。
- **分辨率只有 1 °C**：`Temperature` 是整数开尔文，实测相邻采样会跳 ±1~2 K。
  `High Precision Temperature` **不是更高精度**（恒等于 `Temperature + 0.2 K`）。

### 磁盘温度

- **`GENERIC_READ` 打开 `\\.\PhysicalDriveN` 会被拒绝**（错误 5）：`dwDesiredAccess` 传 0。
- **`STORAGE_DEVICE_DESCRIPTOR` 的偏移字段可能为 0**，表示该字段不存在，不能照着读。
- **`TempDescRaw` 的 header 是 24 字节**，之后每项 16 字节；`InfoCount` 可能大于实际长度，
  要用 `(返回长度 - 24) / 16` 夹一下。
- MinGW 的 `winioctl.h` 未必定义 `StorageDeviceTemperatureProperty`(52) / `StorageAdapterTemperatureProperty`(51)。

### 绘制

- **`PrintWindow` 对本程序的窗口可用**（自绘、无加速），但截图脚本必须 Per-Monitor V2，
  否则 `GetWindowRect` 拿到的是虚拟化尺寸，看着像布局 bug。
- **字体按物理像素创建**：`CreateFontW(-PX(px), ...)`，负号表示字符高度而非单元格高度。
- **`DrawTextW` 的 `lpRect` 不是 const**，传 RECT 的副本。

### 注册表

- 只写 `HKCU\Software\HWMonBall`，不碰 `HKLM`，所以不需要管理员权限。
- 读用 `RegGetValueW` + `RRF_RT_REG_DWORD`（比 `RegQueryValueEx` 省一次类型判断）。
- **坐标是 `DWORD`**，负数会变成很大正数——本程序所有位置都收敛在工作区内，不会出现负值。

---

## 五、实测记录：温度示数准确性

2026-11-05 用 HWiNFO64 v8.54 做对照（同一时刻：读 PDH → 截 HWiNFO → 截球），
负载用 8 进程跑满 40 秒：

| 工况 | 本程序数据源 ACPI (°C) | 球面显示 | HWiNFO CPU (Tctl/Tdie) | 偏差 |
|---|---|---|---|---|
| 空闲 | 47.52 | 47 | 48.6 | −1.1 |
| 满载 +8 s | 73.18 | 73 | 73.4 | −0.2 |
| 满载 +18 s | 77.85 | 78 | 78.5 | −0.7 |
| 满载 +28 s | 81.18 | 81 | 81.6 | −0.4 |
| 冷却 +20 s | 54.85 | 55 | 55.9 | −1.0 |
| 冷却 +40 s | 50.85 | 51 | 51.4 | −0.5 |

SSD：本程序走 IOCTL 读到 35 °C，HWiNFO 报「磁盘温度 35 °C / 磁盘温度 2 35 °C」，一致。

结论：**示数可信，偏差 ≤1.1 °C**，来源是数据源本身低 + 1 °C 量化 + 显示取整，
不是算法错误。`High Precision Temperature` 实测恒为 `Temperature + 0.2 K`，
换它不会提高分辨率（但能消掉 `-273.15` 留下的 `.85` 尾巴）。
