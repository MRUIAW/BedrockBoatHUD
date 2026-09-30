# BoatHud 与 BoatHUD (Extended) 合并功能规格

## 1. 分析对象

本文比较并整合以下两个项目：

| 项目 | 分析版本 | 定位 |
| --- | --- | --- |
| [Hibiii/BoatHud](https://github.com/Hibiii/BoatHud) | `a7018c79a64fe561187add11aeb19c39dcbce2f6`，2026-07-06，模组 1.3.1 | 当前仍在更新的基础版，偏向简洁 HUD、驾驶体验和原版视觉 |
| [jewtvet/boathud_extended](https://github.com/jewtvet/boathud_extended) | `a64b79679e8d64d67a59a0a42ddf9c2643f935ba`，2024-08-02，模组 1.1.0 | 扩展版，偏向竞速数据、遥测与检查点 |

目标不是机械地拼接两份 Java 代码，而是在 LeviLamina 客户端模组中实现一套统一功能：保留两边有价值的能力，对重复功能选择更可靠的算法，并修复原实现中已发现的问题。

## 2. 两个项目共有的功能

以下能力两边均有，应当作为合并版的基础功能：

- 纯客户端运行，不要求服务器安装模组；
- 仅在本地玩家驾驶船时显示 HUD；
- 显示水平速度；
- 显示船头与实际移动方向之间的漂移/侧滑角；
- 扩展和紧凑两种 HUD 思路；
- 显示方向、前进、后退输入；
- 显示 Ping；
- 三种速度条范围：水/浮冰、混合冰、蓝冰；
- 支持 `m/s`、`km/h`、`mph`、`kt`；
- 使用 Minecraft 像素字体与像素纹理；
- 配置启用状态、布局、速度单位和速度条类型。

共有功能不直接照搬任一实现，而采用以下统一原则：

- 每 Tick 直接检查玩家是否为船的驾驶者，不依赖乘客网络包 Hook；
- 内部统一保存 SI 单位，只有显示层进行单位转换；
- 侧滑角内部保留正负方向，界面可选择显示绝对值或带符号值；
- 输入从 Bedrock 移动输入组件读取，以兼容改键、手柄和触摸；
- 所有 HUD 坐标使用 GUI 缩放后的逻辑像素；
- 所有显示项由同一份状态快照驱动，避免渲染期间读取已经变化的游戏对象。

## 3. Hibiii/BoatHud 独有或实现更好的功能

### 3.1 驾驶视角跟随

`CameraHandler` 根据船速在船头方向和速度方向之间插值，再平滑改变玩家 Yaw：

```text
velocityDirection = atan2(velocity.z, velocity.x) - 90°
lookAheadWeight   = clamp(horizontalSpeedPerTick / aggressiveness, 0, 1)
targetYaw         = angleLerp(boatYaw, velocityDirection, lookAheadWeight)
newYaw            = angleLerp(targetYaw, playerYaw, smoothing)
```

这相当于一个简化的 Boat Cam：速度越高，视角越倾向船的实际行进方向，冰船漂移时更容易看向赛道前方。

这是合并版应该新增的功能，但应做以下改进：

- 默认关闭，避免意外改变玩家视角；
- 增加最小生效速度，低速或静止时不追踪不稳定的速度方向；
- 正确处理 `±180°` 角度跨界；
- 允许玩家主动移动视角时暂时减弱或暂停自动跟随；
- 在离船、打开菜单或禁用功能时立即停止；
- 将“跟随强度”和“平滑/惯性”分成含义明确的配置项。

在 LeviLamina 中没有发现稳定的高层 Camera 事件/API。当前 MCAPI 提供 `LocalPlayer::localPlayerTurn(Vec2)`、`Actor::setRotationWrapped(Vec2)` 以及客户端 Camera 对象，因此功能可实现，但属于版本敏感部分，应封装为独立 `CameraAssist` 适配层并进行实机测试。

### 3.2 渲染速度平滑

Hibiii 版在渲染帧之间对显示速度做插值，使 20 Hz 的 Tick 数据看起来更流畅。这一体验优于 Extended 直接显示当前 Tick 值。

合并版保留“平滑显示”，但不照搬递归 `lerp(displayedSpeed, actualSpeed)`。LL 的 UI 事件没有直接提供公开 Tick Delta，建议使用渲染时间间隔实现帧率无关的指数平滑：

```text
alpha = 1 - exp(-responseRate * frameDeltaSeconds)
displayedSpeed += (actualSpeed - displayedSpeed) * alpha
```

配置提供：关闭、快速、平滑三档；遥测和检查点始终使用未经平滑的真实 Tick 值。

### 3.3 渐进式速度条

Hibiii 版新增 `PROGRESSIVE` 第四种速度条，将不同速度段使用不同缩放：

- `0～8 m/s`：高分辨率显示水上低速；
- `8～40 m/s`：显示普通冰/浮冰速度；
- `40～70 m/s`：显示蓝冰高速。

该模式适合不想手动切换速度条的用户，应合并。最终建议保留：

1. Water/Packed；
2. Mixed；
3. Blue Ice；
4. Progressive；
5. 可选 Custom，自定义最小/最大速度。

默认范围建议数据化而不是写死在渲染器中，并允许把高速上限设置为 `72.7 m/s`，避免 Extended 与新版项目不同上限造成维护分叉。

### 3.4 Ping 图标和独立 Sprite

Hibiii 版复用了原版玩家列表的五档 Ping 图标，视觉上比单纯的 `123 ms` 更接近原版；同时将背景、速度条、按键和 Ping 图标拆成独立 sprite，资源维护明显优于一张硬编码坐标的大图集。

合并版采用：

- Bedrock 资源包中的独立纹理；
- 默认显示 Ping 图标；
- 竞速/诊断布局可同时显示数值；
- 未知 Ping 使用单独图标；
- 阈值保留为配置或常量：`<150`、`<300`、`<600`、`<1000`、更高。

### 3.5 玩家名称

Hibiii 扩展布局显示本地玩家名称。它对单人本地 HUD 的信息价值不高，但能强化赛车仪表风格，也可能用于截图或直播。因此保留为可选显示项，默认关闭。

## 4. BoatHUD (Extended) 独有或实现更好的功能

### 4.1 二维加速度与 G 值表

Extended 同时计算：

- 纵向加速度 `longitudinalAcceleration`；
- 横向加速度 `lateralAcceleration`；
- 角速度；
- 带方向的侧滑角。

并用 18×18 的二维表显示横向/纵向加速度点。这比 Hibiii 只显示纵向 `g` 更适合竞速分析，应完整保留。

改进点：

- 内部字段不要命名为 `g`，明确区分 `m/s²` 和重力加速度倍数；
- 所有角度差先归一化到 `[-180°, 180°]`；
- G 值表量程和警示阈值可配置；
- 显示可在 `m/s²` 与 `g` 之间切换；
- 可选择同时显示二维点和具体数值。

### 4.2 输入轨迹

Extended 保存最近 40 Tick（约 2 秒）的方向与油门轨迹。Hibiii 只显示当前输入状态。

两者不冲突，合并版应同时支持：

- 当前输入灯：立即看出左右/前后是否按下；
- 历史轨迹：用于分析漂移前后的操作节奏；
- 轨迹长度可配置，例如 20、40、60 Tick；
- 模拟输入保留连续值，而不是只有 `-1/0/1`。

Hibiii 版的左右方向图标必须保留，并作为独立可配置项。为避免多个布尔开关产生无意义组合，建议使用一个输入显示模式：

| 模式 | 显示内容 |
| --- | --- |
| `Off` | 不显示驾驶输入 |
| `Icons` | 显示 Hibiii 风格的左右方向图标和前进/后退条 |
| `Trace` | 只显示 Extended 风格的转向与油门历史轨迹 |
| `IconsAndTrace` | 同时显示当前图标和历史轨迹 |

`Classic` 默认使用 `Icons`；`Race` 默认使用 `IconsAndTrace`。用户仍可在任意布局中选择其他模式。

### 4.3 FPS 与数字 Ping

Extended 显示 FPS 和数字 Ping，并按阈值改变颜色。这些信息应保留在竞速或诊断布局中。

LL 没有稳定的原生 FPS getter，合并版默认统计 `hud_screen` 的渲染次数，得到“HUD 渲染 FPS”。如果以后使用内部 `ProfilerLite`，必须作为按版本启用的可选实现。

颜色阈值不应完全照搬 Extended 的固定值（例如低于 120 FPS 就标红对很多设备不合理），建议配置为：

- 自动：相对当前帧率上限判断；
- 自定义阈值；
- 关闭颜色提示。

### 4.4 遥测 CSV

Extended 能记录：

```text
time, speed, longitudinal acceleration, lateral acceleration,
slip angle, angular velocity, steering, throttle, x, z, y
```

这是合并版的重要高级功能，应保留并改进：

- 一次会话保持文件流打开并缓冲，不要每 Tick 打开/关闭文件；
- 默认写入模组数据目录，不使用固定 `C:/boat_telemetry/`；
- 文件名加入日期、时间和可选服务器/世界标识；
- 可配置“登船即记录”或“第一次前进输入后记录”；
- 写入配置快照、格式版本和采样频率；
- I/O 错误写入日志并在 HUD 提示；
- 可选记录原始值与平滑显示值，但默认只记录原始值。

### 4.5 检查点和环形赛道

Extended 从文件加载赛道检查点，检测船跨过检查点平面，显示相对基准的时间差和速度差，并支持到达终点后循环。

该功能完整保留，但重新实现时修复以下问题：

1. 原速度插值权重写反。正确公式应为：

   ```text
   crossingSpeed = lastSpeed × (1 - t) + currentSpeed × t
   ```

2. 检查点法向量必须验证非零并归一化。
3. `dotCurrent - dotLast` 接近零时不能除法。
4. 数值必须为有限值，行字段数必须正确。
5. 必须明确定义检查点方向，避免反向穿越也计时。
6. 允许一 Tick 内依次穿过多个非常接近的检查点，但要设置循环上限。
7. 增加赛段、单圈和最佳圈数据模型，为后续排行榜预留接口。

### 4.6 HUD 位置、原版 HUD 替换和状态恢复

Extended 支持 Y 偏移，并在 BoatHUD 显示时隐藏状态栏、经验栏和快捷栏。合并版继续支持，但扩展为：

- 水平锚点：左、中、右；
- 垂直锚点：上、中、下；
- X/Y 偏移；
- 原版 HUD 策略：保留、仅隐藏冲突元素、竞速全替换；
- 离船、聊天、暂停、退出世界或禁用模组时必定恢复。

在 LL 中使用 `GuiData::setHudVisibilityState()`，不需要 Hook 每个原版绘制函数。

## 5. 统一后的 HUD 布局

两个项目的“extended/compact”名称相同，但实际版式并不一致。强行只保留一个布尔开关会丢失价值。建议改为布局枚举：

### 5.1 Compact（紧凑）

适合普通游玩，默认显示：

- 速度条；
- 速度；
- 侧滑角；
- 四方向当前输入；
- 可选 Ping 图标。

不显示 G 值表、输入历史、FPS、玩家名和检查点速度差。

示意布局：

```text
┌──────────────────────────────┐
│████████  Progressive Speed   │
│ 135 km/h      012°      ▂▄▆█ │
│             ◀ ▼ ▲ ▶          │
└──────────────────────────────┘
```

这是最小、最不遮挡画面的模式。输入图标和 Ping 均可关闭，关闭后高度还能进一步缩小。

### 5.2 Classic（经典）

接近 Hibiii 当前版截图：

- 182 px 原版风格背景；
- 速度、侧滑角、纵向加速度；
- 左右方向灯、前进/后退条；
- Ping 图标；
- 可选玩家名；
- 平滑显示速度。

大致对应 Hibiii/BoatHud 当前截图中的界面：宽度约 182 个 GUI 像素，视觉上像一块原版状态栏。第一行是速度条，第二行是三个主要数据，底部放驾驶输入和网络状态。

```text
┌────────────────────────────────────────┐
│████████████████  SPEED BAR             │
│ 135 km/h        012°           +0.3 g  │
│ ◀◀  ▶▶                       ▂▄▆█ Name │
│      BRAKE ◀──────────▶ THROTTLE       │
└────────────────────────────────────────┘
```

其中 `◀◀`、`▶▶` 就是要保留的 Hibiii 左右键图标：未转向时使用暗色纹理，按下对应方向时切换为亮色纹理。前进/后退也采用同样的点亮方式。

Classic 的定位是“日常驾驶仪表”：信息足够但不过密，最接近 Hibiii 原项目的原版风格。

### 5.3 Race（竞速扩展）

接近 BoatHUD Extended：

- 更宽的仪表区域；
- 速度、侧滑角、纵横向加速度；
- 二维 G 值表；
- 当前输入与最近若干 Tick 输入轨迹；
- Ping 图标/数值、FPS；
- 检查点时间差和速度差；
- 用颜色区分领先、落后和异常状态。

示意布局：

```text
┌────────────────────────────────────────────────────┐
│██████████████████████  SPEED BAR                   │
│135 km/h  throttle trace  [G-METER]  steer trace 012°│
│-0.12 s / +1.4 km/h       ▂▄▆█ 42 ms       165 fps │
│              ◀◀  ▶▶   BRAKE / THROTTLE            │
└────────────────────────────────────────────────────┘
```

Race 的定位是“冰船竞速和调校”：宽度更大、数据显示更完整。Hibiii 左右键图标默认保留在底部输入区，历史轨迹则显示最近约两秒的操作；如果用户认为太拥挤，可以把输入模式从 `IconsAndTrace` 改为 `Trace` 或 `Icons`。

检查点未启用时，左下区域显示纵/横向加速度；启用检查点后切换成时间差和速度差。Ping 默认同时显示图标与毫秒数。

### 5.4 Custom（自定义）

后期允许逐项开关，并由简单布局系统排列。第一版不建议立即实现自由拖拽编辑器，先完成三个经过设计的固定预设。

## 6. 重复功能的择优结果

| 重复功能 | 采用方案 | 原因 |
| --- | --- | --- |
| 登船检测 | 新的每 Tick 状态机 | 两项目都依赖网络包 Hook；直接检查载具更稳健 |
| 速度计算 | 两者共同公式 + Hibiii 平滑显示 | 数据准确且视觉更流畅 |
| 侧滑角 | Extended 的带符号归一化 | 比 `acos` 更容易表达左右方向，且可再取绝对值 |
| 纵向加速度 | Extended 原始 `m/s²` 数据模型 | 单位转换应放显示层，支持 `g` 与 `m/s²` |
| 横向加速度 | Extended | Hibiii 没有对应功能 |
| 输入显示 | Hibiii 当前输入灯 + Extended 历史轨迹 | 两者服务不同目的，可同时保留 |
| 输入来源 | LL/Bedrock `MoveInputComponent` | 比直接读键盘兼容性更好 |
| 速度条 | Hibiii 的四模式设计 + 数据化范围 | 保留 Progressive，并消除硬编码分叉 |
| Ping | Hibiii 图标 + Extended 可选数字 | 原版风格与精确数据兼得 |
| FPS | Extended 功能，LL 渲染计数实现 | 保留诊断价值，规避内部字段依赖 |
| HUD 资源 | Hibiii 独立 sprite 思路 | 比 Extended 单图集硬编码 UV 易维护 |
| HUD 定位 | Extended 偏移 + 新锚点 | 满足不同屏幕与 HUD 共存需求 |
| 原版 HUD | 可配置策略 | 两项目行为冲突，不应硬编码只选一边 |
| 配置文件 | LL 版本化 JSON | 优于两边静默吞错的 Properties 实现 |
| 遥测 | Extended 功能 + 持久缓冲流 | 避免每 Tick 打开文件造成的性能与数据风险 |
| 检查点 | Extended 功能 + 修正插值/验证 | 保留功能并修复数学和输入健壮性问题 |
| 摄像机 | Hibiii 功能 + 手动覆盖/低速保护 | 保留体验优势，避免自动视角与玩家争夺控制 |

## 7. 合并后的完整功能清单

### 7.1 核心驾驶数据

- 船的水平速度；
- 纵向加速度；
- 横向加速度；
- 船头方向；
- 行进方向；
- 带符号侧滑角；
- 角速度；
- 当前转向、油门、刹车/倒车输入；
- Ping；
- HUD 渲染 FPS；
- 会话 Tick 时间与真实时间。

### 7.2 显示功能

- Compact、Classic、Race 三个布局；
- Minecraft Bedrock 原版风格字体、背景和图标；
- 五档 Ping 图标与可选数字；
- 四种速度条和自定义范围；
- 平滑速度动画；
- 当前输入灯；
- 输入历史轨迹；
- 二维 G 值表；
- 检查点时间差、速度差；
- 领先/落后、低 FPS、高 Ping 警示颜色；
- 自定义锚点、X/Y 偏移、GUI 缩放适配；
- 聊天、暂停和非游戏界面自动隐藏；
- 可配置隐藏原版 HUD 元素。

### 7.3 竞速与数据功能

- 遥测 CSV；
- 检查点文件；
- 单向穿越检测；
- 环形赛道；
- 基准时间与基准速度；
- 赛段、单圈和最佳圈的可扩展数据结构；
- 会话开始/结束自动管理；
- 文件解析与写入错误提示。

### 7.4 体验与控制

- HUD 总开关快捷键；
- 布局循环快捷键；
- 摄像机跟随总开关；
- 摄像机跟随强度、平滑度和最低速度；
- 玩家主动转动镜头时的临时覆盖；
- 所有配置使用版本化 JSON 保存；
- 客户端命令用于无图形设置页阶段的配置与诊断。

## 8. LeviLamina 实现映射

| 合并功能 | LL/MC API 路径 | 风险等级 |
| --- | --- | --- |
| 世界进入/退出 | `ClientJoinLevelEvent`、`ClientExitLevelEvent` | 低 |
| 每 Tick 状态更新 | `ClientLevelTickEvent` | 低 |
| 本地玩家 | `ll::service::getClientInstance()` → `getLocalPlayer()` | 低 |
| 载具和驾驶位 | `Actor::getVehicle()`、`getFirstPassenger()` | 中低 |
| 位置、速度、旋转 | `Actor::getPosition/getVelocity/getRotation` | 中低 |
| 移动输入 | `ClientMoveInputHandler::getMoveInput()` | 中 |
| HUD 绘制 | `AfterUIRenderEvent`、`MinecraftUIRenderContext` | 低 |
| 纹理和字体 | `getTexture/drawImage/drawText`、客户端资源包 | 中低 |
| 隐藏原版 HUD | `GuiData::setHudVisibilityState()` | 低 |
| Ping | `IClientInstance::getServerPingTime()` | 中低 |
| FPS | `AfterUIRenderEvent` 计数 | 低，数值为近似渲染 FPS |
| 自定义快捷键 | `KeyRegistry`、`KeyHandle` | 低 |
| 配置 | `ll::config::loadConfig/saveConfig` | 低 |
| 数据目录 | `Mod::getDataDir/getConfigDir` | 低 |
| 摄像机跟随 | `LocalPlayer::localPlayerTurn`/旋转或 Camera MCAPI | 中高，必须实机验证 |
| 自定义配置页 | UI 渲染 + 键鼠事件 | 中高，需自行实现控件 |

摄像机跟随是本次合并中新出现的唯一明显高风险功能。它不需要服务端配合，但调用的是 Minecraft 客户端内部 API，升级 LL/Minecraft 时必须重新测试。即使该模块暂时失效，也不应影响 HUD、遥测和检查点。

## 9. 建议配置模型

```text
BoatHudConfig
├─ version
├─ enabled
├─ layout: Compact | Classic | Race
├─ units
│  ├─ speed: MS | KMPH | MPH | KNOTS
│  ├─ acceleration: MSS | G
│  └─ signedSlipAngle
├─ placement
│  ├─ horizontalAnchor
│  ├─ verticalAnchor
│  ├─ offsetX
│  └─ offsetY
├─ display
│  ├─ speedSmoothing
│  ├─ showPingIcon
│  ├─ showPingNumber
│  ├─ showFps
│  ├─ showPlayerName
│  ├─ inputDisplayMode: Off | Icons | Trace | IconsAndTrace
│  └─ traceLengthTicks
├─ speedBar
│  ├─ type
│  ├─ customMin
│  └─ customMax
├─ vanillaHud
│  └─ mode: Keep | HideConflicting | ReplaceRaceHud
├─ cameraAssist
│  ├─ enabled
│  ├─ strength
│  ├─ smoothing
│  ├─ minimumSpeed
│  └─ manualOverride
├─ telemetry
│  ├─ enabled
│  ├─ startMode
│  └─ directory
└─ checkpoints
   ├─ enabled
   ├─ file
   └─ circularTrack
```

## 10. 推荐默认值

为了既像 BoatHud 又不突然干预用户，建议初始默认：

| 配置 | 默认值 |
| --- | --- |
| HUD | 启用 |
| 布局 | `Race` |
| 速度单位 | `km/h` |
| 加速度单位 | `g` |
| 速度条 | `Progressive` |
| 速度平滑 | 快速 |
| 输入显示 | `IconsAndTrace`，同时显示 Hibiii 左右键图标和历史轨迹 |
| Ping | 图标和数字均开启 |
| FPS | 开启 |
| 玩家名 | 关闭 |
| 摄像机跟随 | 关闭 |
| 摄像机跟随方式 | 开启后直接改变玩家视角/Yaw，与 Hibiii 原版一致 |
| 遥测 | 关闭 |
| 检查点 | 关闭 |
| 原版 HUD | 默认保留；可选隐藏与 BoatHUD 重叠的元素 |

## 11. 开发优先级

### P0：最小可用 HUD

- 驾驶船状态机；
- 速度、侧滑角、纵向加速度和 Ping；
- Compact/Classic 布局；
- Progressive 速度条；
- 原版风格纹理和字体；
- JSON 配置与快捷键；
- 原版 HUD 恢复保护。

### P1：竞速扩展

- Race 布局；
- 横向加速度和 G 值表；
- 当前输入与历史轨迹；
- FPS；
- 遥测 CSV；
- 检查点、环形赛道、时间差和速度差。

### P2：驾驶体验与配置界面

- 摄像机跟随；
- 手动镜头覆盖；
- 自制客户端配置覆盖层；
- 自定义 HUD 组件开关和锚点。

### P3：可选增强

- 单圈、最佳圈与赛段；
- HUD 编辑器；
- 服务器同步赛道或排行榜；
- 数据导出格式扩展。

## 12. 已确认的产品选择

1. 首次安装默认使用 `Race` 高信息密度布局。
2. `Race` 默认同时显示 Hibiii 风格的左右键/油门图标与 Extended 风格的历史输入轨迹。
3. 驾驶视角跟随采用 Hibiii 原版方案：功能开启后直接调整玩家视角/Yaw，而不是操作独立的本地 Camera 对象。
4. 摄像机跟随默认关闭，由用户主动开启；强度、平滑度和最低生效速度均可配置。
5. 原版 HUD 隐藏功能保留为配置项，但默认关闭，不主动隐藏快捷栏、生命、饥饿等原版元素。

直接调整玩家 Yaw 的实现优先尝试 `LocalPlayer::localPlayerTurn(Vec2)`，必要时使用 `Actor::setRotationWrapped(Vec2)`。具体旋转轴顺序、网络同步行为以及第一/第三人称表现必须在固定的 LeviLamina/Minecraft 客户端版本中实机验证。

## 13. 许可说明

Hibiii/BoatHud 仓库明确使用 Unlicense，并在 README 中允许学习和整合。BoatHUD Extended 的 `fabric.mod.json` 声明 MIT，但当前仓库根目录没有随附 LICENSE 文件。

实现算法与功能思路没有障碍；若要直接复制 Extended 的纹理或大段源码到未来公开发布的 LL 项目，建议先补充确认其许可证文本及署名要求。新的 Bedrock 资源包优先以 Hibiii 的 Unlicense 素材为参考重新整理，可降低许可和图集维护风险。
