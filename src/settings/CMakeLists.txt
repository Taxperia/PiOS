qt_add_executable(fidan-settings
    main.cpp
    ${PROJECT_SOURCE_DIR}/src/settings/SettingsData.cpp
    ${PROJECT_SOURCE_DIR}/src/settings/SettingsData.h
    UpdateController.cpp
    UpdateController.h
    DisplayController.cpp
    DisplayController.h
    ${PROJECT_SOURCE_DIR}/src/shell/ShellController.cpp
    ${PROJECT_SOURCE_DIR}/src/shell/ShellController.h
    ${PROJECT_SOURCE_DIR}/src/shell/SystemMonitor.cpp
    ${PROJECT_SOURCE_DIR}/src/shell/SystemMonitor.h
    ${PROJECT_SOURCE_DIR}/src/shell/ProductInfo.cpp
    ${PROJECT_SOURCE_DIR}/src/shell/ProductInfo.h
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.cpp
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.h
)

target_include_directories(fidan-settings PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/shell
    ${PROJECT_SOURCE_DIR}/src/common
)

set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/SettingsContent.qml PROPERTIES QT_RESOURCE_ALIAS qml/SettingsContent.qml)
set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/DisplayPage.qml PROPERTIES QT_RESOURCE_ALIAS qml/DisplayPage.qml)
set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/SettingsScrollView.qml PROPERTIES QT_RESOURCE_ALIAS qml/SettingsScrollView.qml)
set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/SettingsControls.qml PROPERTIES QT_RESOURCE_ALIAS qml/SettingsControls.qml)
set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/SettingsCombo.qml PROPERTIES QT_RESOURCE_ALIAS qml/SettingsCombo.qml)
set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/PersonalizationPage.qml PROPERTIES QT_RESOURCE_ALIAS qml/PersonalizationPage.qml)
set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/MachineCard.qml PROPERTIES QT_RESOURCE_ALIAS qml/MachineCard.qml)
set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/SystemPages.qml PROPERTIES QT_RESOURCE_ALIAS qml/SystemPages.qml)
set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/NetworkPage.qml PROPERTIES QT_RESOURCE_ALIAS qml/NetworkPage.qml)
set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/OtherSettingsPage.qml PROPERTIES QT_RESOURCE_ALIAS qml/OtherSettingsPage.qml)

qt_add_qml_module(fidan-settings
    URI Fidan.Settings
    VERSION 1.0
    QML_FILES
        ${PROJECT_SOURCE_DIR}/src/common/qml/SettingsContent.qml qml/Settings.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/DisplayPage.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/SettingsScrollView.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/SettingsControls.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/SettingsCombo.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/PersonalizationPage.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/MachineCard.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/SystemPages.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/NetworkPage.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/OtherSettingsPage.qml
)

target_compile_definitions(fidan-settings PRIVATE
    FIDAN_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    FIDAN_VERSION="${PROJECT_VERSION}"
)
target_link_libraries(fidan-settings PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::DBus
)

install(TARGETS fidan-settings RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/NetworkSettings.qml PROPERTIES QT_RESOURCE_ALIAS qml/NetworkSettings.qml)

set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/DeviceSettings.qml PROPERTIES QT_RESOURCE_ALIAS qml/DeviceSettings.qml)

set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/ApplicationsSettings.qml PROPERTIES QT_RESOURCE_ALIAS qml/ApplicationsSettings.qml)

set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/PrivacySettings.qml PROPERTIES QT_RESOURCE_ALIAS qml/PrivacySettings.qml)

qt_target_qml_sources(fidan-settings QML_FILES
    ${PROJECT_SOURCE_DIR}/src/common/qml/NetworkSettings.qml
    ${PROJECT_SOURCE_DIR}/src/common/qml/DeviceSettings.qml
    ${PROJECT_SOURCE_DIR}/src/common/qml/ApplicationsSettings.qml
    ${PROJECT_SOURCE_DIR}/src/common/qml/PrivacySettings.qml
)

target_sources(fidan-settings PRIVATE ${PROJECT_SOURCE_DIR}/src/settings/DesktopSettings.cpp ${PROJECT_SOURCE_DIR}/src/settings/DesktopSettings.h)
target_link_libraries(fidan-settings PRIVATE Qt6::Concurrent Qt6::Network)
