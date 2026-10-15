cmake_minimum_required(VERSION 3.25)

project(FidanDesktop
    VERSION 0.1.0
    DESCRIPTION "Project Fidan lightweight Wayland desktop shell"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CTest)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Concurrent
    Gui
    Qml
    Quick
    QuickControls2
    DBus
    Network
    LinguistTools
)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(src/shell)
add_subdirectory(src/firstboot)
add_subdirectory(src/greeter)
add_subdirectory(src/store)
add_subdirectory(src/taskmanager)
add_subdirectory(src/settings)
add_subdirectory(src/files)
add_subdirectory(src/photos)

if(BUILD_TESTING)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

install(FILES config/product.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/fidan
)
install(FILES artwork/wallpapers/fidan-mountain-dawn-4k.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/backgrounds/fidan
)
install(FILES artwork/ASSET-LICENSES.md artwork/wallpapers/fidan-mountain-dawn.prompt.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/fidan-artwork
)
install(DIRECTORY data/labwc/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/fidan/labwc
)
install(DIRECTORY data/icons/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    FILES_MATCHING PATTERN "*.svg"
)
install(FILES
    data/applications/org.fidan.Shell.desktop
    data/applications/org.fidan.FirstBoot.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
install(FILES
    data/systemd/user/fidan-session.target
    data/systemd/user/fidan-shell.service
    DESTINATION lib/systemd/user
)
install(FILES data/wayland-sessions/fidan.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/wayland-sessions
)
install(PROGRAMS scripts/fidan-session
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(PROGRAMS scripts/fidan-shellctl scripts/fidan-workspace scripts/fidan-lock scripts/fidan-screenshot scripts/fidan-display-restore scripts/fidan-night-light
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(PROGRAMS scripts/fidan-update
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES data/greetd/config.toml
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/greetd
    RENAME fidan.toml
)
install(FILES data/gtklock/style.css
    DESTINATION ${CMAKE_INSTALL_DATADIR}/fidan/gtklock
)
install(FILES data/fidan-mimeapps.list DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg)
install(FILES data/qt6ct/qt6ct.conf
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/qt6ct
)
install(FILES data/themes/fidan-dark.conf data/themes/fidan.qss
    DESTINATION ${CMAKE_INSTALL_DATADIR}/fidan/themes
)
install(FILES data/qterminal/qterminal.ini
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/qterminal.org
)
install(FILES data/pcmanfm-qt/settings.conf
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/pcmanfm-qt/default
)
install(FILES data/polkit/49-fidan-greeter.rules
    DESTINATION ${CMAKE_INSTALL_DATADIR}/polkit-1/rules.d
)
install(FILES data/store/catalog.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/fidan/store
)
install(FILES data/xdg-desktop-portal/fidan-portals.conf
    DESTINATION ${CMAKE_INSTALL_DATADIR}/xdg-desktop-portal
)
install(FILES
    data/applications/org.fidan.Store.desktop
    data/applications/org.fidan.TaskManager.desktop
    data/applications/org.fidan.Settings.desktop
    data/applications/org.fidan.Files.desktop
    data/applications/org.fidan.Photos.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
install(FILES
    data/icons/org.fidan.Store.svg
    data/icons/org.fidan.TaskManager.svg
    data/icons/org.fidan.Files.svg
    data/icons/org.fidan.Settings.svg
    data/icons/org.fidan.Photos.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)
install(FILES
    data/systemd/user/fidan-update-check.service
    data/systemd/user/fidan-update-check.timer
    DESTINATION lib/systemd/user
)

install(DIRECTORY artwork/wallpapers/collection DESTINATION ${CMAKE_INSTALL_DATADIR}/backgrounds/fidan)
install(FILES docs/daily-desktop.md docs/photos-display.md docs/settings-redesign.md DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/fidan-shell)
