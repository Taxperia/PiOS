// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Control {
    id: root
    objectName: "deviceWelcomeWizard"
    property int step: 0
    property bool restored: false
    property bool finished: false
    readonly property color ink: Controller.darkTheme ? Controller.themeColor("#F1F3FA", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#202638", Controller.themeMode, Controller.darkTheme)
    readonly property color muted: Controller.darkTheme ? Controller.themeColor("#A5ADC3", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#626C83", Controller.themeMode, Controller.darkTheme)
    readonly property color accent: Controller.themeColor("#A8B8FF", Controller.themeMode, Controller.darkTheme)
    readonly property bool wide: width >= 780
    readonly property var titles: [qsTr("Welcome to PiOS"),qsTr("Language and region"),qsTr("Create your account"),qsTr("Connect to a network"),qsTr("Make it yours"),qsTr("Privacy and access"),qsTr("Stay up to date"),qsTr("Your desktop is ready")]
    readonly property bool accountValid: /^[a-z][a-z0-9_-]{1,30}$/.test(username.text) && displayName.text.length <= 80
        && ((DeviceSetup.state.accountReady && password.text.length === 0 && displayName.text === DeviceSetup.state.displayName)
            || (password.text.length >= 8 && password.text === confirmation.text))
    palette.window: Controller.darkTheme ? Controller.themeColor("#1C2133", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#F7F8FC", Controller.themeMode, Controller.darkTheme)
    palette.windowText: ink; palette.text: ink; palette.buttonText: ink
    palette.button: Controller.darkTheme ? Controller.themeColor("#2B3249", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#E8ECF6", Controller.themeMode, Controller.darkTheme)
    palette.base: Controller.darkTheme ? Controller.themeColor("#242A3F", Controller.themeMode, Controller.darkTheme) : "white"
    palette.highlight: accent; palette.placeholderText: muted
    background: Rectangle { radius: 22; color: root.palette.window; border.color: Controller.darkTheme ? Controller.themeColor("#41485E", Controller.themeMode, Controller.darkTheme) : Controller.themeColor("#D9DEEA", Controller.themeMode, Controller.darkTheme) }
    function preferences() {
        return {themeMode:Controller.themeMode,accentColor:Controller.accentColor,dockPosition:Controller.dockPosition,
            panelPosition:Controller.panelPosition,panelStyle:Controller.panelStyle,dateFormat:Controller.dateFormat,
            language:Controller.sessionLanguage,keyboardLayout:Controller.keyboardLayout,reducedMotion:Controller.reducedMotion}
    }
    function next() {
        if (step===0) step=1
        else if (step===1) DeviceSetup.setLocale(language.currentIndex===0?"tr_TR.UTF-8":"en_US.UTF-8",keyboard.currentIndex===0?"trq":"us",timezone.currentText)
        else if (step===2) {
            if (DeviceSetup.state.accountReady && password.text.length===0) {step=3;DeviceSetup.scanWifi()}
            else DeviceSetup.createUser(username.text,displayName.text,password.text)
        } else if (step===3) {
            if (ssid.text.length) DeviceSetup.connectWifi(ssid.text,wifiPassword.text); else DeviceSetup.skipNetwork()
        } else if (step===4) DeviceSetup.setDesktopPreferences(preferences())
        else if (step===5) DeviceSetup.setPrivacy(enableSsh.checked,false,enableRemote.checked)
        else if (step===6) step=7
        else DeviceSetup.complete()
    }
    Component.onCompleted: DeviceSetup.refresh()
    Connections {
        target: DeviceSetup
        function onStateChanged() {
            const state=DeviceSetup.state
            if (root.restored || state.error || !Object.keys(state).length) return
            root.restored=true
            if (state.locale) {language.currentIndex=state.locale.startsWith("tr")?0:1;Controller.sessionLanguage=state.locale;DeviceSetup.setLanguage(state.locale)}
            if (state.keymap) {keyboard.currentIndex=state.keymap==="trq"?0:1;Controller.keyboardLayout=state.keymap==="trq"?"tr":"us"}
            if (state.timezone) timezone.currentIndex=Math.max(0,timezone.model.indexOf(state.timezone))
            username.text=state.username||"";displayName.text=state.displayName||"";ssid.text=state.ssid||""
            enableSsh.checked=!!state.sshEnabled;enableRemote.checked=!!state.remoteEnabled
            const p=state.desktopPreferences||{}
            for (const key of Object.keys(p)) Controller[key==="language"?"sessionLanguage":key]=p[key]
            if (state.complete) {root.step=7;root.finished=true}
            else if (!state.localeReady && !state.username) root.step=0
            else if (state.accountReady && state.networkDone && !state.desktopPreferencesReady) root.step=4
            else root.step=({1:1,2:2,3:3,4:5,5:6,6:7})[state.step]||0
        }
        function onOperationFinished(operation,success) {
            if (!success) return
            if (operation==="SetLocale") root.step=2
            else if (operation==="CreateUser") {password.clear();confirmation.clear();root.step=3;DeviceSetup.scanWifi()}
            else if (operation==="SkipNetwork" || operation==="ConnectWifi") {wifiPassword.clear();root.step=4}
            else if (operation==="SetDesktopPreferences") root.step=5
            else if (operation==="SetPrivacy") root.step=6
            else if (operation==="SkipUpdates") root.step=7
            else if (operation==="Complete") root.finished=true
        }
    }
    Rectangle {
        visible: root.wide; anchors.left:parent.left;anchors.top:parent.top;anchors.bottom:parent.bottom;anchors.margins:1
        width:235;radius:21;color:Controller.darkTheme?Controller.themeColor("#252D46", Controller.themeMode, Controller.darkTheme):Controller.themeColor("#EDF0F9", Controller.themeMode, Controller.darkTheme)
        ColumnLayout {
            anchors.fill:parent;anchors.margins:26;spacing:12
            Label {text:"PiOS";color:root.ink;font.pixelSize:27;font.bold:true;Layout.bottomMargin:16}
            Repeater { model:root.titles
                RowLayout {
                    required property int index
                    required property string modelData
                    Layout.fillWidth:true;spacing:10
                    Rectangle {width:25;height:25;radius:13;color:index===root.step?root.accent:"transparent";border.color:index===root.step?root.accent:root.muted
                        Label {anchors.centerIn:parent;text:index<root.step?"✓":String(index+1);font.pixelSize:11;color:index===root.step?Controller.themeColor("#1D2947", Controller.themeMode, Controller.darkTheme):root.muted}
                    }
                    Label {Layout.fillWidth:true;text:modelData;color:root.ink;wrapMode:Text.WordWrap;font.pixelSize:12}
                }
            }
            Item {Layout.fillHeight:true}
            Label {Layout.fillWidth:true;text:qsTr("Your progress is saved automatically.");color:root.muted;wrapMode:Text.WordWrap;font.pixelSize:11}
        }
    }
    ColumnLayout {
        anchors.fill:parent;anchors.margins:Math.min(32,root.width/18);anchors.leftMargin:root.wide?267:Math.min(32,root.width/18);spacing:15
        RowLayout {Layout.fillWidth:true;Label {visible:!root.wide;text:"PiOS";color:root.ink;font.pixelSize:25;font.bold:true} Item {Layout.fillWidth:true} Label {text:qsTr("%1 / %2").arg(root.step+1).arg(root.titles.length);color:root.muted}}
        ProgressBar {Layout.fillWidth:true;from:0;to:7;value:root.step}
        Label {Layout.fillWidth:true;text:root.titles[root.step];font.pixelSize:28;font.bold:true;color:root.ink;wrapMode:Text.WordWrap}
        ScrollView {
            Layout.fillWidth:true;Layout.fillHeight:true;clip:true;contentWidth:availableWidth
            StackLayout {
                width:parent.width;currentIndex:root.step
                ColumnLayout {
                    spacing:20
                    Label {Layout.fillWidth:true;text:qsTr("Press Next to finish setting up your operating system.");color:root.ink;wrapMode:Text.WordWrap;font.pixelSize:18}
                    Label {Layout.fillWidth:true;text:qsTr("Create your account and choose how your desktop looks and works here.");color:root.muted;wrapMode:Text.WordWrap}
                }
                ColumnLayout {
                    spacing:10
                    Label {text:qsTr("System language");color:root.ink}
                    ComboBox {id:language;objectName:"deviceLanguage";Layout.fillWidth:true;model:["Türkçe","English"];currentIndex:Controller.sessionLanguage.startsWith("tr")?0:1;onActivated:{Controller.sessionLanguage=currentIndex===0?"tr_TR.UTF-8":"en_US.UTF-8";DeviceSetup.setLanguage(Controller.sessionLanguage)}}
                    Label {text:qsTr("Keyboard layout");color:root.ink}
                    ComboBox {id:keyboard;Layout.fillWidth:true;model:["Türkçe Q","English US"];currentIndex:Controller.keyboardLayout==="tr"?0:1;onActivated:Controller.keyboardLayout=currentIndex===0?"tr":"us"}
                    Label {text:qsTr("Time zone");color:root.ink}
                    ComboBox {id:timezone;Layout.fillWidth:true;model:["Europe/Istanbul","Europe/London","UTC"]}
                    Label {text:qsTr("Date format");color:root.ink}
                    ComboBox {Layout.fillWidth:true;model:[qsTr("Follow system locale"),"2026-11-14","14.11.2026"];currentIndex:["locale","iso","day-first"].indexOf(Controller.dateFormat);onActivated:Controller.dateFormat=["locale","iso","day-first"][currentIndex]}
                    TextField {Layout.fillWidth:true;placeholderText:qsTr("Test your keyboard here")}
                }
                ColumnLayout {
                    spacing:10
                    TextField {id:displayName;objectName:"deviceDisplayName";Layout.fillWidth:true;placeholderText:qsTr("Your name");maximumLength:80}
                    TextField {id:username;objectName:"deviceUsername";Layout.fillWidth:true;placeholderText:qsTr("Username (lowercase letters and numbers)");maximumLength:31;readOnly:!!DeviceSetup.state.username}
                    TextField {id:password;objectName:"devicePassword";Layout.fillWidth:true;placeholderText:qsTr("Password (at least 8 characters)");maximumLength:256;echoMode:TextInput.Password}
                    TextField {id:confirmation;objectName:"devicePasswordConfirmation";Layout.fillWidth:true;placeholderText:qsTr("Repeat password");maximumLength:256;echoMode:TextInput.Password}
                    Label {Layout.fillWidth:true;text:qsTr("Passwords are never saved in setup history.");wrapMode:Text.WordWrap;color:root.muted}
                }
                ColumnLayout {
                    spacing:10
                    RowLayout {Layout.fillWidth:true;ComboBox {Layout.fillWidth:true;model:DeviceSetup.networks;onActivated:ssid.text=currentText} Button {text:qsTr("Scan again");enabled:!DeviceSetup.busy;onClicked:DeviceSetup.scanWifi()}}
                    TextField {id:ssid;Layout.fillWidth:true;placeholderText:qsTr("Network name");maximumLength:32}
                    TextField {id:wifiPassword;Layout.fillWidth:true;placeholderText:qsTr("Wi-Fi password");maximumLength:63;echoMode:TextInput.Password}
                    Label {Layout.fillWidth:true;text:qsTr("Ethernet works without this step. You can continue offline and connect later.");wrapMode:Text.WordWrap;color:root.muted}
                    Button {objectName:"deviceSkipNetwork";text:qsTr("Continue without Wi-Fi");enabled:!DeviceSetup.busy;onClicked:DeviceSetup.skipNetwork()}
                }
                ColumnLayout {
                    spacing:10
                    Label {text:qsTr("Theme");color:root.ink}
                    RowLayout {Repeater {model:[{id:"light",name:qsTr("Light")},{id:"dark",name:"Dark"},{id:"green",name:"Green"},{id:"auto",name:qsTr("Automatic")}]
                        Button {required property var modelData;text:modelData.name;checkable:true;checked:Controller.themeMode===modelData.id;onClicked:Controller.themeMode=modelData.id}
                    }}
                    Label {text:qsTr("Accent color");color:root.ink}
                    Flow {Layout.fillWidth:true;spacing:8;Repeater {model:["#63de8a","#2589ff","#ac55ef","#ed449a","#ff9546","#ffd35d","#ff575e","#24c5cc"]
                        Button {required property string modelData;width:40;height:40;onClicked:Controller.accentColor=modelData;Accessible.name:qsTr("Accent color %1").arg(modelData);background:Rectangle {color:parent.modelData;radius:20;border.width:Controller.accentColor===parent.modelData?3:0;border.color:root.ink}}
                    }}
                    Label {text:qsTr("Dock position");color:root.ink}
                    ComboBox {Layout.fillWidth:true;model:[qsTr("Left"),qsTr("Bottom"),qsTr("Right")];currentIndex:["left","bottom","right"].indexOf(Controller.dockPosition);onActivated:Controller.dockPosition=["left","bottom","right"][currentIndex]}
                    Label {text:qsTr("Panel position");color:root.ink}
                    ComboBox {Layout.fillWidth:true;model:[qsTr("Top"),qsTr("Bottom")];currentIndex:Controller.panelPosition==="top"?0:1;onActivated:Controller.panelPosition=currentIndex===0?"top":"bottom"}
                    Label {text:qsTr("Panel design");color:root.ink}
                    ComboBox {Layout.fillWidth:true;model:[qsTr("Detailed"),qsTr("Compact")];currentIndex:Controller.panelStyle==="detailed"?0:1;onActivated:Controller.panelStyle=currentIndex===0?"detailed":"compact"}
                    CheckBox {text:qsTr("Reduce animations");checked:Controller.reducedMotion;onToggled:Controller.reducedMotion=checked}
                }
                ColumnLayout {
                    spacing:12
                    CheckBox {id:enableSsh;objectName:"deviceSsh";text:qsTr("Enable SSH access")}
                    CheckBox {id:enableRemote;objectName:"deviceRemote";text:qsTr("Enable the browser desktop on my local network")}
                    Label {Layout.fillWidth:true;text:qsTr("Enable browser access if you want to reconnect after setup using your new account.");wrapMode:Text.WordWrap;color:root.muted}
                    CheckBox {text:qsTr("Share diagnostic reports (not available yet)");enabled:false;checked:false}
                    Label {Layout.fillWidth:true;text:qsTr("No location is requested. Application permissions can be reviewed later in Settings.");wrapMode:Text.WordWrap;color:root.muted}
                }
                ColumnLayout {
                    spacing:12
                    Label {Layout.fillWidth:true;text:qsTr("Download signed PiOS updates now, or finish without updating.");wrapMode:Text.WordWrap;color:root.muted}
                    Button {text:qsTr("Check for updates");enabled:!DeviceSetup.busy;onClicked:{const u=DeviceSetup.state.update||{};DeviceSetup.startUpdate(u.downloaded?"install":u.updateAvailable&&u.installable?"download":"check")}}
                    Button {objectName:"deviceSkipUpdates";text:qsTr("Continue without updating");enabled:!DeviceSetup.busy;onClicked:DeviceSetup.skipUpdates()}
                    Label {Layout.fillWidth:true;text:DeviceSetup.state.updateError||"";wrapMode:Text.WordWrap;color:root.muted}
                }
                ColumnLayout {
                    spacing:20
                    Label {Layout.fillWidth:true;text:root.finished?qsTr("Setup is complete. The device is restarting."):qsTr("Your account and desktop preferences are ready. Finish to restart and sign in.");wrapMode:Text.WordWrap;color:root.ink}
                }
            }
        }
        Label {objectName:"deviceSetupError";Layout.fillWidth:true;visible:DeviceSetup.error.length>0;text:DeviceSetup.error;color:Controller.themeColor("#D65C52", Controller.themeMode, Controller.darkTheme);wrapMode:Text.WordWrap}
        RowLayout {
            Layout.fillWidth:true
            Button {text:qsTr("Back");visible:root.step>0&&!root.finished;enabled:!DeviceSetup.busy;onClicked:root.step--}
            BusyIndicator {running:DeviceSetup.busy;visible:running;Layout.preferredWidth:28;Layout.preferredHeight:28}
            Item {Layout.fillWidth:true}
            Button {objectName:"deviceWelcomeNext";text:root.step===7?qsTr("Finish and restart"):qsTr("Next");font.bold:true;palette.button:root.accent;palette.buttonText:Controller.themeColor("#1D2947", Controller.themeMode, Controller.darkTheme)
                enabled:!DeviceSetup.busy&&!root.finished&&(root.step!==2||root.accountValid)&&(root.step!==6||!!DeviceSetup.state.updateDone)
                onClicked:root.next()
            }
        }
    }
}
