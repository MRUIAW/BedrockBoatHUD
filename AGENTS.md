# Bedrock BoatHUD：维护 agent 交接指南

本文件同时是仓库内 agent 的工作约定和维护手册。开始修改前完整阅读。发布版本以根目录 `tooth.json` 为准；当前稳定基线为 1.0.0，来自用户已验收的 0.2.22。配置 schema 仍为 3，遥测 schema 仍为 1，不能把它们改成发布版本号。

## 1. 范围与环境

- 仅 Windows x64 客户端模组；目标 Minecraft Bedrock **1.26.51.01**，LeviLamina **26.51.x**，已构建/验收的 LL 为 **26.51.5**。
- C++20，Clang-CL / LLVM 22，Visual Studio C++ Build Tools、Windows SDK，XMake。
- HUD 和设置使用 Bedrock 原生 UI 图元与一个最小 `BaseScreen`。不需要资源包。
- 默认 Race、packed 速度条、km/h、g、图标与轨迹；镜头辅助、隐藏原版 HUD、CSV、检查点默认关闭。
- 用户已明确要求自绘菜单而非表单、原生鼠标光标、隐藏原版 HUD 仅放菜单中。
- 用户已确认相机第一/第三人称及高低速正常、手柄正常、G 表正常、CSV 可用、菜单光标正常、最终箭头修复正常。真实赛道检查点、多分辨率及全部生命周期组合没有完整验收记录，不要声称全部实机测试通过。

## 2. 必读文档与代码规范

遵循 [LeviLamina C++ 风格指南](https://lamina.levimc.org/zh/maintainer_guides/cpp_style_guide/)：目录/命名空间小蛇式，文件/类型/常量大驼峰，函数和普通变量小驼峰，私有成员 `m` 前缀。保留源码 SPDX 许可证头。头文件记录职责、参数、返回值和异常约定；公共配置字段有注释。格式以本仓库 `.clang-format` 为准。

权威资料：

| 资料 | 用途 |
| --- | --- |
| [开发者指南](https://lamina.levimc.org/zh/) | 生命周期、构建、客户端约束 |
| [创建/发布模组](https://lamina.levimc.org/zh/developer_guides/tutorials/create_your_first_mod/#发布你的模组) | tooth、changelog、v 前缀版本、GitHub Release |
| [API 文档](https://lamina.levimc.org/api/) | 确认 API 的声明；网页可能对应最新 LL |
| [找函数](https://lamina.levimc.org/zh/developer_guides/how_to_guides/find_function_guide/) | 版本升级时定位 Bedrock 函数 |
| [钩子指南](https://lamina.levimc.org/zh/developer_guides/how_to_guides/hook_guide/) | 确实需要新增 hook 时使用 |
| [I18n 指南](https://lamina.levimc.org/zh/developer_guides/how_to_guides/i18n_guide/) | 外部语言文件与 LL I18n |
| [lip 包清单](https://lip.levimc.org/concepts/package_manifest.html) | format v3、变体、依赖、安装位置与保留文件 |
| [Hibiii/BoatHud](https://github.com/Hibiii/BoatHud) | 镜头算法、左右箭头设计参考 |
| [BoatHUD Extended](https://github.com/jewtvet/boathud_extended) | 加速度、像素风格与速度条分区参考 |

目标 LL 包的本地头文件比不匹配版本的在线 API 更能说明当前 ABI。通常在 `%LOCALAPPDATA%/.xmake/packages/l/levilamina/<版本>/<包哈希>/include/`，包哈希不可硬编码到源码或可重复发布脚本中。

修改前检查 `git status`，保留用户的已有改动；按任务修改相关文件。不要把 `build/`、`bin/`、缓存、游戏日志、玩家配置或临时参考仓库提交。实机部署前完全退出 Minecraft；不在运行中替换 DLL。不要在未经授权的情况下创建公开 Release、推送标签或覆盖已发布资产；用户要求发布/推送时可直接完成该流程。无需主动派生子 agent。

## 3. 架构与入口

| 模块 | 职责与关键入口 |
| --- | --- |
| `src/mod/BoatHudMod.*` | `load()` 加载 LL I18n/配置；`enable()` 注册控制器；`disable()` 清理 |
| `src/client/ClientController.*` | Join/Exit/Tick/UI/键鼠事件；驾驶位检测；菜单与相机暂停；会话生命周期 |
| `src/session/BoatSession.*` | 20 Hz 速度、加速度、侧滑/角速度、输入历史 |
| `src/input/InputSampler.*` | 方向标志优先、按轴模拟回退，保留改键/手柄支持 |
| `src/hud/HudRenderer.*` | 三布局、像素速度条、G 表、速度平滑、HUD 渲染 FPS |
| `src/hud/HudVisibilityGuard.*` | 原版元素 Hide/Reset 状态恢复 |
| `src/camera/CameraAssist.*` | 每个有效游戏渲染帧调整玩家 yaw |
| `src/settings/BoatHudSettingsScreen.*` | 场景栈页面与鼠标所有权 |
| `src/settings/SettingsOverlay.*` | 面板绘制、坐标换算、即时配置、菜单退出 |
| `src/config/BoatHudConfig.*` | 默认值、验证、LL JSON schema 迁移 |
| `src/localization/Localization.*`、`lang/` | 游戏语言检测、LL I18n、安全英文回退 |
| `src/telemetry/TelemetryWriter.*` | 逐 Tick CSV、缓冲写入、刷新关闭 |
| `src/checkpoint/CheckpointManager.*` | 五列 CSV、平面穿越、Tick 内插值、环形序列 |

事件驱动链路：`ClientLevelTickEvent → findDrivenBoat → InputSampler → BoatSession → checkpoint/telemetry`；`AfterUIRenderEvent → 活动页面和游戏状态过滤 → camera/visibility → HUD`。不要把游戏对象访问提前到 `load()`。会话对象仅存在于本地玩家驾驶有效船只期间。

## 4. 相机升级：地址、符号与 hook

### 当前实现没有模组自写固定地址

相机不是通过一个固定 RVA 或绝对地址实现，也没有模组自己安装的 camera hook。`CameraAssist::update()` 调用 LL 目标版本头文件声明的 `LocalPlayer::localPlayerTurn(Vec2 const&)`。`@levibuildscript/linkrule`、BedrockData/运行时符号机制与 `bedrock_runtime_api.lib` 负责解析/链接相关 Bedrock 调用。其他事件背后的 LL 内部 hooks 由 LL 维护。

升级时，先确认实际声明，不要仅依据本文件假定新版本 ABI 相同。需要核对的文件和行为：

- `mc/client/player/LocalPlayer.h`：`localPlayerTurn` 的名字、参数、调用约定与可用性。
- `mc/world/actor/Actor.h`：`getVelocity()`、`getRotation()`、`getVehicle()`、`getFirstPassenger()`。
- `mc/client/game/IClientInstance.h`：本地玩家与游戏/菜单状态。
- `ll/api/event/render/UIRenderEvent.h`：`AfterUIRenderEvent` 时机和渲染上下文。
- `src/camera/CameraAssist.cpp` 与 `src/session/BoatSession.cpp`：两处 `BoatYawOffset = 90` 必须一起核对。

当前已验收的角度约定：角度单位为**度**；`localPlayerTurn(Vec2{0.0f, yawDelta})` 中第一分量为 pitch、第二分量为 yaw。把 yaw 差再转换成弧度会把跟随幅度缩小约 57 倍；交换分量会导致前进/后退时抬头。Bedrock 船 yaw 与本项目的船头方向相差 90°；速度方向为 `atan2(-velocity.x, velocity.z)` 转为角度。

算法为 `targetYaw = angleLerp(boatYaw, velocityYaw, clamp(speed / cameraAggressiveness, 0, 1))`。再将每 Tick 保留比例换算成逐帧保留比例 `pow(cameraSmoothing, frameSeconds * 20)`，沿最短角差平滑到目标。**不是始终锁向运动方向**：应保持 Hibiii 原作随速度变化的混合效果。只改变水平视角，菜单/退出驾驶时重置相机计时。

### 升级操作顺序

1. 记录 Minecraft 精确版本、LL 版本、客户端/架构和相关依赖。将 `xmake.lua` 的 LL 要求与 `tooth.json` 客户端依赖同步到新系列，不扩大到未测试系列。
2. 更新依赖并读取新包头文件，比较上述 API 与 `BaseScreen` 虚函数及 TypedStorage 字段布局。先解决编译/链接错误，再做实机方向测试。
3. 如出现 unresolved symbol 或加载异常，核对 LL、其 BedrockData、PreLoader、运行时文件与 Minecraft 版本是否配套；重建项目，检查 prelink 日志及 `build/.prelink/`。不要复用旧版本生成的运行时链接库。
4. 若 LL 暂未映射目标函数，按官方找函数指南在**目标版本客户端二进制**上定位，优先在 LL/BedrockData 上游更新符号；能用受支持的 API 时继续用 API。
5. 确实必须增加本地 hook 时，按 LL Hook API 编写独立模块和启停生命周期。记录目标版本、签名/特征、调用约定、RVA（如需）、验证证据及失败回退。解析失败应停用该功能并报告，不把未经验证的地址当有效指针。不能把 ASLR 后某次运行的绝对地址复制进源码。
6. 实机验证四个船头方向、向前/倒退、冰面漂移、跨 ±180°、低速/高速、第一/第三人称、主动鼠标转向、打开/关闭设置和下船。固定偏转先检查船 yaw 偏移；俯仰被改变先查 Vec2 分量；跟随微弱先查角度单位；卡顿先查调用频率与时间平滑。

本文件不提供虚构的新版地址。如果使用 IDA，只有任务实际需要二进制分析时再遵循可用的 IDAPython skill。

## 5. 容易回归的界面与输入约定

- `BoatHudSettingsScreen::shouldStealMouse()` **必须保持 `false`**，这才释放相对视角鼠标捕获。页面仍 `absorbsInput() == true`、`isModal() == true`，场景栈负责原生鼠标。
- 不恢复自绘软件光标、无条件逐帧 `releaseMouse()` 或直接访问 `HudScreenController::mShowCursor`。历史版本分别出现过光标漂移、消失和进世界崩溃。
- 设置不是 `custom_form`。保留游戏画面背后渲染和退出后的输入恢复。UI 像素来自 `GuiData::mScreenSizeData->clientUIScreenSize`，不是约 1×1 的归一化视口。
- 左右/前后按轴采用数字方向标志优先；只有该轴无标志时才用模拟向量。不要再将数字方向与模拟方向逐项 OR，否则 A/D 可能同时点亮两侧。
- 原版 HUD 隐藏默认关闭，菜单暂停/下船/退出/禁用时恢复。不得重新引入 H 快捷键。
- G 表为居中 18×18 外框、10×10 内框与 2×2 点，非十字线。点映射按 `acceleration / 2.5` 截断并限制 ±8，任一轴绝对值大于 22.5 时变红。
- packed 的蓝/白分界为 8 m/s，mixed 的白/蓝分界为 40 m/s；blue 全蓝。三模式量程依次为 0–40、0–72、40–72.7 m/s。边框内缩 1 像素，整数宽度及超速 Tick 闪烁。
- Race 加速度按换算后各自绝对值 `<10` 两位、`>=10` 一位；纵向带符号，横向取绝对值；斜杠两边有空格。此需求有意不完全采用 Java `threeSigFig()` 的 9.95/99.95 边界。
- LL I18n 从 `getLangDir()` 加载 `en.json` 与 `zh_CN.json`。仅一套简体中文；代码接受 `zh-Hans`，不是单独维护 zh-SG/TW/HK。两语言键集保持一致，配置继续保存内部标识。
- `showPlayerName` 是预留字段，当前未绘制，不把它列为已实现功能。

## 6. 构建、测试与发布

```powershell
xmake f -a x64 -m release -p windows --target_type=client -y
xmake -y
pwsh -File scripts/Package-Release.ps1
```

版本来自 `tooth.json.version`，`xmake.lua` 读取该值写入产物清单。运行 `clang-format --dry-run --Werror` 检查改动 C++，以及 `git diff --check`。发布打包脚本检验 tooth/manifest/tag/changelog 一致性、语言键、目录布局及产物，并从 changelog 提取 release notes。ZIP 不带个人配置或遥测。

验证分三层：编译/链接，按风险进行输入或算法回归，目标客户端实机。不能用编译通过代替鼠标、相机、驾驶位、GUI 缩放、关闭恢复的实机验收。曾在模拟引擎输入对象上运行生产 `InputSampler.cpp` 的 15 个场景；该检查不代表完整引擎 ABI 验证。

发布步骤遵循 [官方发布指南](https://lamina.levimc.org/zh/developer_guides/tutorials/create_your_first_mod/#发布你的模组)：

1. 更新 `tooth.json.version` 与两份 README 的安装版本/兼容说明，添加 `CHANGELOG.md` 的 `## [版本] - YYYY-MM-DD`，日期使用维护者所在时区。
2. 完成构建、打包、检查后提交相关源代码与文档，推送到 GitHub。不要提交构建输出或 force-push。
3. 推送 **`v` 前缀语义版本标签**（例如 `v1.0.0`）。本仓库扩展模板流程：标签触发客户端 release 工作流，构建成功后自动从 changelog 创建公开 Release 并上传 ZIP、tooth 与 SHA256；也支持手工创建 published Release。
4. 确认 Actions 完成，GitHub Release 实际有 `BoatHUD-client-windows-x64.zip`。`tooth.json` 下载 URL、ZIP 顶层 `BedrockBoatHUD/`、安装位置 `mods/BedrockBoatHUD/` 必须完全一致。展示名/ZIP 名可为 BoatHUD，不能因此错误改写实际 config/data 保留路径。原 `BedrockBoatHUD-client-windows-x64.zip` 保留为同内容兼容入口，避免已有 v1.0.0 清单失效。
5. lip 是 Git 仓库驱动分发，不存在必须上传到中央 lip 仓库的步骤。验证 `lip view github.com/MRUIAW/BedrockBoatHUD@版本` 与隔离目录中的 `lip install "github.com/MRUIAW/BedrockBoatHUD#client@版本"`。轻量安装检查可 `--no-dependencies`；它不代表加载器依赖的完整安装测试。
6. `preserve_files` 保护 config/data。发布时不塞进维护者的真实配置、日志、遥测、赛道文件或原项目纹理。保留 changelog/双语文档及示例。

CI 使用仓库 `GITHUB_TOKEN` 的 `contents: write` 发布，无需在仓库保存个人 token。Release ZIP、tooth、清单版本一致后，才能声称 lip 可安装；只有标签或 workflow 已排队时不能声称发布完成。

用户于 2026-10-01 指定：以后 Release **标题**统一为 `v<模组版本>-mc<游戏系列>`，例如 `v1.0.0-mc26.5x`。模组版本仍来自 tooth；游戏系列显式维护在 `release-config.json.minecraftSeries`，由打包脚本生成 `release-title.txt`，创建和更新 Release 时都使用它。升级游戏版本时同步修改此字段；它只是展示标签，不能据此放宽未经测试的兼容范围。Git 标签仍为 `v1.0.0`，不把 `-mc26.5x` 写进 tooth.version 或 tag，以免改变 lip 版本发现。

已获授权的原版本重发：使用 Release 的手动工作流，`tag` 为原版本标签、`source_ref` 为经过检查的新源码提交 SHA；完整重编译并覆盖原 Release 资产，不删 Release、不移动标签、不 force-push。正常首次发布 `source_ref` 应使用该 tag。同版本资产覆盖后，已安装的 lip 实例或下载缓存可能仍保留旧文件，不能把版本列表不变当作重编译失败。

开发历史见 `DEVELOPMENT_STATUS.md`。规格/可行性/原作分析是历史资料，遇到冲突以当前源码、用户最新验收要求和本指南为准，保留历史但不要再实施已撤回方案。
