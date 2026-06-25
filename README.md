# mo_ecat_pc

Qt6-based EtherCAT master upper-computer application.

## Project Layout

- `app/`: Qt application, including GUI, application controller, and `core_bridge`.
- `app/core_bridge/`: boundary layer that owns and schedules `mo_ecat_core`.
- `app/gui/`: UI widgets and user-facing models/signals.
- `docs/`: project architecture and interface-boundary documents.
- `mo_ecat_core/`: EtherCAT master core library submodule.

## Build

```bash
cmake -S . -B build
cmake --build build -j$(nproc)
```

The Qt application target is `mo_ecat_pc_app`.
