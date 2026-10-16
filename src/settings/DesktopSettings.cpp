// SPDX-License-Identifier: GPL-3.0-or-later
#include "DesktopSettings.h"
#include "SettingsData.h"
#include <QtConcurrentRun>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusArgument>
#include <QProcessEnvironment>
#include <QSet>
#include <functional>
#include <algorithm>

namespace {
QString readFile(const QString &path) { QFile f(path); return f.open(QIODevice::ReadOnly)?QString::fromUtf8(f.readAll()).trimmed():QString(); }
QByteArray command(const QString &program,const QStringList &args,int timeout=2500) {
    if(QStandardPaths::findExecutable(program).isEmpty()) return {};
    QProcess p; auto env=QProcessEnvironment::systemEnvironment(); env.insert("LC_ALL","C.UTF-8"); p.setProcessEnvironment(env); p.start(program,args);
    if(!p.waitForFinished(timeout)) { p.kill(); p.waitForFinished(1000); return {}; }
    return p.exitStatus()==QProcess::NormalExit && p.exitCode()==0?p.readAllStandardOutput():QByteArray();
}
QString configPath() { return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)+"/fidan/input.conf"; }
QVariantList devices(const QByteArray &json,const QString &defaultName,bool source) {
    QVariantList result;
    for(const auto &value:QJsonDocument::fromJson(json).array()) {
        const auto object=value.toObject(); const auto name=object.value("name").toString(); if(source && name.endsWith(".monitor")) continue;
        QVariantList ports; const auto rawPorts=object.value("ports");
        auto appendPort=[&](QString id,QJsonObject port){ ports.append(QVariantMap{{"name",id},{"description",port.value("description").toString(id)},{"available",port.value("availability").toString()!="not available"}}); };
        if(rawPorts.isObject()) { const auto objectPorts=rawPorts.toObject(); for(auto it=objectPorts.begin();it!=objectPorts.end();++it) appendPort(it.key(),it.value().toObject()); }
        if(rawPorts.isArray()) for(const auto &port:rawPorts.toArray()) appendPort(port.toObject().value("name").toString(),port.toObject());
        double percent=0; int count=0; const auto volumes=object.value("volume").toObject();
        for(auto it=volumes.begin();it!=volumes.end();++it) { percent+=it.value().toObject().value("value").toDouble()*100/65536; ++count; }
        result.append(QVariantMap{{"name",name},{"description",object.value("description").toString(name)},{"default",name==defaultName},{"volume",count?qRound(percent/count):0},{"muted",object.value("mute").toBool()},{"state",object.value("state").toString()},{"ports",ports},{"port",object.value("active_port").toString()}});
    }
    return result;
}
QVariantList readApplications() {
    QMap<QString,QVariantMap> packages;
    for(const auto &line:QString::fromUtf8(command("dpkg-query",{"-W","-f=${binary:Package}\t${Version}\t${Installed-Size}\t${db:Status-Status}\n"},7000)).split('\n')) {
        const auto fields=line.split('\t'); if(fields.size()==4 && fields[3]=="installed") { const auto name=fields[0].section(':',0,0); packages[name]={{"package",fields[0]},{"version",fields[1]},{"size",qlonglong(fields[2].toLongLong()*1024)}}; }
    }
    QMap<QString,QString> owners;
    for(const auto &line:QString::fromUtf8(command("dpkg-query",{"-S","/usr/share/applications/*.desktop"},7000)).split('\n')) {
        const auto position=line.indexOf(": /"); if(position>0) owners[line.mid(position+2).trimmed()]=line.left(position).section(':',0,0);
    }
    QVariantList result; QSet<QString> seen;
    auto roots=QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    for(const auto &root:roots) for(const auto &file:QDir(root).entryInfoList({"*.desktop"},QDir::Files)) {
        if(seen.contains(file.fileName())) continue;
        QSettings desktop(file.absoluteFilePath(),QSettings::IniFormat); desktop.beginGroup("Desktop Entry");
        // User entries can hide a system entry of the same ID.
        seen.insert(file.fileName());
        if(desktop.value("Type").toString()!="Application" || desktop.value("Hidden",false).toBool() || desktop.value("NoDisplay",false).toBool() || desktop.value("Exec").toString().isEmpty()) continue;
        const QString flatpak=desktop.value("X-Flatpak").toString(); const QString owner=owners.value(file.absoluteFilePath()); auto entry=packages.value(owner);
        entry.insert("id",file.fileName()); entry.insert("name",desktop.value("Name[tr]",desktop.value("Name")).toString()); entry.insert("icon",desktop.value("Icon",file.completeBaseName())); entry.insert("desktop",file.fileName()); entry.insert("source",flatpak.isEmpty()?"Debian / yerel":"Flatpak"); entry.insert("flatpak",flatpak);
        if(entry.value("version").toString().isEmpty()) entry["version"]="Yerel kurulum";
        result.append(entry);
    }
    const auto flatpak=QString::fromUtf8(command("flatpak",{"list","--app","--columns=application,name,version,installation,size"},7000));
    for(const auto &line:flatpak.split('\n')) { const auto f=line.split('\t'); if(f.size()<4) continue; bool found=false;
        for(auto &entry:result) { auto map=entry.toMap(); if(map.value("flatpak")==f[0]) { map["version"]=f[2]; map["installation"]=f[3]; if(f.size()>4) map["sizeText"]=f[4]; entry=map; found=true; } }
        if(!found) result.append(QVariantMap{{"id",f[0]},{"flatpak",f[0]},{"name",f[1]},{"version",f[2]},{"installation",f[3]},{"source","Flatpak"},{"icon",f[0]},{"sizeText",f.size()>4?f[4]:QString()}});
    }
    std::sort(result.begin(),result.end(),[](const auto &a,const auto &b){ return a.toMap().value("name").toString().localeAwareCompare(b.toMap().value("name").toString())<0; });
    return result;
}
QVariantMap portalPermissions(const QString &table,const QString &id) {
    const auto request=QDBusMessage::createMethodCall("org.freedesktop.impl.portal.PermissionStore","/org/freedesktop/impl/portal/PermissionStore","org.freedesktop.impl.portal.PermissionStore","Lookup");
    auto message=request; message.setArguments({table,id}); const auto reply=QDBusConnection::sessionBus().call(message,QDBus::Block,1000);
    QVariantMap permissions;
    if(reply.type()==QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        const auto argument=qvariant_cast<QDBusArgument>(reply.arguments().first()); argument.beginMap();
        while(!argument.atEnd()) { QString app; QStringList values; argument.beginMapEntry(); argument>>app>>values; argument.endMapEntry(); permissions.insert(app,values.value(0)); }
        argument.endMap();
    }
    return permissions;
}
}

DesktopSettings::DesktopSettings(bool mock,QObject *parent):QObject(parent),m_mock(mock) {
    connect(&m_query,&QFutureWatcher<QVariantMap>::finished,this,[this]{ m_state=m_query.result(); m_refreshing=false; emit changed(); if(m_refreshAgain) {m_refreshAgain=false;refresh();} });
    connect(&m_action,&QProcess::stateChanged,this,[this]{ emit changed(); });
    connect(&m_action,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus status){ m_timeout.stop(); if(code || status!=QProcess::NormalExit) { m_error=QString::fromUtf8(m_action.readAllStandardError()).trimmed().left(1500); if(m_error.isEmpty()) m_error="Sistem bu değişikliği kabul etmedi."; } refresh(); emit changed(); });
    connect(&m_action,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){ if(error==QProcess::FailedToStart) { m_timeout.stop(); m_error="Gerekli sistem aracı kurulu değil."; emit changed(); } });
    m_timeout.setSingleShot(true); m_timeout.setInterval(20000); connect(&m_timeout,&QTimer::timeout,this,[this]{m_error="İşlem zaman aşımına uğradı."; m_action.kill();});
    m_state={{"interfaces",QVariantList()},{"vpns",QVariantList()},{"outputs",QVariantList()},{"inputs",QVariantList()},{"streams",QVariantList()},{"applications",QVariantList()},{"profiles",QStringList()},{"permissions",QVariantMap()},{"cameraPermissions",QVariantMap()}};
}
DesktopSettings::~DesktopSettings() { disconnect(&m_action,nullptr,this,nullptr); if(busy()) {m_action.kill();m_action.waitForFinished(1000);} m_query.waitForFinished(); }
void DesktopSettings::refresh() { if(m_mock || busy()) return; if(refreshing()) {m_refreshAgain=true;return;} m_refreshing=true; m_query.setFuture(QtConcurrent::run(&DesktopSettings::collect)); emit changed(); }
QVariantMap DesktopSettings::collect() {
    QVariantMap state; QVariantList interfaces,vpns; quint64 received=0,sent=0; bool online=false;
    for(const auto &iface:QNetworkInterface::allInterfaces()) {
        if(iface.flags().testFlag(QNetworkInterface::IsLoopBack)) continue;
        const auto name=iface.name(); QStringList addresses; for(const auto &entry:iface.addressEntries()) if(entry.ip().protocol()==QAbstractSocket::IPv4Protocol) addresses.append(entry.ip().toString()+"/"+QString::number(entry.prefixLength()));
        const bool up=iface.flags().testFlag(QNetworkInterface::IsRunning) && !addresses.isEmpty(); online|=up;
        const auto rx=readFile("/sys/class/net/"+name+"/statistics/rx_bytes").toULongLong(),tx=readFile("/sys/class/net/"+name+"/statistics/tx_bytes").toULongLong();
        const bool virtualDevice=QRegularExpression("^(lo|tun|tap|wg|docker|veth|br-)").match(name).hasMatch();
        if(!virtualDevice) { received+=rx; sent+=tx; }
        interfaces.append(QVariantMap{{"name",name},{"type",iface.type()==QNetworkInterface::Wifi?"Wi-Fi":virtualDevice?"Sanal":"Ethernet"},{"connected",up},{"address",addresses.join(" · ")},{"mac",iface.hardwareAddress()},{"mtu",iface.maximumTransmissionUnit()},{"received",qulonglong(rx)},{"sent",qulonglong(tx)}});
    }
    const QString connectivity=QString::fromUtf8(command("nmcli",{"-t","-f","CONNECTIVITY","general"})).trimmed();
    state["connectivity"]=connectivity; state["connected"]=online; state["received"]=qulonglong(received); state["sent"]=qulonglong(sent); state["interfaces"]=interfaces;
    for(const auto &line:QString::fromUtf8(command("nmcli",{"-t","-f","NAME,UUID,TYPE,DEVICE","connection","show"})).split('\n',Qt::SkipEmptyParts)) {
        const auto fields=SettingsData::splitFields(line); if(fields.size()<4) continue;
        if(fields[2]=="vpn" || fields[2]=="wireguard") vpns.append(QVariantMap{{"name",fields[0]},{"uuid",fields[1]},{"type",fields[2]},{"active",!fields[3].isEmpty() && fields[3]!="--"}});
    }
    state["vpns"]=vpns; state["networkManager"]=!command("nmcli",{"-t","-f","RUNNING","general"}).trimmed().isEmpty();
    const QString sink=QString::fromUtf8(command("pactl",{"get-default-sink"})).trimmed(),source=QString::fromUtf8(command("pactl",{"get-default-source"})).trimmed();
    state["outputs"]=devices(command("pactl",{"--format=json","list","sinks"}),sink,false);
    state["inputs"]=devices(command("pactl",{"--format=json","list","sources"}),source,true);
    QVariantList streams; for(const auto &value:QJsonDocument::fromJson(command("pactl",{"--format=json","list","sink-inputs"})).array()) { const auto object=value.toObject(); const auto props=object.value("properties").toObject(); const auto volumes=object.value("volume").toObject(); int volume=0,count=0; for(auto it=volumes.begin();it!=volumes.end();++it) {volume+=qRound(it.value().toObject().value("value").toDouble()*100/65536);++count;} streams.append(QVariantMap{{"id",object.value("index").toInt()},{"name",props.value("application.name").toString("Ses akışı")},{"volume",count?volume/count:0}}); } state["streams"]=streams;
    QStringList profiles; for(const auto &line:QString::fromUtf8(command("powerprofilesctl",{"list"})).split('\n')) { const auto match=QRegularExpression("^\\s*\\*?\\s*(power-saver|balanced|performance):").match(line); if(match.hasMatch()) profiles.append(match.captured(1)); }
    state["profiles"]=profiles; state["profile"]=QString::fromUtf8(command("powerprofilesctl",{"get"})).trimmed();
    QVariantList batteries; for(const auto &supply:QDir("/sys/class/power_supply").entryList(QDir::Dirs|QDir::NoDotAndDotDot)) if(readFile("/sys/class/power_supply/"+supply+"/type")=="Battery") batteries.append(QVariantMap{{"name",supply},{"capacity",readFile("/sys/class/power_supply/"+supply+"/capacity").toInt()},{"status",readFile("/sys/class/power_supply/"+supply+"/status")}}); state["batteries"]=batteries;
    state["applications"]=readApplications(); QVariantMap permissions;
    for(const auto &entry:state["applications"].toList()) { const auto app=entry.toMap().value("flatpak").toString(); if(app.isEmpty() || permissions.contains(app)) continue;
        const auto text=QString::fromUtf8(command("flatpak",{"info","--show-permissions",app})); QVariantMap values{{"available",!text.isEmpty()}}; QString section;
        for(const auto &line:text.split('\n')) { if(line.startsWith('[')) section=line; if(section!="[Context]") continue; const auto key=line.section('=',0,0),value=line.section('=',1); if(key=="shared") values["network"]=value.split(';').contains("network"); if(key=="sockets") values["audio"]=value.split(';').contains("pulseaudio"); if(key=="devices") values["devices"]=value.split(';').contains("all"); if(key=="filesystems") { QStringList grants; for(const auto &v:value.split(';',Qt::SkipEmptyParts)) if(!v.startsWith('!')) grants.append(v); values["files"]=!grants.isEmpty(); values["filesystemSummary"]=grants.join(" · "); } }
        permissions.insert(app,values);
    } state["permissions"]=permissions;
    state["cameraPermissions"]=portalPermissions("devices","camera");
    state["microphonePermissions"]=portalPermissions("devices","microphone");
    state["locationPermissions"]=portalPermissions("location","location");
    state["notificationPermissions"]=portalPermissions("notifications","notification");
    state["backgroundPermissions"]=portalPermissions("background","background");
    const auto disks=QJsonDocument::fromJson(command("lsblk",{"--json","-o","NAME,TYPE,MOUNTPOINTS,FSTYPE"})).object(); bool rootSeen=false,rootEncrypted=false;
    std::function<void(QJsonObject,bool)> scan=[&](QJsonObject object,bool encrypted){ encrypted|=object.value("type").toString()=="crypt" || object.value("fstype").toString()=="crypto_LUKS"; for(const auto &mount:object.value("mountpoints").toArray()) if(mount.toString()=="/") {rootSeen=true; rootEncrypted=encrypted;} for(const auto &child:object.value("children").toArray()) scan(child.toObject(),encrypted); };
    for(const auto &disk:disks.value("blockdevices").toArray()) scan(disk.toObject(),false);
    state["encryption"]=rootSeen?(rootEncrypted?"encrypted":"unencrypted"):"unknown";
    state["flatpakAvailable"]=!QStandardPaths::findExecutable("flatpak").isEmpty();
    return state;
}
bool DesktopSettings::known(const QString &list,const QString &field,const QVariant &value) const { for(const auto &entry:m_state.value(list).toList()) if(entry.toMap().value(field)==value) return true; return false; }
void DesktopSettings::action(const QString &program,const QStringList &args) { if(m_mock || busy()) return; m_error.clear(); m_action.start(program,args); m_timeout.start(); emit changed(); }
void DesktopSettings::setPowerProfile(const QString &profile) { if(m_state.value("profiles").toStringList().contains(profile)) action("powerprofilesctl",{"set",profile}); }
void DesktopSettings::setAudioDefault(const QString &kind,const QString &name) { if((kind=="sink" || kind=="source") && known(kind=="sink"?"outputs":"inputs","name",name)) action("pactl",{"set-default-"+kind,name}); }
void DesktopSettings::setAudioVolume(const QString &kind,const QString &name,int value) { if((kind=="sink" || kind=="source") && value>=0 && value<=100 && known(kind=="sink"?"outputs":"inputs","name",name)) action("pactl",{"set-"+kind+"-volume",name,QString::number(value)+"%"}); }
void DesktopSettings::setAudioMute(const QString &kind,const QString &name,bool muted) { if((kind=="sink" || kind=="source") && known(kind=="sink"?"outputs":"inputs","name",name)) action("pactl",{"set-"+kind+"-mute",name,muted?"1":"0"}); }
void DesktopSettings::setAudioPort(const QString &kind,const QString &name,const QString &port) { if(kind!="sink" && kind!="source") return; for(const auto &entry:m_state.value(kind=="sink"?"outputs":"inputs").toList()) if(entry.toMap().value("name")==name) for(const auto &p:entry.toMap().value("ports").toList()) if(p.toMap().value("name")==port && p.toMap().value("available").toBool()) {action("pactl",{"set-"+kind+"-port",name,port});return;} }
void DesktopSettings::setApplicationVolume(int id,int value) { if(value>=0 && value<=100 && known("streams","id",id)) action("pactl",{"set-sink-input-volume",QString::number(id),QString::number(value)+"%"}); }
void DesktopSettings::activateConnection(const QString &uuid,bool active) { if(known("vpns","uuid",uuid)) action("nmcli",{"--wait","15","connection",active?"up":"down","uuid",uuid}); }
QVariantMap DesktopSettings::input() const { QSettings settings(configPath(),QSettings::IniFormat); return {{"pointerSpeed",settings.value("pointerSpeed",0)},{"naturalScroll",settings.value("naturalScroll",false)},{"leftHanded",settings.value("leftHanded",false)},{"tap",settings.value("tap",true)},{"repeatRate",settings.value("repeatRate",25)},{"repeatDelay",settings.value("repeatDelay",600)}}; }
void DesktopSettings::setInput(const QString &key,const QVariant &value) {
    if(m_mock || busy()) return;
    QVariant checked;
    if(QStringList{"naturalScroll","leftHanded","tap"}.contains(key)) checked=value.toBool();
    else if(key=="pointerSpeed" && value.toDouble()>=-1 && value.toDouble()<=1) checked=value.toDouble();
    else if(key=="repeatRate" && value.toInt()>=1 && value.toInt()<=80) checked=value.toInt();
    else if(key=="repeatDelay" && value.toInt()>=200 && value.toInt()<=1500) checked=value.toInt();
    else return;
    QSettings settings(configPath(),QSettings::IniFormat); settings.setValue(key,checked); settings.sync();
    action("fidan-input",{"apply"}); emit changed();
}
QVariantMap DesktopSettings::permissions(const QString &app) const {
    auto values=m_state.value("permissions").toMap().value(app).toMap();
    for(const auto &key:{"camera","microphone","location","notification","background"}) values[key]=m_state.value(QString(key)+"Permissions").toMap().value(app,"ask");
    return values;
}
void DesktopSettings::setFlatpakPermission(const QString &app,const QString &permission,bool allowed) {
    if(!known("applications","flatpak",app) || !QRegularExpression("\\A[A-Za-z0-9_]+(?:\\.[A-Za-z0-9_-]+){2,}\\z").match(app).hasMatch()) return;
    const QMap<QString,QStringList> options{{"network",{allowed?"--share=network":"--unshare=network"}},{"audio",{allowed?"--socket=pulseaudio":"--nosocket=pulseaudio"}},{"devices",{allowed?"--device=all":"--nodevice=all"}},{"files",{allowed?"--filesystem=home":"--nofilesystem=host:reset"}}};
    if(!options.contains(permission)) return;
    QStringList args{"override","--user"}; args.append(options.value(permission)); args.append(app); action("flatpak",args);
}
void DesktopSettings::setCameraPermission(const QString &app,const QString &decision) {
    setPortalPermission(app,"camera",decision);
}
void DesktopSettings::setPortalPermission(const QString &app,const QString &resource,const QString &decision) {
    if(!known("applications","flatpak",app) || !QRegularExpression("\\A[A-Za-z0-9_]+(?:\\.[A-Za-z0-9_-]+){2,}\\z").match(app).hasMatch()) return;
    if(decision=="ask" && permissions(app).value(resource).toString()=="ask") return;
    const QMap<QString,QStringList> resources{{"camera",{"devices","camera"}},{"microphone",{"devices","microphone"}},{"location",{"location","location"}},{"notification",{"notifications","notification"}},{"background",{"background","background"}}};
    if(!resources.contains(resource) || !QStringList{"ask","yes","no"}.contains(decision)) return;
    auto args=resources.value(resource); args.append(app);
    if(decision=="ask") args.prepend("permission-remove");
    else { args.prepend("permission-set"); if(resource=="location") args.append({decision=="yes"?"EXACT":"NONE","0"}); else args.append(decision); }
    action("flatpak",args);
}
