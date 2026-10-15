// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>

class LayerShellBridge final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)

public:
    explicit LayerShellBridge(bool enabled, QObject *parent = nullptr);
    bool available() const;

    Q_INVOKABLE void configure(QObject *windowObject, const QString &role);
    Q_INVOKABLE void positionDock(QObject *windowObject, const QString &position);

private:
    bool m_enabled = false;
};

