// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QDateTime>
#include <QElapsedTimer>
#include <QTimer>
#include <QVariantList>
#include <QUrl>

class CalculatorModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QVariantList history READ history NOTIFY changed)
    Q_PROPERTY(QString memory READ memory NOTIFY changed)
public:
    explicit CalculatorModel(QObject *parent=nullptr);
    QString error() const { return m_error; }
    QVariantList history() const { return m_history; }
    QString memory() const;
    Q_INVOKABLE QString evaluate(const QString &expression);
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE void setMemory(const QString &value, bool add=false);
    Q_INVOKABLE void clearMemory();
    Q_INVOKABLE void copy(const QString &text);
signals:
    void changed();
private:
    void save();
    QString m_error;
    QVariantList m_history;
    double m_memory=0;
};

class CalendarModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QString today READ today NOTIFY changed)
    Q_PROPERTY(int revision READ revision NOTIFY changed)
public:
    explicit CalendarModel(QObject *parent=nullptr);
    QString error() const { return m_error; }
    QString today() const { return QDate::currentDate().toString(Qt::ISODate); }
    int revision() const { return m_revision; }
    Q_INVOKABLE QVariantList month(int year, int month) const;
    Q_INVOKABLE QVariantList events(const QString &date) const;
    Q_INVOKABLE bool saveEvent(const QString &id, const QString &date, const QString &time, const QString &title);
    Q_INVOKABLE bool removeEvent(const QString &id);
    Q_INVOKABLE QString dateLabel(const QString &date) const;
signals:
    void changed();
private:
    bool read();
    bool write();
    QString m_path, m_error;
    QVariantList m_events;
    int m_revision=0;
};

class ClockModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString time READ time NOTIFY wallTick)
    Q_PROPERTY(QString date READ date NOTIFY wallTick)
    Q_PROPERTY(QVariantList cities READ cities NOTIFY wallTick)
    Q_PROPERTY(qint64 elapsed READ elapsed NOTIFY tick)
    Q_PROPERTY(bool running READ running NOTIFY tick)
    Q_PROPERTY(QVariantList laps READ laps NOTIFY tick)
    Q_PROPERTY(int remaining READ remaining NOTIFY tick)
    Q_PROPERTY(bool counting READ counting NOTIFY tick)
public:
    explicit ClockModel(QObject *parent=nullptr);
    QString time() const;
    QString date() const;
    QVariantList cities() const;
    qint64 elapsed() const;
    bool running() const { return m_running; }
    QVariantList laps() const { return m_laps; }
    int remaining() const;
    bool counting() const { return m_counting; }
    Q_INVOKABLE void toggleStopwatch();
    Q_INVOKABLE void resetStopwatch();
    Q_INVOKABLE void lap();
    Q_INVOKABLE void startTimer(int seconds);
    Q_INVOKABLE void pauseTimer();
    Q_INVOKABLE void resumeTimer();
    Q_INVOKABLE void cancelTimer();
signals:
    void tick();
    void wallTick();
    void timerFinished();
private:
    QTimer m_tick;
    QElapsedTimer m_stopwatch, m_countdown;
    qint64 m_accumulated=0, m_remainingMs=0;
    bool m_running=false, m_counting=false;
    qint64 m_lastWall=0;
    QVariantList m_laps;
};

class DocumentFiles final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source NOTIFY sourceChanged)
    Q_PROPERTY(QString name READ name NOTIFY sourceChanged)
    Q_PROPERTY(QString error READ error NOTIFY changed)
public:
    using QObject::QObject;
    QUrl source() const { return m_source; }
    QString name() const { return m_name; }
    QString error() const { return m_error; }
    Q_INVOKABLE bool open(const QUrl &url);
    Q_INVOKABLE bool openLink(const QUrl &url);
signals:
    void sourceChanged();
    void changed();
private:
    QUrl m_source;
    QString m_name, m_error;
};
