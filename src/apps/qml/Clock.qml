// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: root
    objectName: "clockPage"
    readonly property color foreground: UiStyle.color("#F2F7F4",UiStyle.dark)
    property int tab: 0
    property bool finished: false
    function duration(ms) { const seconds=Math.floor(ms/1000); return String(Math.floor(seconds/3600)).padStart(2,"0")+":"+String(Math.floor(seconds/60)%60).padStart(2,"0")+":"+String(seconds%60).padStart(2,"0") }
    Connections { target: ClockData; function onTimerFinished() { root.finished=true } }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 28; spacing: 20
        Text { text: "Saat"; color: root.foreground; font.pixelSize: 28; font.weight: Font.DemiBold }
        RowLayout {
            spacing: 8
            Repeater { model: ["Dünya saati","Kronometre","Sayaç"]; delegate: AppButton { required property int index; required property string modelData; text: modelData; accent: root.tab === index; onClicked: root.tab=index } }
            Item { Layout.fillWidth: true }
        }
        StackLayout {
            currentIndex: root.tab; Layout.fillWidth: true; Layout.fillHeight: true
            ColumnLayout {
                spacing: 20
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 170; radius: 15; color: UiStyle.color("#17231F",UiStyle.dark); border.color: UiStyle.color("#465750",UiStyle.dark)
                    Column { anchors.centerIn: parent; spacing: 8; Text { text: "YEREL SAAT"; color: UiStyle.color("#63DE8A",UiStyle.dark); font.pixelSize: 11; anchors.horizontalCenter: parent.horizontalCenter } Text { text: ClockData.time; color: root.foreground; font.pixelSize: 62; font.weight: Font.Light; anchors.horizontalCenter: parent.horizontalCenter } Text { text: ClockData.date; color: UiStyle.color("#9FB2A8",UiStyle.dark); font.pixelSize: 14; anchors.horizontalCenter: parent.horizontalCenter } }
                }
                GridLayout {
                    columns: 3; columnSpacing: 12; rowSpacing: 12; Layout.fillWidth: true; Layout.fillHeight: true
                    Repeater {
                        model: ClockData.cities
                        delegate: Rectangle {
                            required property var modelData; Layout.fillWidth: true; Layout.fillHeight: true; radius: 12; color: UiStyle.color("#111C19",UiStyle.dark)
                            Column { anchors.fill: parent; anchors.margins: 18; spacing: 8; Text { text: modelData.name; color: root.foreground; font.pixelSize: 15 } Text { text: modelData.time; color: root.foreground; font.pixelSize: 32 } Text { text: modelData.date+" · "+modelData.zone; color: UiStyle.color("#9FB2A8",UiStyle.dark); font.pixelSize: 11 } }
                        }
                    }
                }
            }
            ColumnLayout {
                spacing: 20
                Item { Layout.fillHeight: true }
                Text { Layout.alignment: Qt.AlignHCenter; text: root.duration(ClockData.elapsed); color: root.foreground; font.pixelSize: 68; font.weight: Font.Light }
                Text { Layout.alignment: Qt.AlignHCenter; text: String(Math.floor(ClockData.elapsed%1000/10)).padStart(2,"0"); color: UiStyle.color("#63DE8A",UiStyle.dark); font.pixelSize: 30 }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter; spacing: 12
                    AppButton { text: ClockData.running ? "Duraklat" : "Başlat"; accent: true; onClicked: ClockData.toggleStopwatch() }
                    AppButton { text: "Tur"; enabled: ClockData.running; onClicked: ClockData.lap() }
                    AppButton { text: "Sıfırla"; onClicked: ClockData.resetStopwatch() }
                }
                ListView { Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: ClockData.laps; delegate: ItemDelegate { required property int index; required property var modelData; width: ListView.view.width; text: "Tur "+(ClockData.laps.length-index)+"                                      "+root.duration(modelData)+"."+String(Math.floor(modelData%1000/10)).padStart(2,"0") } ScrollBar.vertical: ScrollBar {} }
            }
            ColumnLayout {
                Item { Layout.fillHeight: true }
                Text { Layout.alignment: Qt.AlignHCenter; text: root.finished ? "Süre doldu" : root.duration(ClockData.remaining*1000); color: root.finished ? UiStyle.accent : palette.text; font.pixelSize: 64; font.weight: Font.Light }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter; visible: ClockData.remaining === 0; spacing: 10
                    SpinBox { id: hours; from: 0; to: 23; editable: true; Accessible.name: "Saat" }
                    Text { text: "sa"; color: root.foreground }
                    SpinBox { id: minutes; from: 0; to: 59; value: 5; editable: true; Accessible.name: "Dakika" }
                    Text { text: "dk"; color: root.foreground }
                    SpinBox { id: seconds; from: 0; to: 59; editable: true; Accessible.name: "Saniye" }
                    Text { text: "sn"; color: root.foreground }
                }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter; spacing: 12
                    AppButton { text: ClockData.counting ? "Duraklat" : ClockData.remaining > 0 ? "Devam et" : "Başlat"; accent: true; enabled: ClockData.remaining > 0 || hours.value+minutes.value+seconds.value > 0; onClicked: { root.finished=false; if(ClockData.counting) ClockData.pauseTimer(); else if(ClockData.remaining>0) ClockData.resumeTimer(); else ClockData.startTimer(hours.value*3600+minutes.value*60+seconds.value) } }
                    AppButton { text: "İptal"; onClicked: { ClockData.cancelTimer(); root.finished=false } }
                }
                Text { Layout.alignment: Qt.AlignHCenter; Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: "Sayaç çalışırken uygulamayı açık veya küçültülmüş tut.\nBilgisayarın uyuması bildirimi geciktirebilir."; color: UiStyle.color("#9FB2A8",UiStyle.dark); font.pixelSize: 12; wrapMode: Text.Wrap }
                Item { Layout.fillHeight: true }
            }
        }
    }
}
