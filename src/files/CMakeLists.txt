qt_add_executable(fidan-files main.cpp FilesModel.cpp FilesModel.h
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.cpp
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.h)
target_include_directories(fidan-files PRIVATE ${PROJECT_SOURCE_DIR}/src/common)
qt_add_qml_module(fidan-files URI Fidan.Files VERSION 1.0 QML_FILES qml/Files.qml)
target_link_libraries(fidan-files PRIVATE Qt6::Concurrent Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)
install(TARGETS fidan-files RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

target_sources(fidan-files PRIVATE ${PROJECT_SOURCE_DIR}/src/common/AppStyle.h)
