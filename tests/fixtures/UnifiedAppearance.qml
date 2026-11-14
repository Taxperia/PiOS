import QtQml
QtObject {
    property bool darkTheme: true
    property string themeMode: "dark"
    onThemeModeChanged: darkTheme = themeMode !== "light"
    property string accentColor: "#63de8a"
    property string dockPosition: "left"
    property string panelPosition: "top"
    property string panelStyle: "detailed"
    property string dateFormat: "locale"
    property string sessionLanguage: "tr_TR.UTF-8"
    property string keyboardLayout: "tr"
    property bool reducedMotion: false
}
