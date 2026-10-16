// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QRegularExpression>
#include <QString>

namespace FirstBootValidation {
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
