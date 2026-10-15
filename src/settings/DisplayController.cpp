// SPDX-License-Identifier: GPL-3.0-or-later
#include "DisplayController.h"
#include <QGuiApplication>
#include <QScreen>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QSettings>
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QFileInfo>
#include <cmath>

namespace {
QString profilePath() { return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/fidan/displays.json"; }
}

DisplayController::DisplayController(bool mock, QObject *parent) : QObject(parent), m_mock(mock) {
    connect(&m_process, &QProcess::stateChanged, this, [this] { emit busyChanged(); });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) { error(tr("The display service could not be started.")); m_timeout.stop(); emit busyChanged(); }
    });
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &DisplayController::finished);
    m_timeout.setSingleShot(true); m_timeout.setInterval(8000);
    connect(&m_timeout, &QTimer::timeout, this, [this] { error(tr("The display service did not respond.")); m_process.kill(); });
    m_countdown.setInterval(1000);
    connect(&m_countdown, &QTimer::timeout, this, [this] { --m_seconds; emit confirmationChanged(); if (m_seconds <= 0) revert(); });
    connect(&m_color, &QProcess::stateChanged, this, [this] { emit colorChanged(); });
    connect(&m_color, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) { if (e == QProcess::FailedToStart) error(tr("Night Light service is not installed.")); });
    connect(&m_color, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this](int code, QProcess::ExitStatus status) {
        const auto result = QJsonDocument::fromJson(m_color.readAllStandardOutput()).object();
        if (code != 0 || status != QProcess::NormalExit || !result.value("ok").toBool()) {
            error(result.value("error").toString(tr("Night Light is not supported by this output.")));
        } else {
            m_nightEnabled = result.value("enabled").toBool(); m_temperature = result.value("temperature").toInt(m_temperature);
        }
        emit colorChanged();
    });
    fallbackOutputs();
    const auto screensChanged = [this] { QTimer::singleShot(300, this, &DisplayController::refresh); };
    connect(qGuiApp, &QGuiApplication::screenAdded, this, screensChanged);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, screensChanged);
    QTimer::singleShot(0, this, &DisplayController::refresh);
}

DisplayController::~DisplayController() {
    disconnect(&m_process, nullptr, this, nullptr);
    disconnect(&m_color, nullptr, this, nullptr);
    m_timeout.stop(); m_countdown.stop();
    if (m_process.state() != QProcess::NotRunning && !m_process.waitForFinished(1500)) { m_process.kill(); m_process.waitForFinished(); }
    if (m_changed && !m_original.isEmpty()) {
        QProcess restore; restore.start("wlr-randr", arguments(m_original));
        if (!restore.waitForFinished(4000)) { restore.kill(); restore.waitForFinished(); }
    }
    if (m_color.state() != QProcess::NotRunning && !m_color.waitForFinished(2000)) { m_color.kill(); m_color.waitForFinished(); }
}

QVariantMap DisplayController::currentOutput() const {
    for (const auto &value : m_outputs) if (value.toMap().value("name").toString() == m_selected) return value.toMap();
    return {};
}
void DisplayController::setSelectedName(const QString &name) {
    if (m_selected == name || m_pending || busy() || colorBusy()) return;
    for (const auto &value : m_outputs) if (value.toMap().value("name").toString() == name) {
        m_selected = name; loadColor(); error({}); emit selectionChanged(); return;
    }
}
void DisplayController::loadColor() {
    QFile file(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/fidan/night-light.json");
    QJsonObject entry;
    if (file.open(QIODevice::ReadOnly)) entry = QJsonDocument::fromJson(file.readAll()).object().value(m_selected).toObject();
    m_temperature = qBound(2500, entry.value("temperature").toInt(4500), 6500);
    m_nightEnabled = false;
    if (entry.value("enabled").toBool() && nightLightAvailable()) {
        QProcess query; query.start("fidan-night-light", {"status", m_selected});
        if (query.waitForFinished(700)) m_nightEnabled = QJsonDocument::fromJson(query.readAllStandardOutput()).object().value("enabled").toBool();
        else { query.kill(); query.waitForFinished(); }
    }
    emit colorChanged();
}
bool DisplayController::nightLightAvailable() const {
    return !m_mock && !m_selected.isEmpty() && !m_selected.startsWith("WL-") && !m_selected.startsWith("HEADLESS-")
        && !QStandardPaths::findExecutable("wlsunset").isEmpty() && !QStandardPaths::findExecutable("fidan-night-light").isEmpty();
}
void DisplayController::setNightLight(bool enabled, int temperature) {
    if (!nightLightAvailable() || colorBusy() || m_pending) return;
    error({});
    m_color.start("fidan-night-light", {enabled ? "on" : "off", m_selected, QString::number(qBound(2500, temperature, 6500))});
}
void DisplayController::fallbackOutputs() {
    m_outputs.clear();
    int number = 0;
    for (auto *screen : QGuiApplication::screens()) {
        const auto size = screen->size();
        const QVariantMap mode{{"width", size.width()}, {"height", size.height()}, {"refresh", screen->refreshRate()}, {"current", true}, {"preferred", true}};
        m_outputs.append(QVariantMap{{"name", screen->name()}, {"label", tr("Display %1").arg(++number)}, {"description", screen->name()},
            {"enabled", true}, {"modes", QVariantList{mode}}, {"scale", screen->devicePixelRatio()}, {"transform", "normal"}, {"number", number}, {"primary", screen == QGuiApplication::primaryScreen()}});
    }
    if (!m_outputs.isEmpty()) m_selected = m_outputs.first().toMap().value("name").toString();
    loadColor();
}
void DisplayController::refresh() {
    if (busy() || m_pending) return;
    if (m_mock) {
        loadOutputs(R"([{"name":"eDP-1","description":"Built-in Display","enabled":true,"scale":1,"transform":"normal","modes":[{"width":1920,"height":1080,"refresh":60,"current":true,"preferred":true},{"width":1280,"height":720,"refresh":60}]},{"name":"HDMI-A-1","description":"HDMI Display","enabled":true,"scale":1,"transform":"normal","modes":[{"width":2560,"height":1440,"refresh":60,"current":true,"preferred":true}]}])"); return;
    }
    if (QStandardPaths::findExecutable("wlr-randr").isEmpty()) {
        m_available = false; error(tr("Install wlr-randr to configure the connected displays.")); emit outputsChanged(); return;
    }
    start("refresh", {"--json"});
}
void DisplayController::loadOutputs(const QByteArray &json) {
    QJsonParseError parse; const auto document = QJsonDocument::fromJson(json, &parse);
    if (parse.error != QJsonParseError::NoError || !document.isArray()) { error(tr("The display service returned invalid monitor data.")); return; }
    QVariantList outputs; int number = 0;
    for (const auto &value : document.array()) {
        auto output = value.toObject().toVariantMap();
        if (!output.value("enabled").toBool() || output.value("name").toString().isEmpty()) continue;
        const auto name = output.value("name").toString();
        const bool builtin = name.startsWith("eDP") || name.startsWith("LVDS") || name.startsWith("DSI");
        output.insert("label", builtin ? tr("Built-in Display") : name.startsWith("HDMI") ? tr("HDMI Display") : name.startsWith("WL-") ? tr("Virtual Display") : tr("Display %1").arg(number + 1));
        output.insert("number", ++number);
        output.insert("primary", QGuiApplication::primaryScreen() && QGuiApplication::primaryScreen()->name() == name);
        outputs.append(output);
    }
    m_outputs = outputs; m_available = !m_mock && !outputs.isEmpty();
    bool found = false; for (const auto &output : outputs) if (output.toMap().value("name").toString() == m_selected) found = true;
    if (!found) m_selected = outputs.isEmpty() ? QString() : outputs.first().toMap().value("name").toString();
    loadColor(); emit outputsChanged(); emit selectionChanged();
}
QStringList DisplayController::arguments(const QVariantMap &output) {
    QString mode = QStringLiteral("%1x%2").arg(output.value("width").toInt()).arg(output.value("height").toInt());
    const double hz = output.value("refresh").toDouble(); if (hz > 0) mode += "@" + QString::number(hz, 'f', 3) + "Hz";
    return {"--output", output.value("name").toString(), "--mode", mode, "--scale", QString::number(output.value("scale").toDouble(), 'f', 2), "--transform", output.value("transform").toString()};
}
void DisplayController::apply(int width, int height, double refreshRate, double scale, const QString &transform) {
    if (!m_available || busy() || m_pending || colorBusy()) return;
    if (!std::isfinite(scale) || scale < 0.75 || scale > 2.0 || !QStringList{"normal", "90", "180", "270"}.contains(transform)) { error(tr("Choose a supported scale and orientation.")); return; }
    const auto current = currentOutput(); QVariantMap originalMode; bool valid = false;
    for (const auto &value : current.value("modes").toList()) {
        const auto mode = value.toMap();
        if (mode.value("current").toBool()) originalMode = mode;
        if (mode.value("width").toInt() == width && mode.value("height").toInt() == height && std::abs(mode.value("refresh").toDouble() - refreshRate) < 0.01) valid = true;
    }
    if (!valid || originalMode.isEmpty()) { error(tr("That mode is not advertised by the selected monitor.")); return; }
    m_original = current;
    for (const auto &key : {"width", "height", "refresh"}) m_original.insert(key, originalMode.value(key));
    m_target = {{"name", m_selected}, {"width", width}, {"height", height}, {"refresh", refreshRate}, {"scale", scale}, {"transform", transform}};
    error({}); auto args = arguments(m_target); args.prepend("--dryrun"); start("validate", args);
}
void DisplayController::start(const QString &phase, const QStringList &args) {
    m_phase = phase; m_process.start("wlr-randr", args); m_timeout.start();
}
void DisplayController::finished(int code, QProcess::ExitStatus status) {
    m_timeout.stop(); const auto out = m_process.readAllStandardOutput(); const auto err = QString::fromUtf8(m_process.readAllStandardError()).trimmed();
    const auto phase = m_phase; m_phase.clear();
    const bool ok = code == 0 && status == QProcess::NormalExit;
    if (phase == "refresh") {
        if (ok) loadOutputs(out);
        else { m_available = false; error(err.isEmpty() ? tr("The compositor does not support display configuration.") : err); emit outputsChanged(); }
    } else if (phase == "validate") {
        if (ok && !m_revertRequested) { m_changed = true; start("apply", arguments(m_target)); return; }
        m_original.clear(); m_target.clear(); m_revertRequested = false;
        if (!ok) error(err.isEmpty() ? tr("The monitor rejected these settings.") : err);
    } else if (phase == "apply") {
        if (ok && !m_revertRequested) { m_pending = true; m_seconds = 15; m_countdown.start(); emit confirmationChanged(); }
        else { if (!ok) error(err.isEmpty() ? tr("The display change failed; restoring the previous mode.") : err); revert(); }
    } else if (phase == "revert") {
        m_changed = false; m_original.clear(); m_target.clear(); m_revertRequested = false;
        if (!ok) error(tr("The previous display mode could not be restored: %1").arg(err));
        refresh();
    }
    emit busyChanged();
}
void DisplayController::keep() {
    if (!m_pending || busy()) return;
    m_countdown.stop(); m_pending = false;
    if (!saveTarget()) error(tr("Settings were applied for this session, but the saved display profile could not be written."));
    m_changed = false; m_original.clear(); m_target.clear(); emit confirmationChanged(); refresh();
}
void DisplayController::revert() {
    m_countdown.stop(); m_pending = false; emit confirmationChanged();
    if (busy()) { m_revertRequested = true; return; }
    if (!m_changed || m_original.isEmpty()) { m_original.clear(); return; }
    start("revert", arguments(m_original));
}
bool DisplayController::saveTarget() {
    QFile existing(profilePath()); QJsonObject profiles;
    if (existing.open(QIODevice::ReadOnly)) profiles = QJsonDocument::fromJson(existing.readAll()).object();
    profiles.insert(m_selected, QJsonObject::fromVariantMap(m_target));
    QDir().mkpath(QFileInfo(profilePath()).absolutePath());
    QSaveFile file(profilePath());
    return file.open(QIODevice::WriteOnly) && file.write(QJsonDocument(profiles).toJson()) >= 0 && file.commit();
}
void DisplayController::identify() {
    if (m_identifying) return;
    m_identifying = true; emit identifyingChanged();
    QTimer::singleShot(3000, this, [this] { m_identifying = false; emit identifyingChanged(); });
}
void DisplayController::error(const QString &text) { if (m_error != text) { m_error = text; emit errorChanged(); } }
