set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(fidan-shell
    main.cpp
    ${PROJECT_SOURCE_DIR}/src/settings/SettingsData.cpp
    ${PROJECT_SOURCE_DIR}/src/settings/SettingsData.h
    ${PROJECT_SOURCE_DIR}/src/settings/UpdateController.cpp
    ${PROJECT_SOURCE_DIR}/src/settings/UpdateController.h
    ${PROJECT_SOURCE_DIR}/src/settings/DisplayController.cpp
    ${PROJECT_SOURCE_DIR}/src/settings/DisplayController.h
    AppModel.cpp
    AppModel.h
    DesktopModel.cpp
    DesktopModel.h
    ${PROJECT_SOURCE_DIR}/src/files/FilesModel.cpp
    ${PROJECT_SOURCE_DIR}/src/files/FilesModel.h
    LayerShellBridge.cpp
    LayerShellBridge.h
    NotificationService.cpp
    NotificationService.h
    ProductInfo.cpp
    ProductInfo.h
    ShellController.cpp
    ShellController.h
    SystemMonitor.cpp
    SystemMonitor.h
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.cpp
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.h
)

target_include_directories(fidan-shell PRIVATE ${PROJECT_SOURCE_DIR}/src/common ${PROJECT_SOURCE_DIR}/src/settings)

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

qt_add_qml_module(fidan-shell
    URI Fidan.Shell
    VERSION 1.0
    QML_FILES
        ${PROJECT_SOURCE_DIR}/src/common/qml/SettingsContent.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/DisplayPage.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/SettingsScrollView.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/SettingsControls.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/SettingsCombo.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/PersonalizationPage.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/MachineCard.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/SystemPages.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/NetworkPage.qml
        ${PROJECT_SOURCE_DIR}/src/common/qml/OtherSettingsPage.qml
        qml/Main.qml
        qml/Theme.qml
        qml/components/AppLauncher.qml
        qml/components/DesktopBackground.qml
        qml/components/DesktopContextMenu.qml
        qml/components/DesktopIcons.qml
        qml/components/Dock.qml
        qml/components/GlassPanel.qml
        qml/components/IconButton.qml
        qml/components/FirstBootPreview.qml
        qml/components/LockScreenPreview.qml
        qml/components/MetricBar.qml
        qml/components/PiBoard.qml
        qml/components/NotificationToast.qml
        qml/components/NotificationCenter.qml
        qml/components/QuickSettings.qml
        qml/components/SettingsWindow.qml
        qml/components/SoftwareCenterWindow.qml
        qml/components/StatusCard.qml
        qml/components/SystemIcon.qml
        qml/components/TopBar.qml
        qml/surfaces/StartupSurface.qml
        qml/surfaces/DesktopSurface.qml
        qml/surfaces/DockSurface.qml
        qml/surfaces/LauncherSurface.qml
        qml/surfaces/NotificationSurface.qml
        qml/surfaces/NotificationCenterSurface.qml
        qml/surfaces/PanelSurface.qml
        qml/surfaces/PreviewWindow.qml
        qml/surfaces/QuickSettingsSurface.qml
)

qt_add_translations(fidan-shell
    TS_FILES ${PROJECT_SOURCE_DIR}/translations/fidan-shell_tr.ts
    RESOURCE_PREFIX /i18n
)

find_package(LayerShellQt QUIET)
if(LayerShellQt_FOUND)
    target_compile_definitions(fidan-shell PRIVATE FIDAN_HAS_LAYER_SHELL=1)
    target_link_libraries(fidan-shell PRIVATE LayerShellQt::Interface)
endif()

target_compile_definitions(fidan-shell PRIVATE
    FIDAN_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    FIDAN_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(fidan-shell PRIVATE
    Qt6::Core
    Qt6::Concurrent
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::DBus
    Qt6::Network
)

install(TARGETS fidan-shell
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/NetworkSettings.qml PROPERTIES QT_RESOURCE_ALIAS qml/NetworkSettings.qml)

set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/DeviceSettings.qml PROPERTIES QT_RESOURCE_ALIAS qml/DeviceSettings.qml)

set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/ApplicationsSettings.qml PROPERTIES QT_RESOURCE_ALIAS qml/ApplicationsSettings.qml)

set_source_files_properties(${PROJECT_SOURCE_DIR}/src/common/qml/PrivacySettings.qml PROPERTIES QT_RESOURCE_ALIAS qml/PrivacySettings.qml)

qt_target_qml_sources(fidan-shell QML_FILES
    ${PROJECT_SOURCE_DIR}/src/common/qml/NetworkSettings.qml
    ${PROJECT_SOURCE_DIR}/src/common/qml/DeviceSettings.qml
    ${PROJECT_SOURCE_DIR}/src/common/qml/ApplicationsSettings.qml
    ${PROJECT_SOURCE_DIR}/src/common/qml/PrivacySettings.qml
)

target_sources(fidan-shell PRIVATE ${PROJECT_SOURCE_DIR}/src/settings/DesktopSettings.cpp ${PROJECT_SOURCE_DIR}/src/settings/DesktopSettings.h)
target_link_libraries(fidan-shell PRIVATE Qt6::Concurrent Qt6::Network)
