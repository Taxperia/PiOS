// SPDX-License-Identifier: GPL-3.0-or-later
pragma Singleton
import QtQuick

QtObject {
    readonly property color accent: Controller.accentColor
    readonly property color accentStrong: Qt.darker(accent, 1.35)
    readonly property color darkBackground: "#091210"
    readonly property color darkSurface: "#D20D1714"
    readonly property color darkSurfaceRaised: "#EB111B18"
    readonly property color darkBorder: "#6674827B"
    readonly property color darkText: "#F5F8F6"
    readonly property color darkTextMuted: "#AEBBB5"
    readonly property color lightBackground: "#E7EFEA"
    readonly property color lightSurface: "#EAF8FBF9"
    readonly property color lightSurfaceRaised: "#FAFFFFFF"
    readonly property color lightBorder: "#554E6258"
    readonly property color lightText: "#14201B"
    readonly property color lightTextMuted: "#54645C"
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

