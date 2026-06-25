# mo_ecat_core 公共接口说明

> 本文档说明 UI 工程允许依赖的 `mo_ecat_core` 公共接口，以及当前实现状态。

## 1. 公共头文件

UI/bridge 只允许依赖以下公共头文件：

```cpp
#include <mo_ecat/master.h>
#include <mo_ecat/config.h>
#include <mo_ecat/types.h>
```

禁止 UI/bridge include `mo_ecat_core/src/` 下的头文件。`src/` 下的 `EcatController`、`EcMaster`、`SlaveNodeManager`、`ProcessDataEngine` 等都属于内部实现。

## 2. 主入口类

当前核心库主入口为：

```cpp
mo_ecat::MoEcatMaster
```

该类通过 PImpl 隐藏内部实现。外部调用方不需要，也不应该知道 SOEM、控制器、调度器、从站节点等内部对象。

## 3. 生命周期接口

| 接口 | 当前状态 | 说明 |
|------|----------|------|
| `InitializeAdapter(const EcMasterConfig&)` | 已实现 | 初始化网卡和主站配置 |
| `Scan()` | 已实现 | 扫描 EtherCAT 从站 |
| `EnterMaintenance()` | 已实现 | 进入维护/PREOP 阶段 |
| `PrepareRun()` | 已实现 | 配置 PDO、SAFEOP、DC，准备运行 |
| `StartOperation()` | 已实现 | 请求进入 OPERATIONAL |
| `BackToMaintenance()` | 已实现 | 从 ReadyToRun/Operational 回到 Maintenance |
| `Stop()` | 已实现 | 停止主站 |
| `RequestFault(const std::string&)` | 已实现 | 外部请求进入 Fault |
| `RequestEmergencyStop(const std::string&)` | 已实现 | 外部请求进入 EmergencyStop |

调用约束：

- 生命周期接口应在 `MoEcatMaster` 所在线程调用。
- UI 线程不直接调用这些接口，应通过 bridge 投递命令。
- 返回 `false` 表示动作失败或状态不允许，UI 应读取状态并提示用户。

## 4. 周期服务接口

```cpp
void Service();
```

当前实现中，`Service()` 会根据主站状态执行单步服务：

- `Maintenance`：检查从站状态。
- `Operational`：执行一次 PDO 周期并检查从站状态。
- 其他状态：基本不做周期动作。

调用约束：

- `Service()` 不主动创建主站周期线程。
- 调用方需要按周期调用它。
- 推荐由 `core_bridge` 的 worker 线程调用。
- 不建议由 UI 主线程调用。

## 5. 状态查询接口

| 接口 | 当前状态 | 说明 |
|------|----------|------|
| `GetState()` | 已实现 | 返回 `MasterState` |
| `GetSlaveCount()` | 已实现 | 返回当前从站数量 |
| `GetSlaveInfos()` | 已实现 | 返回从站静态信息列表 |
| `GetSnapshot()` | 部分实现 | 当前填充 state/slaves，`last_fault` 尚未填充 |

UI 展示状态时，应优先通过 bridge 统一拉取和缓存，避免多个 UI 控件直接并发访问 core。

## 6. 诊断接口

| 接口 | 当前状态 | 说明 |
|------|----------|------|
| `ReadSdo(...)` | 已实现 | 同步 SDO 读 |
| `WriteSdo(...)` | 已实现 | 同步 SDO 写 |
| `DumpPdoMapping(int slave_id)` | 已实现 | 通过 SDO 读取并格式化 PDO 映射 |

调用约束：

- SDO 接口是同步阻塞调用，默认超时为 1000 ms。
- `DumpPdoMapping()` 内部也会进行 SDO 读取，可能阻塞。
- 这些接口不得在 UI 主线程直接调用。
- UI 应显示 busy/error 状态，并允许后续扩展取消或超时策略。

## 7. 回调接口

| 回调 | 当前状态 | 触发线程/说明 |
|------|----------|---------------|
| `on_state_changed` | 已实现 | 在 `Service()` 调用线程中触发 |
| `on_log_message` | 已实现 | 在 Logger 后台线程中触发 |
| `on_fault` | 已定义，当前未触发 | 需要后续实现补齐 |
| `on_feedback` | 已定义，当前未触发 | 需要后续实现补齐周期反馈发布 |

回调约束：

- 回调中不得直接操作 UI 控件。
- 回调中不应执行耗时操作。
- 回调中不建议重入调用 `MoEcatMaster` 的阻塞接口。
- bridge 接收到回调后，应使用 Qt queued signal 或线程安全队列转发给 UI。

## 8. 配置结构

`EcMasterConfig` 当前字段包括：

| 字段 | 说明 |
|------|------|
| `ifname` | 网卡名，例如 `eth0`、`enp0s31f6` |
| `cycle_time_us` | 周期时间，默认 1000 us |
| `use_dc` | 是否使用 DC |
| `expected_slave_count` | 预期从站数量，0 表示不校验 |
| `expected_identities` | 预期从站身份列表 |
| `log_sink` | 日志输出模式 |
| `log_path` | 日志文件路径 |
| `feedback_publish_hz` | 反馈发布频率配置；当前反馈回调尚未实际触发 |

UI 可以提供配置界面，但必须通过 bridge 显式转换为 `EcMasterConfig`。

## 9. 公共数据结构

| 类型 | 用途 |
|------|------|
| `MasterState` | 主站生命周期状态 |
| `SlaveIdentity` | 预期从站身份校验 |
| `SlaveInfo` | 扫描后的从站静态信息 |
| `SlaveFeedback` | 周期反馈数据结构，当前回调尚未发布 |
| `MasterSnapshot` | 主站状态和从站列表快照 |

这些结构可以跨 bridge 复制到 UI 侧，但 UI 不应修改后再假设 core 内部状态也被同步。

## 10. 后续接口补齐建议

为支持 UI 第一版，建议优先补齐：

1. `on_fault` 触发逻辑，至少在进入 Fault/EmergencyStop 时发布原因。
2. `MasterSnapshot::last_fault` 填充逻辑。
3. `on_feedback` 周期发布逻辑，并遵守 `feedback_publish_hz`。
4. 明确 `MoEcatMaster` 是否要求单线程访问；如果要求，应在文档和代码注释中固定下来。

