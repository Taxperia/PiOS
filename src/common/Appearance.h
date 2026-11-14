// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QColor>
#include <QSettings>
#include <QStringList>
#include <QTime>

namespace FidanAppearance {
inline QString mode(QSettings &settings) {
    QString value=settings.value("appearance/themeMode",settings.value("appearance/darkTheme",true).toBool()?"green":"light").toString();
    // Old releases called the original green palette "dark".
    if(value=="dark" && settings.value("appearance/themeSchema",0).toInt()<2)value="green";
    return QStringList{"light","dark","green","auto"}.contains(value)?value:QStringLiteral("green");
}
inline bool dark(const QString &mode) {
    return mode=="dark" || mode=="green" || (mode=="auto" && (QTime::currentTime().hour()<8 || QTime::currentTime().hour()>=19));
}
inline QColor color(const QString &value,const QString &mode,bool dark) {
    QColor result(value);
    if(!result.isValid() || !dark || (mode!="dark" && mode!="auto"))return result;
    // Preserve accent/status colours and alpha, neutralise tinted surfaces,
    // borders and text for the black palette.
    const qreal hue=result.hslHueF(),saturation=result.hslSaturationF();
    if(saturation<0.55 && (hue<0 || (hue>=0.16 && hue<=0.76))) {
        int grey=qGray(result.rgb());
        if(grey<85)grey=qRound(grey*0.72);
        const int alpha=grey<62 && result.alpha()>=160?255:result.alpha();
        result.setRgb(grey,grey,grey,alpha);
    }
    return result;
}
}
