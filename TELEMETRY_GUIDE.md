# Bedrock BoatHUD 遥测 CSV 使用指南

## 1. 文件位置与记录时机

遥测文件保存在：

```text
<游戏实例>/mods/BoatHUD/data/telemetry/<日期_时间>.csv
```

驾驶船时在设置菜单打开 `Telemetry CSV` 会立即创建文件并开始逐 Tick 记录。关闭该选项、下船、退出世界或停用模组时，文件会刷新并关闭。

## 2. 与 BoatHUD Extended 的兼容性

BoatHUD Extended 的原始表头是：

```csv
time,speed,gLon,gLat,slipAngle,angularVelocity,steering,throttle,xPos,zPos,yPos
```

Bedrock BoatHUD 保留了这些数据的含义，但使用更明确的单位名称，并增加 Tick、Ping 和 FPS：

```csv
tick,time_seconds,speed_mps,longitudinal_acceleration_mps2,lateral_acceleration_mps2,slip_angle_degrees,angular_velocity_dps,steering,throttle,x,y,z,ping_ms,fps
```

两者不是逐字节相同的格式。依赖 Extended 固定表头、固定 11 列或 `x,z,y` 坐标顺序的旧脚本不能直接读取当前文件；通用的 Excel、LibreOffice、Python、R 或数据可视化软件可以正常使用。原版的 `gLon`、`gLat` 实际记录加速度，本版用 `m/s²` 明确标注。

## 3. 字段说明

| 字段 | 含义 | 单位或范围 |
| --- | --- | --- |
| `tick` | 当前驾驶会话的客户端采样序号 | 20 Tick/s |
| `time_seconds` | 从驾驶会话开始计算的时间 | s |
| `speed_mps` | 水平速度 | m/s |
| `longitudinal_acceleration_mps2` | 沿运动方向的加减速度 | m/s² |
| `lateral_acceleration_mps2` | 横向加速度 | m/s² |
| `slip_angle_degrees` | 船头方向与运动方向的有符号夹角 | ° |
| `angular_velocity_dps` | 船头旋转速度 | °/s |
| `steering` | 转向输入；左为正、右为负 | -1～1 |
| `throttle` | 油门输入；前进为正、后退为负 | -1～1 |
| `x`,`y`,`z` | 船的世界坐标 | 方块 |
| `ping_ms` | 客户端测得的服务器延迟；本地世界可能为 -1 | ms |
| `fps` | BoatHUD HUD 渲染帧率 | 帧/s |

## 4. 用 Excel 或 LibreOffice 分析

文件第一行是格式版本注释，第二行才是表头。使用“从文本/CSV 导入”时选择逗号分隔，并跳过第一行注释。

常用图表：

1. 以 `time_seconds` 为横轴、`speed_mps` 为纵轴，查看加速、失速和不同冰面的速度变化。
2. 同时绘制 `steering`、`slip_angle_degrees` 和 `lateral_acceleration_mps2`，观察一次转向何时开始漂移、何时恢复抓地。
3. 以 `x` 为横轴、`z` 为纵轴绘制散点图，可以得到俯视行驶轨迹并比较线路。
4. 对多次记录比较相同位置附近的 `time_seconds` 和 `speed_mps`，判断哪次路线更快。
5. 用 `ping_ms` 和 `fps` 排查某次异常操作是否同时伴随网络或渲染卡顿。

## 5. 制作检查点文件

遥测 CSV 不能直接改名为 `checkpoints.csv`。检查点文件每行需要：

```csv
reference_time_seconds,reference_speed_mps,normal_x,normal_z,plane_offset
```

可从一次满意的基准跑法中选择若干 `x,z` 位置，并取该处前进方向在水平面的单位向量作为 `normal_x,normal_z`。随后计算：

```text
plane_offset = x * normal_x + z * normal_z
```

把该行的 `time_seconds`、`speed_mps` 和计算出的平面参数写入 `mods/BoatHUD/config/checkpoints.csv`。比赛时只有从法平面的负侧穿到正侧才会触发，因此法向量必须指向赛道前进方向。

当前仍需手工选择检查点。后续可以增加“从遥测轨迹生成检查点”的转换工具，以及严格复刻 Extended 11 列表头的兼容导出模式。
