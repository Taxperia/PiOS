// SPDX-License-Identifier: GPL-3.0-or-later
#include "LayerShellBridge.h"

#include <QMargins>
#include <QWindow>

#include <initializer_list>

#ifdef FIDAN_HAS_LAYER_SHELL
#include <LayerShellQt/Window>
#endif

LayerShellBridge::LayerShellBridge(bool enabled, QObject *parent)
    : QObject(parent)
    , m_enabled(enabled)
{
}

bool LayerShellBridge::available() const
{
#ifdef FIDAN_HAS_LAYER_SHELL
    return m_enabled;
#else
    return false;
#endif
}

void LayerShellBridge::configure(QObject *windowObject, const QString &role)
{
    QWindow *window = qobject_cast<QWindow *>(windowObject);
    if (!window) {
        return;
    }

    if (!available()) {
        window->setFlags(window->flags() | Qt::FramelessWindowHint);
        return;
    }

#ifdef FIDAN_HAS_LAYER_SHELL
    using LayerWindow = LayerShellQt::Window;
    const auto anchors = [](std::initializer_list<LayerWindow::Anchor> values) {
        LayerWindow::Anchors result;
        for (const LayerWindow::Anchor value : values) {
            result.setFlag(value);
        }
        return result;
    };

    LayerWindow *surface = LayerWindow::get(window);
    surface->setScope(QStringLiteral("fidan-") + role);
    surface->setKeyboardInteractivity(LayerWindow::KeyboardInteractivityNone);

    if (role == QStringLiteral("startup")) {
        surface->setLayer(LayerWindow::LayerOverlay);
        surface->setAnchors(anchors({LayerWindow::AnchorTop, LayerWindow::AnchorBottom,
            LayerWindow::AnchorLeft, LayerWindow::AnchorRight}));
        surface->setExclusiveZone(-1);
        surface->setKeyboardInteractivity(LayerWindow::KeyboardInteractivityExclusive);
    } else if (role == QStringLiteral("background")) {
        surface->setLayer(LayerWindow::LayerBackground);
        surface->setAnchors(anchors({LayerWindow::AnchorTop, LayerWindow::AnchorBottom,
                                     LayerWindow::AnchorLeft, LayerWindow::AnchorRight}));
        surface->setExclusiveZone(-1);
        surface->setKeyboardInteractivity(LayerWindow::KeyboardInteractivityOnDemand);
    } else if (role == QStringLiteral("panel")) {
        surface->setLayer(LayerWindow::LayerTop);
        surface->setAnchors(anchors({LayerWindow::AnchorTop, LayerWindow::AnchorLeft,
                                     LayerWindow::AnchorRight}));
        surface->setExclusiveZone(window->height());
        connect(window, &QWindow::heightChanged, surface, [surface](int height) { surface->setExclusiveZone(height); });
    } else if (role == QStringLiteral("dock")) {
        surface->setLayer(LayerWindow::LayerTop);
        positionDock(window, "left");
        connect(window, &QWindow::widthChanged, surface, [surface,window](int) { surface->setExclusiveZone(surface->anchors().testFlag(LayerWindow::AnchorBottom) ? window->height()+20 : window->width()+28); });
        connect(window, &QWindow::heightChanged, surface, [surface,window](int) { surface->setExclusiveZone(surface->anchors().testFlag(LayerWindow::AnchorBottom) ? window->height()+20 : window->width()+28); });
    } else if (role == QStringLiteral("quick-settings")) {
        surface->setLayer(LayerWindow::LayerOverlay);
        surface->setAnchors(anchors({LayerWindow::AnchorTop, LayerWindow::AnchorRight}));
        // Zone 0 uses the work area: the panel's exclusive height is already
        // excluded by the compositor. Add only the gap below that panel.
        surface->setMargins(QMargins(0, 4, 12, 0));
        surface->setExclusiveZone(0);
        surface->setKeyboardInteractivity(LayerWindow::KeyboardInteractivityOnDemand);
    } else if (role == QStringLiteral("notification")) {
        surface->setLayer(LayerWindow::LayerOverlay);
        surface->setAnchors(anchors({LayerWindow::AnchorBottom, LayerWindow::AnchorRight}));
        surface->setMargins(QMargins(0, 0, 18, 18));
        surface->setExclusiveZone(0);
    } else if (role == QStringLiteral("notification-center")) {
        surface->setLayer(LayerWindow::LayerOverlay);
        surface->setAnchors(anchors({LayerWindow::AnchorTop, LayerWindow::AnchorRight}));
        surface->setMargins(QMargins(0, 4, 12, 0));
        surface->setExclusiveZone(0);
        surface->setKeyboardInteractivity(LayerWindow::KeyboardInteractivityOnDemand);
    } else if (role == QStringLiteral("launcher")) {
        surface->setLayer(LayerWindow::LayerOverlay);
        surface->setAnchors(LayerWindow::Anchors{});
        surface->setExclusiveZone(0);
        surface->setKeyboardInteractivity(LayerWindow::KeyboardInteractivityExclusive);
    }
#else
    Q_UNUSED(role)
#endif
}

void LayerShellBridge::positionDock(QObject *object, const QString &position) {
    auto *window = qobject_cast<QWindow *>(object); if (!window || !available()) return;
#ifdef FIDAN_HAS_LAYER_SHELL
    using LayerWindow = LayerShellQt::Window;
    auto *surface = LayerWindow::get(window);
    surface->setAnchors(position == "bottom" ? LayerWindow::AnchorBottom : position == "right" ? LayerWindow::AnchorRight : LayerWindow::AnchorLeft);
    surface->setMargins(position == "bottom" ? QMargins(0,0,0,10) : position == "right" ? QMargins(0,70,14,0) : QMargins(14,70,0,0));
    surface->setExclusiveZone(position == "bottom" ? window->height()+20 : window->width()+28);
#else
    Q_UNUSED(position)
#endif
}

