qt_add_executable(fidan-greeter
    GreeterController.cpp
    GreeterController.h
    main.cpp
)
qt_add_qml_module(fidan-greeter
    URI Fidan.Greeter
    VERSION 1.0
    QML_FILES qml/Greeter.qml
)
qt_add_translations(fidan-greeter
    TS_FILES ${PROJECT_SOURCE_DIR}/translations/fidan-greeter_tr.ts
    RESOURCE_PREFIX /i18n
)
target_link_libraries(fidan-greeter PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Network)
install(TARGETS fidan-greeter RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

