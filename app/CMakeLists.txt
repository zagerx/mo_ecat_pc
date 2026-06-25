find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(CMAKE_AUTOMOC ON)

add_executable(mo_ecat_pc_app
    main.cpp
    app_controller/app_controller.cpp
    app_controller/app_controller.h
    core_bridge/ecat_qt_bridge.cpp
    core_bridge/ecat_qt_bridge.h
    core_bridge/ecat_types.h
    core_bridge/ecat_worker.cpp
    core_bridge/ecat_worker.h
    gui/main_window.cpp
    gui/main_window.h
)

target_include_directories(mo_ecat_pc_app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mo_ecat_pc_app
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        mo_ecat_core
)

