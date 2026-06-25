# UI 与 core_bridge 边界说明

> 本文档说明 Qt UI、`core_bridge` 和 `mo_ecat_core` 之间的边界条件。目标是让 UI 开发不侵入 EtherCAT 核心，同时让核心能力能安全、稳定地呈现在界面上。

## 1. 分层目标

推荐分层：

```text
app/main
  -> app_controller
    -> gui
    -> core_bridge
      -> mo_ecat_core
```

每层职责：

| 层 | 职责 |
|----|------|
| `gui` | 窗口、控件、表格、用户输入、状态显示 |
| `app_controller` | 组装 UI 和 bridge，处理页面级流程 |
| `core_bridge` | 持有 core 实例、线程调度、数据转换、命令队列、Qt 信号输出 |
| `mo_ecat_core` | EtherCAT 主站能力 |

## 2. UI 不直接接触 core 的内容

UI 层不应直接做以下事情：

- 不直接 `#include <mo_ecat/master.h>`。
- 不持有 `MoEcatMaster`。
- 不直接调用 `ReadSdo()` / `WriteSdo()`。
- 不直接调用 `Service()`。
- 不访问 `mo_ecat_core/src`。
- 不直接操作 SOEM。

UI 应只依赖 bridge 提供的 Qt 类型、Qt signals/slots、Qt models 或简单 DTO。

## 3. bridge 对 UI 提供的接口

建议 bridge 对 UI 提供面向应用的接口，而不是原样暴露 core API。

示例命令：

```cpp
class EcatQtBridge : public QObject {
    Q_OBJECT

public slots:
    void initializeAdapter(const UiMasterConfig &config);
    void scan();
    void enterMaintenance();
    void prepareRun();
    void startOperation();
    void backToMaintenance();
    void stop();
    void readSdo(const SdoReadRequest &request);
    void writeSdo(const SdoWriteRequest &request);
    void dumpPdoMapping(int slaveId);

signals:
    void masterStateChanged(UiMasterState state);
    void slaveListUpdated(QVector<UiSlaveInfo> slaves);
    void logReceived(UiLogRecord record);
    void sdoReadFinished(SdoReadResult result);
    void operationFailed(QString command, QString reason);
};
```

实际类型名称可以调整，但原则是：UI 看到的是 UI 友好的类型，不直接绑定 core 内部实现。

## 4. 线程边界

推荐 `core_bridge` 使用 worker 线程：

```text
GUI thread
    | user command
    v
queued signal
    |
CoreWorker thread
    | owns MoEcatMaster
    | calls Service()
    v
mo_ecat_core
```

规则：

1. `MoEcatMaster` 在 worker 线程创建、使用和销毁。
2. UI 到 worker 的调用使用 queued connection。
3. worker 到 UI 的通知使用 signal，确保 UI 槽函数在 UI 线程执行。
4. 所有阻塞操作都在 worker 线程执行。
5. UI 主线程只做显示和用户交互，不等待 EtherCAT 操作完成。

## 5. Service 调度

bridge 应负责周期调用 `MoEcatMaster::Service()`。

可选实现：

| 方案 | 适用场景 | 说明 |
|------|----------|------|
| `QThread` + `QTimer` | UI 开发和普通验证 | 简单，容易接 Qt 信号槽 |
| `std::thread` / realtime runner | 更高实时性验证 | 需要更明确的线程同步和退出控制 |
| UI 主线程 `QTimer` | 仅临时 demo | 不推荐作为正式方案 |

第一版建议使用 `QThread` + `QTimer`，后续如需更高实时性，再把周期执行迁移到更专用的线程机制。

## 6. 数据转换边界

bridge 负责把 core 类型转换为 UI 类型：

| core 类型 | bridge/UI 类型 |
|-----------|----------------|
| `mo_ecat::MasterState` | `UiMasterState` 或 enum wrapper |
| `mo_ecat::SlaveInfo` | `UiSlaveInfo` / `SlaveTableModel` |
| `mo_ecat::SlaveFeedback` | `UiSlaveFeedback` / feedback model |
| core log callback | `UiLogRecord` / `LogModel` |
| `EcMasterConfig` | 由 `UiMasterConfig` 转换生成 |

转换原则：

- bridge 可以复制数据，UI 不直接持有 core 内部引用。
- UI model 更新应降频，避免每个 EtherCAT 周期刷新表格。
- 日志和反馈数据应有容量限制，避免长时间运行导致内存增长。

## 7. 错误处理边界

core 调用失败时，bridge 负责统一转换为 UI 可显示的错误。

建议错误来源分为：

| 来源 | 示例 | UI 处理 |
|------|------|---------|
| 状态不允许 | 当前不能 `StartOperation` | 禁用按钮或显示命令失败 |
| 总线/从站错误 | 扫描失败、状态切换失败 | 显示错误日志和当前状态 |
| SDO 超时 | 读写超时 | 面板显示失败原因 |
| bridge 错误 | worker 未启动、配置无效 | UI 弹出或状态栏提示 |

UI 不能只根据按钮状态判断安全性，必须以 core 返回值和状态回读为准。

## 8. Mock 模式边界

建议在 bridge 层抽象服务接口：

```text
IEcatService
├── RealEcatService
└── MockEcatService
```

Mock 模式提供：

- 模拟主站状态迁移。
- 模拟从站列表。
- 模拟日志输出。
- 模拟 SDO 成功/失败。
- 模拟周期反馈。

Mock 不应污染 `mo_ecat_core` 的生产接口。UI 是否连接真实 core，由应用启动配置或开发选项决定。

## 9. UI 第一版建议范围

建议第一版 UI 只实现最小闭环：

1. 网卡名输入和初始化。
2. 主站状态显示。
3. 生命周期按钮：初始化、扫描、维护、准备运行、开始、返回维护、停止。
4. 从站列表表格。
5. 日志窗口。
6. Mock 模式开关。

SDO 面板、PDO 映射、运行反馈可以作为第二阶段接入，除非 owner 明确第一版必须包含。

## 10. 不建议的做法

- UI 直接链接并调用 SOEM。
- UI 直接 include `mo_ecat_core/src` 内部头文件。
- 在按钮槽函数里同步调用 SDO 读写。
- 在 core 回调里直接更新 QLabel/QTableView。
- 让 UI 自己维护一套与 core 不一致的主站状态机。
- 为了 UI 临时需求在 core 中加入 Qt 类型。

