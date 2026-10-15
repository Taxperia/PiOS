qt_add_executable(fidan-photos main.cpp PhotoModel.cpp PhotoModel.h
    ${PROJECT_SOURCE_DIR}/src/common/AppStyle.h
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.cpp
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.h)
qt_add_qml_module(fidan-photos URI Fidan.Photos VERSION 1.0 QML_FILES qml/Photos.qml)
target_link_libraries(fidan-photos PRIVATE Qt6::Core Qt6::Gui Qt6::Concurrent Qt6::Qml Qt6::Quick Qt6::QuickControls2)
install(TARGETS fidan-photos RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
