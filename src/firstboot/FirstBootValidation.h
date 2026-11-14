// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QRegularExpression>
#include <QString>
#include <QVariantMap>

namespace FirstBootValidation {
inline bool desktopPreferences(const QVariantMap &values)
{
    const QMap<QString,QStringList> choices{
        {"themeMode",{"light","dark","green","auto"}}, {"dockPosition",{"left","bottom","right"}},
        {"panelPosition",{"top","bottom"}}, {"panelStyle",{"detailed","compact"}},
        {"dateFormat",{"locale","iso","day-first"}}, {"language",{"tr_TR.UTF-8","en_US.UTF-8"}},
        {"keyboardLayout",{"tr","us","de","gb"}}};
    if (values.size()!=9 || values.value("reducedMotion").metaType().id()!=QMetaType::Bool) return false;
    for (auto it=choices.cbegin();it!=choices.cend();++it)
        if (values.value(it.key()).metaType().id()!=QMetaType::QString || !it.value().contains(values.value(it.key()).toString())) return false;
    return values.value("accentColor").metaType().id()==QMetaType::QString
        && QRegularExpression("\\A#[0-9a-fA-F]{6}\\z").match(values.value("accentColor").toString()).hasMatch();
}
inline bool singleLine(const QString &value)
{
    return !value.contains(QLatin1Char('\n')) && !value.contains(QLatin1Char('\r'))
        && !value.contains(QChar::Null);
}

inline bool account(const QString &username, const QString &displayName, const QString &password)
{
    static const QRegularExpression userPattern(QStringLiteral("\\A[a-z][a-z0-9_-]{1,30}\\z"));
    if (!userPattern.match(username).hasMatch() || displayName.size() > 80
        || displayName.contains(QLatin1Char(':')) || !singleLine(displayName)
        || password.size() < 8 || password.size() > 256 || !singleLine(password)) return false;
    for (QChar character : displayName)
        if (character.unicode() < 32 || character.unicode() == 127) return false;
    return true;
}
}
