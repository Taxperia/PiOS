qt_add_executable(fidan-unit-tests
    tst_core.cpp
    ${PROJECT_SOURCE_DIR}/src/shell/ProductInfo.cpp
    ${PROJECT_SOURCE_DIR}/src/shell/ProductInfo.h
    ${PROJECT_SOURCE_DIR}/src/shell/SystemMonitor.cpp
    ${PROJECT_SOURCE_DIR}/src/shell/SystemMonitor.h
    ${PROJECT_SOURCE_DIR}/src/shell/NotificationService.cpp
    ${PROJECT_SOURCE_DIR}/src/shell/NotificationService.h
    ${PROJECT_SOURCE_DIR}/src/shell/ShellController.cpp
    ${PROJECT_SOURCE_DIR}/src/shell/ShellController.h
    ${PROJECT_SOURCE_DIR}/src/shell/AppModel.cpp
    ${PROJECT_SOURCE_DIR}/src/shell/AppModel.h
    ${PROJECT_SOURCE_DIR}/src/shell/DesktopModel.cpp
    ${PROJECT_SOURCE_DIR}/src/shell/DesktopModel.h
    ${PROJECT_SOURCE_DIR}/src/store/StoreModel.cpp
    ${PROJECT_SOURCE_DIR}/src/store/StoreModel.h
    ${PROJECT_SOURCE_DIR}/src/taskmanager/ProcessModel.cpp
    ${PROJECT_SOURCE_DIR}/src/taskmanager/ProcessModel.h
    ${PROJECT_SOURCE_DIR}/src/files/FilesModel.cpp
    ${PROJECT_SOURCE_DIR}/src/files/FilesModel.h
)
target_include_directories(fidan-unit-tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/shell
    ${PROJECT_SOURCE_DIR}/src/store
    ${PROJECT_SOURCE_DIR}/src/taskmanager
    ${PROJECT_SOURCE_DIR}/src/files
)
target_compile_definitions(fidan-unit-tests PRIVATE
    FIDAN_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    FIDAN_VERSION="${PROJECT_VERSION}"
)
target_link_libraries(fidan-unit-tests PRIVATE Qt6::Concurrent Qt6::Core Qt6::Gui Qt6::DBus Qt6::Test)
add_test(NAME fidan-unit-tests COMMAND fidan-unit-tests)
set_tests_properties(fidan-unit-tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(fidan-files-ui-tests
    tst_files_ui.cpp
    ${PROJECT_SOURCE_DIR}/src/files/FilesModel.cpp
    ${PROJECT_SOURCE_DIR}/src/files/FilesModel.h
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.cpp
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.h
)
target_include_directories(fidan-files-ui-tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/files ${PROJECT_SOURCE_DIR}/src/common
)
target_compile_definitions(fidan-files-ui-tests PRIVATE FIDAN_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_link_libraries(fidan-files-ui-tests PRIVATE
    Qt6::Concurrent Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Test
)
add_test(NAME fidan-files-ui-tests COMMAND fidan-files-ui-tests)
set_tests_properties(fidan-files-ui-tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")


target_sources(fidan-files-ui-tests PRIVATE ${PROJECT_SOURCE_DIR}/src/common/AppStyle.h)

qt_add_executable(fidan-photos-display-tests
    tst_photos_display.cpp
    ${PROJECT_SOURCE_DIR}/src/photos/PhotoModel.cpp
    ${PROJECT_SOURCE_DIR}/src/photos/PhotoModel.h
    ${PROJECT_SOURCE_DIR}/src/settings/DisplayController.cpp
    ${PROJECT_SOURCE_DIR}/src/settings/DisplayController.h
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.cpp
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.h
    ${PROJECT_SOURCE_DIR}/src/common/AppStyle.h
)
target_include_directories(fidan-photos-display-tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/photos ${PROJECT_SOURCE_DIR}/src/settings ${PROJECT_SOURCE_DIR}/src/common)
target_compile_definitions(fidan-photos-display-tests PRIVATE FIDAN_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_link_libraries(fidan-photos-display-tests PRIVATE
    Qt6::Concurrent Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Test)
add_test(NAME fidan-photos-display-tests COMMAND fidan-photos-display-tests)
set_tests_properties(fidan-photos-display-tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")

find_program(FIDAN_PYTHON python3 REQUIRED)
add_test(NAME fidan-display-restore-tests COMMAND ${FIDAN_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/test_display_restore.py)

qt_add_executable(fidan-settings-tests
    tst_settings.cpp
    ${PROJECT_SOURCE_DIR}/src/settings/SettingsData.cpp
    ${PROJECT_SOURCE_DIR}/src/settings/SettingsData.h
    ${PROJECT_SOURCE_DIR}/src/settings/DisplayController.cpp
    ${PROJECT_SOURCE_DIR}/src/settings/DisplayController.h
    ${PROJECT_SOURCE_DIR}/src/settings/UpdateController.cpp
    ${PROJECT_SOURCE_DIR}/src/settings/UpdateController.h
    ${PROJECT_SOURCE_DIR}/src/shell/ShellController.cpp
    ${PROJECT_SOURCE_DIR}/src/shell/ShellController.h
    ${PROJECT_SOURCE_DIR}/src/shell/ProductInfo.cpp
    ${PROJECT_SOURCE_DIR}/src/shell/ProductInfo.h
    ${PROJECT_SOURCE_DIR}/src/shell/SystemMonitor.cpp
    ${PROJECT_SOURCE_DIR}/src/shell/SystemMonitor.h
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.cpp
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.h)
target_include_directories(fidan-settings-tests PRIVATE ${PROJECT_SOURCE_DIR}/src/settings ${PROJECT_SOURCE_DIR}/src/shell ${PROJECT_SOURCE_DIR}/src/common)
target_compile_definitions(fidan-settings-tests PRIVATE FIDAN_SOURCE_DIR="${PROJECT_SOURCE_DIR}" FIDAN_VERSION="${PROJECT_VERSION}")
target_link_libraries(fidan-settings-tests PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::DBus Qt6::Test)
add_test(NAME fidan-settings-tests COMMAND fidan-settings-tests)
set_tests_properties(fidan-settings-tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
add_test(NAME fidan-lock-preferences-tests COMMAND ${FIDAN_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/test_lock_preferences.py)
add_test(NAME fidan-updater-tests COMMAND ${FIDAN_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/test_updater.py)
