// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

ScrollView {
    id: root
    clip: true
    contentWidth: availableWidth
    // Keep wheel input usable over nested display cards and horizontal monitor
    // previews. Trackpads retain pixel precision; ordinary wheels use steps.
    WheelHandler {
        target: null
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: function(event) {
            const view = root.contentItem
            const delta = event.pixelDelta.y || event.angleDelta.y / 3
            if (!view || !delta || view.contentHeight <= view.height) return
            view.contentY = Math.max(0, Math.min(view.contentHeight - view.height, view.contentY - delta))
            event.accepted = true
        }
    }
}
