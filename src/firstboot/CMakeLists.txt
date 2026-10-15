qt_add_executable(fidan-firstbootd
    FirstBootService.cpp
    FirstBootService.h
    daemon-main.cpp
)
target_link_libraries(fidan-firstbootd PRIVATE Qt6::Core Qt6::DBus)

qt_add_executable(fidan-firstboot
    FirstBootClient.cpp
    FirstBootClient.h
    client-main.cpp
)
qt_add_qml_module(fidan-firstboot
    URI Fidan.FirstBoot
    VERSION 1.0
    QML_FILES qml/FirstBoot.qml
)
qt_add_translations(fidan-firstboot
    TS_FILES ${PROJECT_SOURCE_DIR}/translations/fidan-firstboot_tr.ts
    RESOURCE_PREFIX /i18n
)
target_link_libraries(fidan-firstboot PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::DBus)

install(TARGETS fidan-firstboot fidan-firstbootd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES
    ${PROJECT_SOURCE_DIR}/data/dbus/org.fidan.FirstBoot1.conf
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/dbus-1/system.d
)
install(FILES
    ${PROJECT_SOURCE_DIR}/data/systemd/system/fidan-firstbootd.service
    ${PROJECT_SOURCE_DIR}/data/systemd/system/fidan-firstboot-session.service
    DESTINATION lib/systemd/system
)

