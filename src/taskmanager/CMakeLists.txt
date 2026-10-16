qt_add_executable(fidan-task-manager
    main.cpp
    ProcessModel.cpp
    ProcessModel.h
    ../common/ThemeIconProvider.cpp
    ../common/ThemeIconProvider.h
)

qt_add_qml_module(fidan-task-manager
    URI Fidan.TaskManager
    VERSION 1.0
    QML_FILES qml/TaskManager.qml
)

target_link_libraries(fidan-task-manager PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
)

install(TARGETS fidan-task-manager RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

target_sources(fidan-task-manager PRIVATE ${PROJECT_SOURCE_DIR}/src/common/AppStyle.h)
