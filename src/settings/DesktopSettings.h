// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QFutureWatcher>
#include <QProcess>
#include <QTimer>
#include <QVariantMap>

class DesktopSettings final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
    Q_PROPERTY(bool refreshing READ refreshing NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QVariantMap input READ input NOTIFY changed)
public:
    explicit DesktopSettings(bool mock=false,QObject *parent=nullptr);
    ~DesktopSettings() override;
    QVariantMap state() const { return m_state; }
    QVariantMap input() const;
    bool refreshing() const { return m_refreshing; }
    bool busy() const { return m_action.state()!=QProcess::NotRunning; }
    QString error() const { return m_error; }
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setPowerProfile(const QString &profile);
    Q_INVOKABLE void setAudioDefault(const QString &kind,const QString &name);
    Q_INVOKABLE void setAudioVolume(const QString &kind,const QString &name,int value);
    Q_INVOKABLE void setAudioMute(const QString &kind,const QString &name,bool muted);
    Q_INVOKABLE void setAudioPort(const QString &kind,const QString &name,const QString &port);
    Q_INVOKABLE void setApplicationVolume(int id,int value);
    Q_INVOKABLE void activateConnection(const QString &uuid,bool active);
    Q_INVOKABLE void setInput(const QString &key,const QVariant &value);
    Q_INVOKABLE void setFlatpakPermission(const QString &app,const QString &permission,bool allowed);
    Q_INVOKABLE void setCameraPermission(const QString &app,const QString &decision);
    Q_INVOKABLE void setPortalPermission(const QString &app,const QString &resource,const QString &decision);
    Q_INVOKABLE QVariantMap permissions(const QString &app) const;
    static QVariantMap collect();
signals:
    void changed();
private:
    bool known(const QString &list,const QString &field,const QVariant &value) const;
    void action(const QString &program,const QStringList &arguments);
    bool m_mock=false;
    bool m_refreshing=false;
    bool m_refreshAgain=false;
    QString m_error;
    QVariantMap m_state;
    QFutureWatcher<QVariantMap> m_query;
    QProcess m_action;
    QTimer m_timeout;
};
