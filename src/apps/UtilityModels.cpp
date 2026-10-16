// SPDX-License-Identifier: GPL-3.0-or-later
#include "UtilityModels.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QLockFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLocale>
#include <QSettings>
#include <QStandardPaths>
#include <QTimeZone>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
QString settingsPath() { return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)+"/fidan/apps.conf"; }
QString number(double value) { return QString::number(value == 0 ? 0 : value, 'g', 15); }
// A bounded arithmetic grammar; user expressions are never executed as code.
class Arithmetic {
    QString text;
    qsizetype position=0;
    int depth=0;
    void spaces() { while(position<text.size() && text[position].isSpace()) ++position; }
    bool take(QChar c) { spaces(); if(position<text.size() && text[position]==c) { ++position; return true; } return false; }
    double checked(double value) { if(!std::isfinite(value)) throw std::runtime_error("Sonuç sayı aralığının dışında."); return value; }
    double sum() { double value=product(); while(true) { if(take('+')) value=checked(value+product()); else if(take('-')) value=checked(value-product()); else return value; } }
    double product() { double value=unary(); while(true) { if(take('*')) value=checked(value*unary()); else if(take('/')) { double divisor=unary(); if(divisor==0) throw std::runtime_error("Sıfıra bölünemez."); value=checked(value/divisor); } else return value; } }
    double unary() { if(++depth>64) throw std::runtime_error("İfade çok karmaşık."); double value; if(take('+')) value=unary(); else if(take('-')) value=-unary(); else value=power(); --depth; return value; }
    double power() { double value=atom(); while(take('%')) value/=100; if(take('^')) value=checked(std::pow(value,unary())); return value; }
    double atom() {
        spaces();
        if(take('(')) { double value=sum(); if(!take(')')) throw std::runtime_error("Parantezleri kontrol et."); return value; }
        if(text.mid(position,4)=="sqrt") { position+=4; if(!take('(')) throw std::runtime_error("Karekök için parantez kullan."); double value=sum(); if(!take(')') || value<0) throw std::runtime_error("Geçersiz karekök."); return checked(std::sqrt(value)); }
        const auto start=position;
        while(position<text.size() && (text[position].isDigit() || text[position]=='.')) ++position;
        if(position<text.size() && (text[position]=='e' || text[position]=='E')) {
            ++position; if(position<text.size() && (text[position]=='+' || text[position]=='-')) ++position;
            while(position<text.size() && text[position].isDigit()) ++position;
        }
        bool ok=false; double value=text.mid(start,position-start).toDouble(&ok);
        if(!ok) throw std::runtime_error("Geçerli bir işlem yaz."); return checked(value);
    }
public:
    explicit Arithmetic(QString expression):text(std::move(expression)) { text.replace(',', '.').replace(QChar(0x00D7),'*').replace(QChar(0x00F7),'/').replace(QChar(0x2212),'-'); }
    double evaluate() { if(text.size()>512) throw std::runtime_error("İfade çok uzun."); double value=sum(); spaces(); if(position!=text.size()) throw std::runtime_error("Geçerli bir işlem yaz."); return checked(value); }
};
}

CalculatorModel::CalculatorModel(QObject *parent):QObject(parent) {
    QSettings settings(settingsPath(),QSettings::IniFormat);
    m_history=settings.value("calculator/history").toList();
    while(m_history.size()>30) m_history.removeLast();
    m_memory=settings.value("calculator/memory",0).toDouble();
    if(!std::isfinite(m_memory)) m_memory=0;
}
QString CalculatorModel::memory() const { return number(m_memory); }
void CalculatorModel::save() { QSettings s(settingsPath(),QSettings::IniFormat); s.setValue("calculator/history",m_history); s.setValue("calculator/memory",m_memory); }
QString CalculatorModel::evaluate(const QString &expression) {
    m_error.clear();
    try { const QString result=number(Arithmetic(expression).evaluate()); m_history.prepend(QVariantMap{{"expression",expression},{"result",result}}); while(m_history.size()>30) m_history.removeLast(); save(); emit changed(); return result; }
    catch(const std::exception &error) { m_error=QString::fromUtf8(error.what()); emit changed(); return {}; }
}
void CalculatorModel::clearHistory() { m_history.clear(); save(); emit changed(); }
void CalculatorModel::clearError() { if(!m_error.isEmpty()) { m_error.clear(); emit changed(); } }
void CalculatorModel::setMemory(const QString &value,bool add) { bool ok; double result=value.toDouble(&ok); if(ok && std::isfinite(result) && std::isfinite(add?m_memory+result:result)) { m_memory=add?m_memory+result:result; save(); emit changed(); } }
void CalculatorModel::clearMemory() { m_memory=0; save(); emit changed(); }
void CalculatorModel::copy(const QString &text) { QGuiApplication::clipboard()->setText(text); }

CalendarModel::CalendarModel(QObject *parent):QObject(parent) {
    m_path=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/fidan/calendar/events.json";
    read();
    auto *timer=new QTimer(this); timer->setInterval(60000); connect(timer,&QTimer::timeout,this,&CalendarModel::changed); timer->start();
}
bool CalendarModel::read() {
    QFile file(m_path); if(!file.exists()) { m_events.clear(); return true; }
    if(!file.open(QIODevice::ReadOnly) || file.size()>4*1024*1024) { m_error="Takvim dosyası okunamadı."; return false; }
    QJsonParseError error; const auto document=QJsonDocument::fromJson(file.readAll(),&error);
    if(error.error!=QJsonParseError::NoError || !document.isArray()) { m_error="Takvim dosyası bozuk; mevcut dosya korunuyor."; return false; }
    m_events=document.array().toVariantList(); return true;
}
bool CalendarModel::write() {
    QSaveFile file(m_path);
    if(!file.open(QIODevice::WriteOnly)) { m_error="Etkinlikler kaydedilemedi."; return false; }
    file.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);
    const auto data=QJsonDocument(QJsonArray::fromVariantList(m_events)).toJson();
    if(file.write(data)!=data.size() || !file.commit()) { m_error="Etkinlikler kaydedilemedi."; return false; }
    return true;
}
QVariantList CalendarModel::month(int year,int monthNumber) const {
    const QDate first(year,monthNumber,1); QVariantList cells; if(!first.isValid()) return cells;
    QDate date=first.addDays(1-first.dayOfWeek());
    for(int i=0;i<42;++i,date=date.addDays(1)) { const QString iso=date.toString(Qt::ISODate); cells.append(QVariantMap{{"date",iso},{"day",date.day()},{"current",date.month()==monthNumber},{"today",date==QDate::currentDate()},{"count",events(iso).size()}}); }
    return cells;
}
QVariantList CalendarModel::events(const QString &date) const {
    QVariantList result; for(const auto &event:m_events) if(event.toMap().value("date").toString()==date) result.append(event);
    std::stable_sort(result.begin(),result.end(),[](const QVariant &a,const QVariant &b){return a.toMap().value("time").toString()<b.toMap().value("time").toString();}); return result;
}
bool CalendarModel::saveEvent(const QString &id,const QString &date,const QString &time,const QString &title) {
    m_error.clear();
    if(!QDate::fromString(date,Qt::ISODate).isValid() || QDate::fromString(date,Qt::ISODate).toString(Qt::ISODate)!=date || (!time.isEmpty() && (!QTime::fromString(time,"HH:mm").isValid() || QTime::fromString(time,"HH:mm").toString("HH:mm")!=time)) || title.trimmed().isEmpty() || title.size()>200) { m_error="Tarih, saat (SS:dd) ve başlığı kontrol et."; emit changed(); return false; }
    QDir().mkpath(QFileInfo(m_path).absolutePath()); QLockFile lock(m_path+".lock");
    if(!lock.tryLock(1000)) { m_error="Takvim başka bir pencerede kaydediliyor."; emit changed(); return false; }
    if(!read()) { emit changed(); return false; } const auto previous=m_events;
    const QString eventId=id.isEmpty()?QUuid::createUuid().toString(QUuid::WithoutBraces):id;
    if(!id.isEmpty() && std::none_of(m_events.begin(),m_events.end(),[&](const QVariant &v){ return v.toMap().value("id")==id; })) { m_error="Etkinlik artık mevcut değil."; emit changed(); return false; }
    m_events.erase(std::remove_if(m_events.begin(),m_events.end(),[&](const QVariant &v){return v.toMap().value("id")==eventId;}),m_events.end());
    if(m_events.size()>=5000) { m_events=previous; m_error="Etkinlik sınırına ulaşıldı."; emit changed(); return false; }
    m_events.append(QVariantMap{{"id",eventId},{"date",date},{"time",time},{"title",title.trimmed()}});
    const bool ok=write(); if(!ok) m_events=previous; ++m_revision; emit changed(); return ok;
}
bool CalendarModel::removeEvent(const QString &id) {
    m_error.clear(); QLockFile lock(m_path+".lock"); if(!lock.tryLock(1000)) { m_error="Takvim dosyası meşgul."; emit changed(); return false; }
    if(!read()) { emit changed(); return false; } const auto previous=m_events;
    m_events.erase(std::remove_if(m_events.begin(),m_events.end(),[&](const QVariant &v){return v.toMap().value("id")==id;}),m_events.end());
    const bool ok=write(); if(!ok) m_events=previous; ++m_revision; emit changed(); return ok;
}
QString CalendarModel::dateLabel(const QString &date) const { return QLocale("tr_TR").toString(QDate::fromString(date,Qt::ISODate),"d MMMM yyyy, dddd"); }

ClockModel::ClockModel(QObject *parent):QObject(parent) {
    m_tick.setInterval(50); connect(&m_tick,&QTimer::timeout,this,[this]{
        const auto wall=QDateTime::currentSecsSinceEpoch();
        if(wall!=m_lastWall) { m_lastWall=wall; emit wallTick(); }
        if(m_counting && m_countdown.elapsed()>=m_remainingMs) { m_counting=false; m_remainingMs=0; emit timerFinished(); emit tick(); }
        if(m_running || m_counting) emit tick();
    }); m_tick.start();
}
QString ClockModel::time() const { return QTime::currentTime().toString("HH:mm:ss"); }
QString ClockModel::date() const { return QLocale("tr_TR").toString(QDate::currentDate(),"d MMMM yyyy, dddd"); }
QVariantList ClockModel::cities() const {
    QVariantList result; const QList<QPair<QString,QByteArray>> zones={{"İstanbul","Europe/Istanbul"},{"Londra","Europe/London"},{"New York","America/New_York"},{"Tokyo","Asia/Tokyo"},{"Berlin","Europe/Berlin"},{"Sidney","Australia/Sydney"}};
    const auto now=QDateTime::currentDateTimeUtc(); for(const auto &zone:zones) { const auto local=now.toTimeZone(QTimeZone(zone.second)); result.append(QVariantMap{{"name",zone.first},{"time",local.toString("HH:mm")},{"date",QLocale("tr_TR").toString(local.date(),"d MMM ddd")},{"zone",local.timeZoneAbbreviation()}}); } return result;
}
qint64 ClockModel::elapsed() const { return m_accumulated+(m_running?m_stopwatch.elapsed():0); }
void ClockModel::toggleStopwatch() { if(m_running) m_accumulated+=m_stopwatch.elapsed(); else m_stopwatch.start(); m_running=!m_running; emit tick(); }
void ClockModel::resetStopwatch() { m_accumulated=0; m_running=false; m_laps.clear(); emit tick(); }
void ClockModel::lap() { if(m_running && m_laps.size()<100) { m_laps.prepend(elapsed()); emit tick(); } }
int ClockModel::remaining() const { return int(std::max<qint64>(0,m_remainingMs-(m_counting?m_countdown.elapsed():0)+999)/1000); }
void ClockModel::startTimer(int seconds) { if(seconds<1 || seconds>24*60*60) return; m_remainingMs=qint64(seconds)*1000; m_countdown.start(); m_counting=true; emit tick(); }
void ClockModel::pauseTimer() { if(m_counting) { m_remainingMs=std::max<qint64>(0,m_remainingMs-m_countdown.elapsed()); m_counting=false; emit tick(); } }
void ClockModel::resumeTimer() { if(!m_counting && m_remainingMs>0) { m_countdown.start(); m_counting=true; emit tick(); } }
void ClockModel::cancelTimer() { m_counting=false; m_remainingMs=0; emit tick(); }

bool DocumentFiles::open(const QUrl &url) {
    m_error.clear(); const QFileInfo file(url.toLocalFile());
    if(!url.isLocalFile() || !file.isFile() || !file.isReadable()) { m_error="Okunabilir bir yerel PDF dosyası seç."; emit changed(); return false; }
    QFile input(file.absoluteFilePath()); if(!input.open(QIODevice::ReadOnly) || !input.read(1024).contains("%PDF-")) { m_error="Bu dosya geçerli bir PDF değil."; emit changed(); return false; }
    m_source=QUrl::fromLocalFile(file.absoluteFilePath()); m_name=file.fileName(); emit sourceChanged(); emit changed(); return true;
}

bool DocumentFiles::openLink(const QUrl &url) {
    m_error.clear();
    if(!url.isValid() || url.isRelative() || !QStringList{"https","http"}.contains(url.scheme().toLower())
        || url.host().isEmpty() || !url.userInfo().isEmpty()) {
        m_error="PDF içinden yalnızca HTTP/HTTPS web bağlantıları açılabilir.";
        emit changed(); return false;
    }
    const bool opened=QDesktopServices::openUrl(url);
    if(!opened) m_error="Web bağlantısı tarayıcıda açılamadı.";
    emit changed(); return opened;
}
