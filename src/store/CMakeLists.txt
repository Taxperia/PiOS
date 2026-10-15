qt_add_executable(fidan-store
    main.cpp
    StoreModel.cpp
    StoreModel.h
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.cpp
    ${PROJECT_SOURCE_DIR}/src/common/ThemeIconProvider.h
)

target_include_directories(fidan-store PRIVATE ${PROJECT_SOURCE_DIR}/src/common)

set_source_files_properties(${PROJECT_SOURCE_DIR}/src/shell/qml/components/PiBoard.qml PROPERTIES QT_RESOURCE_ALIAS qml/PiBoard.qml)
qt_add_qml_module(fidan-store
    URI Fidan.Store
    VERSION 1.0
    QML_FILES qml/Store.qml ${PROJECT_SOURCE_DIR}/src/shell/qml/components/PiBoard.qml
)

target_compile_definitions(fidan-store PRIVATE FIDAN_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_link_libraries(fidan-store PRIVATE
    Qt6::Concurrent Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
)

install(TARGETS fidan-store RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

target_sources(fidan-store PRIVATE ${PROJECT_SOURCE_DIR}/src/common/AppStyle.h)
