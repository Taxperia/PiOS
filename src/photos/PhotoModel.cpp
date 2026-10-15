// SPDX-License-Identifier: GPL-3.0-or-later
#include "PhotoModel.h"
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QClipboard>
#include <QImageReader>
#include <QLocale>
#include <QSettings>
#include <QStandardPaths>
#include <QTransform>
#include <QtConcurrentRun>

PhotoModel::PhotoModel(QObject *parent) : QObject(parent) {
    QImageReader::setAllocationLimit(256);
    connect(&m_copy, &QFutureWatcher<QImage>::finished, this, [this] {
        m_copying = false;
        const auto image = m_copy.result();
        if (image.isNull()) setError(tr("This image could not be copied."));
        else { QGuiApplication::clipboard()->setImage(image); emit message(tr("Image copied to clipboard.")); }
        emit copyingChanged();
    });
}
PhotoModel::~PhotoModel() { m_copy.waitForFinished(); }

QStringList PhotoModel::supportedExtensions() {
    QStringList result;
    const auto formats = QImageReader::supportedImageFormats();
    for (const auto &format : formats) {
        const auto ext = QString::fromLatin1(format).toLower();
        if (QStringList{"png", "jpg", "jpeg", "webp", "bmp", "gif", "tif", "tiff", "avif", "heic", "heif"}.contains(ext))
            result.append(ext);
    }
    return result;
}

bool PhotoModel::open(const QUrl &url) {
    if (!url.isLocalFile()) { setError(tr("Choose an image from this computer.")); return false; }
    const QFileInfo info(url.toLocalFile());
    if (!info.exists() || !info.isFile() || !info.isReadable()) {
        setError(tr("The image is missing or cannot be read.")); return false;
    }
    QStringList siblings;
    const auto extensions = supportedExtensions();
    for (const auto &entry : info.dir().entryInfoList(QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase))
        if (extensions.contains(entry.suffix().toLower())) siblings.append(entry.absoluteFilePath());
    if (!siblings.contains(info.absoluteFilePath())) siblings.append(info.absoluteFilePath());
    m_files = siblings;
    m_index = m_files.indexOf(info.absoluteFilePath());
    return load(info.absoluteFilePath());
}

bool PhotoModel::load(const QString &path) {
    QImageReader reader(path);
    reader.setAutoTransform(true);
    m_name = QFileInfo(path).fileName(); m_rotation = 0; m_size = reader.size();
    if (reader.transformation() & QImageIOHandler::TransformationRotate90) m_size.transpose();
    const bool valid = reader.canRead() && m_size.isValid()
        && qint64(m_size.width()) * m_size.height() <= 64000000;
    m_url = valid ? QUrl::fromLocalFile(path) : QUrl{};
    m_details = valid ? tr("%1 × %2 pixels · %3 · %4")
        .arg(m_size.width()).arg(m_size.height()).arg(QString::fromLatin1(reader.format()).toUpper(), QLocale().formattedDataSize(QFileInfo(path).size())) : QString();
    setError(valid ? QString() : tr("This image cannot be opened. It may be damaged, unsupported or larger than 64 megapixels."));
    emit rotationChanged(); emit currentChanged();
    return valid;
}
void PhotoModel::next() { if (m_index + 1 < m_files.size()) load(m_files.at(++m_index)); }
void PhotoModel::previous() { if (m_index > 0) load(m_files.at(--m_index)); }
void PhotoModel::rotate(int direction) {
    if (m_url.isEmpty()) return;
    m_rotation = (m_rotation + (direction < 0 ? 270 : 90)) % 360;
    emit rotationChanged();
}
void PhotoModel::copyImage() {
    if (m_copying || m_url.isEmpty()) return;
    const auto path = m_url.toLocalFile(); const int rotation = m_rotation;
    m_copying = true; emit copyingChanged();
    m_copy.setFuture(QtConcurrent::run([path, rotation] {
        QImageReader reader(path); reader.setAutoTransform(true);
        auto image = reader.read();
        return rotation ? image.transformed(QTransform().rotate(rotation)) : image;
    }));
}
bool PhotoModel::useAsWallpaper() {
    if (m_url.isEmpty() || !QFileInfo::exists(m_url.toLocalFile())) return false;
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/fidan/shell.conf", QSettings::IniFormat);
    settings.setValue("appearance/wallpaper", m_url.toString()); settings.sync();
    if (settings.status() != QSettings::NoError) { setError(tr("The wallpaper preference could not be saved.")); return false; }
    emit message(tr("Desktop wallpaper updated.")); return true;
}
void PhotoModel::decodeFailed() { setError(tr("The image could not be decoded. Try another file.")); }
void PhotoModel::setError(const QString &text) { if (m_error != text) { m_error = text; emit errorChanged(); } }
