# Bedrock BoatHUD

[![English](https://img.shields.io/badge/README-English-blue)](README.md)
[![Release](https://img.shields.io/github/v/release/MRUIAW/BedrockBoatHUD)](https://github.com/MRUIAW/BedrockBoatHUD/releases)

基于 [LeviLamina](https://lamina.levimc.org/zh/) 的 Minecraft 基岩版客户端赛船 HUD，参考了 [Hibiii/BoatHud](https://github.com/Hibiii/BoatHud) 和 [BoatHUD Extended](https://github.com/jewtvet/boathud_extended) 的设计与驾驶体验。

## 功能

- 自动识别普通船、运输船的驾驶者；默认使用 **Race（竞速）** 布局，也可切换 **Classic（经典）**、**Compact（紧凑）**。
- 显示速度、侧滑角、纵向/横向加速度、服务器延迟和 HUD 渲染 FPS。
- BoatHUD 风格的像素速度条、加速度指示器、转向/油门轨迹，以及可选的左右箭头和前进/后退图标。
- 游戏内设置界面使用原生鼠标光标；设置即时生效，自动跟随游戏语言显示英文或简体中文。
- 可选 Hibiii 式镜头辅助：速度越高，视角越偏向实际运动方向，并进行平滑处理。
- 可选原版 HUD 隐藏、遥测 CSV、检查点时间差和速度差。

镜头辅助、隐藏原版 HUD、遥测和检查点均**默认关闭**；速度平滑默认开启。无需安装额外资源包。

## 安装

1.0.1 适用于 **Minecraft 基岩版 1.26.51.01**、**LeviLamina 26.51.x**（已测试 26.51.5），且**仅支持 Windows x64 客户端**。不支持其他游戏/加载器系列或专用服务端。

### 通过 LeviLauncher 安装（收录后可用）

下载 [LeviLauncher](https://github.com/LiteLDev/LeviLauncher)，打开 `Bedrinth` 并搜索“BoatHud”，选择与游戏版本兼容的版本。模组需先被 [lipr 注册表](https://github.com/LiteLDev/lipr) 收录，发布 GitHub Release 并不代表已经上架。未收录时请使用下方 lip 或手动安装方法。

### 通过 lip 安装

安装或更新前完全退出 Minecraft。在 LeviLamina 游戏实例目录中使用 [lip](https://lip.levimc.org/)：

```powershell
lip install "github.com/MRUIAW/BedrockBoatHUD#client@1.0.1"
```

### 手动安装

从 [Releases](https://github.com/MRUIAW/BedrockBoatHUD/releases) 下载 `BedrockBoatHUD-client-windows-x64.zip`，将压缩包中的 `BoatHUD` 文件夹放进实例的 `mods` 文件夹。更新时保留原有 `config` 和 `data` 文件夹。Release 标题统一为 `v<模组版本>-mc<游戏系列>`，当前为 `v1.0.1-mc26.5x`；标题不扩大上述实际兼容范围。仅发布一份重新编译的客户端包。

实际安装目录为 `mods/BoatHUD/`。如果早期安装使用 `mods/BedrockBoatHUD`，请先退出游戏，将旧目录移到 `mods` 之外，避免两个模组同时加载；旧配置不自动迁移。

启动游戏并坐上船的驾驶位即可看到 HUD；作为乘客时不显示驾驶 HUD。

## 操作

| 按键 | 功能 |
| --- | --- |
| `B` | 开启/关闭 BoatHUD |
| `N` | 按 Race → Classic → Compact 切换布局 |
| `C` | 开启/关闭镜头辅助 |
| `O` | 打开设置 |

设置中将鼠标移到一行上，左键切换到下一值，右键切换到上一值。方向键上下选择行，左右调整；回车和空格也可切换。按 `O`、`Esc` 或选择“完成”关闭，修改自动保存。“隐藏原版 HUD”仅在菜单内设置，没有独立快捷键。

移动图标读取游戏的移动输入，支持移动键重新绑定和手柄；上述四个模组快捷键为固定键位。

## 速度条

速度条量程始终按 **m/s** 计算，与速度数字选择的单位无关。

| 模式 | 量程 | 颜色分区 |
| --- | --- | --- |
| 水与浮冰（`packed`，默认） | 0–40 m/s | 8 m/s 以下为蓝色，其后为灰白色 |
| 混合冰（`mixed`） | 0–72 m/s | 40 m/s 以下为灰白色，其后为蓝色 |
| 蓝冰（`blue`） | 40–72.7 m/s | 全蓝色 |

点亮长度随速度变化，超过上限时闪烁，速度条保持在面板边框内部。Race 的两个加速度读数分别按绝对值选取小数位：小于 10 显示两位小数，大于等于 10 显示一位小数。

## 配置与语言文件

以下路径均相对于游戏实例目录：

| 路径 | 用途 |
| --- | --- |
| `mods/BoatHUD/config/config.json` | 配置；首次加载时生成 |
| `mods/BoatHUD/lang/en.json` | 英文界面文本 |
| `mods/BoatHUD/lang/zh_CN.json` | 简体中文文本（`zh-Hans` / Minecraft `zh_CN`） |
| `mods/BoatHUD/data/telemetry/` | 带时间戳的遥测 CSV |
| `mods/BoatHUD/config/checkpoints.csv` | 默认检查点文件 |

手工修改 JSON 前退出游戏，也可以直接使用设置菜单。速度单位支持 `ms`、`kmh`、`mph`、`knots`，加速度单位支持 `g`、`mss`。高级镜头参数为 `cameraAggressiveness`（默认 60 m/s）、`cameraSmoothing`（每 Tick 保留比例 0.45）、`cameraMinimumSpeed`（0.5 m/s）。配置格式版本 3 与模组发布版本相互独立。

## 遥测与检查点

驾驶时在设置中开启“遥测 CSV”立即开始记录；关闭该选项或结束驾驶会刷新并关闭文件。导出使用固定单位，不随 HUD 单位改变。CSV 第一行是格式版本注释，第二行是表头，**并非 Java 原作的 11 列原样导出格式**。导入表格软件时跳过第一行。

检查点文件使用五列：

```csv
reference_time_seconds,reference_speed_mps,normal_x,normal_z,plane_offset
0.000,0.000,0.000,1.000,100.000
5.000,40.000,0.000,1.000,200.000
```

检查点平面为 `x * normal_x + z * normal_z = plane_offset`，从负侧穿越到正侧才触发。第一点建立圈起点；在 JSON 中开启 `circularTrack` 可循环检查点序列。检查点需手工准备；功能已实现，但真实赛道验收覆盖还没有 HUD 和相机充分。

详见 [遥测使用指南](TELEMETRY_GUIDE.md) 和 [检查点示例](examples/checkpoints.csv)。FPS 表示此 HUD 的渲染帧率，不是独立的整游戏性能测试；配置中的玩家名称显示字段为预留项，1.0.1 不绘制玩家名称。

## 构建与维护

需要 Windows x64、XMake、LLVM/Clang 22、Visual Studio C++ Build Tools 和 Windows SDK。

```powershell
xmake f -a x64 -m release -p windows --target_type=client -y
xmake -y
pwsh -File scripts/Package-Release.ps1
```

模组目录输出到 `bin/BoatHUD/`，校验后的 ZIP 输出到 `build/release/`。[CHANGELOG.md](CHANGELOG.md) 记录发布变更；[AGENTS.md](AGENTS.md) 为后续维护者提供规范、版本升级、相机符号、菜单鼠标及发布流程说明。[DEVELOPMENT_STATUS.md](DEVELOPMENT_STATUS.md) 保留开发与验收历史，其中早期方案可能已被后续版本替代。

## 致谢与许可证

项目代码使用 [CC0-1.0](LICENSE)。BoatHUD 设计与镜头行为参考上述两个 Java 项目。HUD 通过基岩版原生 UI 图元绘制，发行包没有包含原作的二进制纹理。本项目与 Mojang、Microsoft 及原模组作者没有隶属关系。
