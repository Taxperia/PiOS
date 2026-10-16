qt_add_executable(fidan-apps main.cpp UtilityModels.cpp UtilityModels.h
    ${PROJECT_SOURCE_DIR}/src/common/AppStyle.h
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.cpp
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.h)
qt_add_qml_module(fidan-apps URI Fidan.Apps VERSION 1.0 QML_FILES
    qml/Main.qml qml/AppButton.qml qml/Calculator.qml qml/Clock.qml qml/Calendar.qml qml/PdfViewer.qml qml/SafePdfMultiPageView.qml)
target_link_libraries(fidan-apps PRIVATE Qt6::Core Qt6::Gui Qt6::DBus Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Pdf)
install(TARGETS fidan-apps RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
foreach(application Pdf Calculator Clock Calendar)
    install(FILES ${PROJECT_SOURCE_DIR}/data/applications/org.fidan.${application}.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES ${PROJECT_SOURCE_DIR}/data/icons/org.fidan.${application}.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endforeach()
