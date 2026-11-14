// SPDX-License-Identifier: GPL-3.0-or-later
pragma Singleton
import QtQuick

QtObject {
    readonly property color accent: Controller.accentColor
    readonly property color accentStrong: Qt.darker(accent, 1.35)
    readonly property color darkBackground: Controller.themeColor("#091210", Controller.themeMode, Controller.darkTheme)
    readonly property color darkSurface: Controller.themeColor("#D20D1714", Controller.themeMode, Controller.darkTheme)
    readonly property color darkSurfaceRaised: Controller.themeColor("#EB111B18", Controller.themeMode, Controller.darkTheme)
    readonly property color darkBorder: Controller.themeColor("#6674827B", Controller.themeMode, Controller.darkTheme)
    readonly property color darkText: Controller.themeColor("#F5F8F6", Controller.themeMode, Controller.darkTheme)
    readonly property color darkTextMuted: Controller.themeColor("#AEBBB5", Controller.themeMode, Controller.darkTheme)
    readonly property color lightBackground: Controller.themeColor("#E7EFEA", Controller.themeMode, Controller.darkTheme)
    readonly property color lightSurface: Controller.themeColor("#EAF8FBF9", Controller.themeMode, Controller.darkTheme)
    readonly property color lightSurfaceRaised: Controller.themeColor("#FAFFFFFF", Controller.themeMode, Controller.darkTheme)
    readonly property color lightBorder: Controller.themeColor("#554E6258", Controller.themeMode, Controller.darkTheme)
    readonly property color lightText: Controller.themeColor("#14201B", Controller.themeMode, Controller.darkTheme)
    readonly property color lightTextMuted: Controller.themeColor("#54645C", Controller.themeMode, Controller.darkTheme)
    readonly property int radiusSmall: 8
    readonly property int radiusMedium: 12
    readonly property int radiusLarge: 18
    readonly property int panelHeight: 50
    readonly property int motionFast: Controller.reducedMotion ? 0 : 120
    readonly property int motionNormal: Controller.reducedMotion ? 0 : 200
    readonly property string fontFamily: "Inter"

    function surface(dark) { return dark ? darkSurface : lightSurface }
    function raised(dark) { return dark ? darkSurfaceRaised : lightSurfaceRaised }
    function border(dark) { return dark ? darkBorder : lightBorder }
    function text(dark) { return dark ? darkText : lightText }
    function muted(dark) { return dark ? darkTextMuted : lightTextMuted }
}

