// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QColor>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

// Shared palette for standalone Fidan applications. Preferences are refreshed
// while a window is open, so Settings changes apply without restarting apps.
class AppStyle final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
public:
    explicit AppStyle(QObject *parent = nullptr) : QObject(parent) {
        refresh();
        auto *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &AppStyle::refresh);
        timer->start(500);
    }
    bool dark() const { return m_dark; }
    QColor accent() const { return m_accent; }
    Q_INVOKABLE QColor color(const QString &value, bool dark) const {
        const auto key = value.toUpper();
        if (key == "#63DE8A" || key == "#63DE87" || key == "#2BA85B") return dark ? m_accent : m_accent.darker(150);
        if (key == "#28533E" || key == "#315F46" || key == "#397D58") return dark ? m_accent.darker(220) : m_accent.lighter(150);
        if (dark) return QColor(value);
        static const QHash<QString, QString> light {
            {"#0E1715", "#EFF5F1"}, {"#0B1412", "#F8FBF9"}, {"#101A17", "#E5EEE8"},
            {"#111C19", "#EAF1ED"}, {"#17221F", "#FFFFFF"}, {"#17231F", "#F4F8F5"},
            {"#15211D", "#F7FAF8"}, {"#1A2823", "#DFEAE3"}, {"#1B2421", "#EEF4F0"},
            {"#1B2924", "#DCE9E1"}, {"#23332D", "#DBE9E0"}, {"#263F30", "#C7E5D3"},
            {"#304039", "#CEE1D5"}, {"#315F46", "#BDDFC9"}, {"#28533E", "#C7E5D3"},
            {"#397D58", "#ACD8BC"}, {"#2A3933", "#DDEAE2"}, {"#28543D", "#C7E5D3"},
            {"#465750", "#B4C8BC"}, {"#2B3A35", "#CFDDD4"}, {"#34473F", "#A4BCAF"},
            {"#43554C", "#AFC6B7"}, {"#F2F7F4", "#18261F"}, {"#9FB2A8", "#586B60"},
            {"#17251F", "#F4F8F5"}, {"#1A2A24", "#DFEAE3"}, {"#20322B", "#DBE9E0"},
            {"#30483D", "#B4C8BC"}, {"#2B4136", "#CFDDD4"},
            {"#63DE8A", "#207243"}, {"#71847A", "#60756A"}, {"#68776F", "#6E8075"},
            {"#709682", "#4E7560"}, {"#FF9292", "#B52F3B"}, {"#347551", "#76A889"}
        };
        return QColor(light.value(value.toUpper(), value));
    }
signals:
    void changed();
private:
    void refresh() {
        QSettings settings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
            + QStringLiteral("/fidan/shell.conf"), QSettings::IniFormat);
        const bool value = settings.value(QStringLiteral("appearance/darkTheme"), true).toBool();
        QColor accent(settings.value("appearance/accentColor", "#63de8a").toString());
        if (!accent.isValid()) accent = QColor("#63de8a");
        if (value != m_dark || accent != m_accent) { m_dark = value; m_accent = accent; emit changed(); }
    }
    bool m_dark = true;
    QColor m_accent = QColor("#63de8a");
};
