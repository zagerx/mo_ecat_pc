#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QVector>

namespace mo_ecat_pc
{

enum class UiMasterMode {
    kInit,
    kPrepare,
    kRun,
    kFault,
    kEmergencyStop,
    kUnknown,
};

enum class UiTransitionStage {
    kEntering,
    kStable,
    kExiting,
    kUnknown,
};

enum class UiPrepareStage {
    kNone,
    kAdapterReady,
    kTopologyDiscovered,
    kPreOpMaintenance,
    kSafeOpReady,
    kUnknown,
};

struct UiRuntimeState {
    UiMasterMode mode = UiMasterMode::kInit;
    UiTransitionStage transition = UiTransitionStage::kStable;
    UiPrepareStage prepare_stage = UiPrepareStage::kNone;
};

struct UiMasterConfig {
    QString interface_name = QStringLiteral("enp0s31f6");
    int cycle_time_us = 1000;
    bool use_dc = true;
};

struct UiSlaveInfo {
    int slave_id = 0;
    quint32 vendor_id = 0;
    quint32 product_id = 0;
    quint32 revision_id = 0;
    quint32 serial_id = 0;
    QString name;
    bool supports_dc = false;
    quint32 output_bytes = 0;
    quint32 input_bytes = 0;
};

struct UiLogRecord {
    QString level;
    QString source;
    QString message;
    QDateTime timestamp;
};

struct SdoReadRequest {
    int slave_id = 0;
    quint16 index = 0;
    quint8 subindex = 0;
    int size = 4;
    int timeout_ms = 1000;
};

struct SdoReadResult {
    bool success = false;
    int slave_id = 0;
    quint16 index = 0;
    quint8 subindex = 0;
    QByteArray data;
    QString error;
};

QString ToDisplayString(const UiRuntimeState &state);

} // namespace mo_ecat_pc

Q_DECLARE_METATYPE(mo_ecat_pc::UiRuntimeState)
Q_DECLARE_METATYPE(mo_ecat_pc::UiMasterConfig)
Q_DECLARE_METATYPE(mo_ecat_pc::UiSlaveInfo)
Q_DECLARE_METATYPE(QVector<mo_ecat_pc::UiSlaveInfo>)
Q_DECLARE_METATYPE(mo_ecat_pc::UiLogRecord)
Q_DECLARE_METATYPE(mo_ecat_pc::SdoReadRequest)
Q_DECLARE_METATYPE(mo_ecat_pc::SdoReadResult)
