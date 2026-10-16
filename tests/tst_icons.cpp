// SPDX-License-Identifier: GPL-3.0-or-later
#include "ThemeIconProvider.h"
#include "IconLookup.h"
#include "DesktopModel.h"
#include "FilesModel.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>

class IconTest final : public QObject {
    Q_OBJECT
    QTemporaryDir home;
    void write(const QString &path,const QByteArray &data) {
        QDir().mkpath(QFileInfo(path).absolutePath()); QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(data),data.size());
    }
    static QByteArray svg() { return "<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32'><rect width='32' height='32' fill='#df2438'/></svg>"; }
private slots:
    void initTestCase() { QVERIFY(home.isValid()); qputenv("XDG_DATA_HOME",home.path().toUtf8()+"/data"); }
    void embeddedArtworkWorksWithoutTheme() {
        ThemeIconProvider provider; QIcon::setThemeSearchPaths({home.path()+"/missing-theme"}); QIcon::setThemeName("no-such-theme"); QIcon::setFallbackThemeName("");
        const auto entries=QDir(QString(FIDAN_SOURCE_DIR)+"/data/applications").entryInfoList({"*.desktop"},QDir::Files);
        QSet<QByteArray> images;
        for(const auto &entry:entries) {
            QSettings desktop(entry.absoluteFilePath(),QSettings::IniFormat); desktop.beginGroup("Desktop Entry"); const QString icon=desktop.value("Icon").toString();
            QVERIFY2(QFileInfo::exists(":/fidan/icons/"+icon+".svg"),qPrintable(icon));
            for(int scale:{16,32,64}) {
                QSize actual; const auto pixmap=provider.requestPixmap(icon+"?theme=dark",&actual,QSize(scale,scale));
                QVERIFY(!pixmap.isNull()); QCOMPARE(actual,QSize(scale,scale));
                QCOMPARE(pixmap.toImage(),QIcon(":/fidan/icons/"+icon+".svg").pixmap(scale,scale).toImage());
            }
            const auto image=provider.requestPixmap(icon,nullptr,QSize(32,32)).toImage();
            images.insert(QByteArray(reinterpret_cast<const char*>(image.constBits()),image.sizeInBytes()));
        }
        QVERIFY(images.size()>=9);
        QCOMPARE(lookupDesktopIcon("fidan-calculator.desktop").pixmap(32,32).toImage(),lookupDesktopIcon("org.fidan.Calculator").pixmap(32,32).toImage());
        setPiOSApplicationIdentity(*qobject_cast<QGuiApplication*>(qApp),"org.fidan.Calculator");
        QCOMPARE(QGuiApplication::desktopFileName(),QString("org.fidan.Calculator")); QVERIFY(!QGuiApplication::windowIcon().isNull());
    }
    void desktopAndFilesAgreeOnTypes() {
        QTemporaryDir folder;
        const QMap<QString,QString> expected={{"notes.TXT","text-x-generic"},{"report.pdf","application-pdf"},{"photo.webp","image-x-generic"},{"clip.mp4","video-x-generic"},{"track.mp3","audio-x-generic"},{"backup.zip","package-x-generic"},{"code.cpp","text-x-script"},{"letter.odt","x-office-document"},{"unknown.xyz123","application-octet-stream"},{"Calculator.desktop","org.fidan.Calculator"}};
        for(auto it=expected.begin();it!=expected.end();++it) write(folder.filePath(it.key()),it.key().endsWith(".desktop")?QByteArray("[Desktop Entry]\nType=Application\nName=Calculator\nIcon=org.fidan.Calculator\nExec=true\n"):QByteArray());
        DesktopModel desktop(folder.path()); FilesModel files(folder.path()); ThemeIconProvider provider;
        QCOMPARE(desktop.rowCount(),expected.size()); QCOMPARE(files.rowCount(),expected.size());
        for(int row=0;row<desktop.rowCount();++row) {
            const auto index=desktop.index(row,0); const auto path=desktop.data(index,DesktopModel::PathRole).toString(); const auto icon=desktop.data(index,DesktopModel::IconRole).toString();
            QCOMPARE(icon,expected.value(QFileInfo(path).fileName()));
            bool matched=false;
            for(int other=0;other<files.rowCount();++other) if(files.data(files.index(other,0),FilesModel::PathRole).toString()==path) { QCOMPARE(files.data(files.index(other,0),FilesModel::IconRole).toString(),icon); matched=true; }
            QVERIFY(matched); QVERIFY(!provider.requestPixmap(icon,nullptr,QSize(48,48)).isNull());
        }
        const auto text=provider.requestPixmap("text-x-generic",nullptr,QSize(48,48)).toImage();
        QVERIFY(text!=provider.requestPixmap("application-x-executable",nullptr,QSize(48,48)).toImage());
        QVERIFY(text!=provider.requestPixmap("application-pdf",nullptr,QSize(48,48)).toImage());
    }
    void absoluteIconPathAndEncodedQmlUrl() {
        const QString path=home.filePath("Çağrı # icon.svg"); write(path,svg()); ThemeIconProvider provider;
        const auto expected=QIcon(path).pixmap(32,32).toImage(); QVERIFY(!expected.isNull());
        QCOMPARE(provider.requestPixmap(QString::fromUtf8(QUrl::toPercentEncoding(path)),nullptr,QSize(32,32)).toImage(),expected);
        QCOMPARE(provider.requestPixmap(QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded),nullptr,QSize(32,32)).toImage(),expected);
        QQmlEngine engine; engine.addImageProvider("theme",new ThemeIconProvider);
        engine.rootContext()->setContextProperty("testSource","image://theme/"+QString::fromUtf8(QUrl::toPercentEncoding(path)));
        QQmlComponent component(&engine); component.setData("import QtQuick\nImage { width:32; height:32; source: testSource; sourceSize: Qt.size(32,32) }",QUrl());
        std::unique_ptr<QObject> image(component.create()); QVERIFY2(image,qPrintable(component.errorString())); QTRY_COMPARE(image->property("status").toInt(),1);
    }
    void lateInstalledIconBypassesOldThemeCache() {
        ThemeIconProvider provider; const QString name="org.example.LateInstalled";
        const auto before=provider.requestPixmap(name,nullptr,QSize(32,32)).toImage();
        const QString path=home.path()+"/data/icons/hicolor/scalable/apps/"+name+".svg"; write(path,svg());
        const auto after=provider.requestPixmap(name,nullptr,QSize(32,32)).toImage();
        QVERIFY(before!=after); QCOMPARE(after,QIcon(path).pixmap(32,32).toImage());
    }
};
QTEST_MAIN(IconTest)
#include "tst_icons.moc"
