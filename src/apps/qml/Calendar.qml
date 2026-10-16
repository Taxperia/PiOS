// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: root
    objectName: "calendarPage"
    readonly property color foreground: UiStyle.color("#F2F7F4",UiStyle.dark)
    property date monthDate: new Date()
    property string selectedDate: CalendarData.today
    property string editingId: ""
    property string editorError: ""
    function shiftMonth(direction) { monthDate=new Date(monthDate.getFullYear(),monthDate.getMonth()+direction,1) }
    function editEvent(event) { editingId=event ? event.id : ""; eventTitle.text=event ? event.title : ""; eventTime.text=event ? event.time : ""; eventDate.text=event ? event.date : selectedDate; editorError=""; editor.open() }
    RowLayout {
        anchors.fill: parent; anchors.margins: 24; spacing: 22
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
            Text { text: "Takvim"; color: root.foreground; font.pixelSize: 28; font.weight: Font.DemiBold }
            RowLayout {
                Text { text: root.monthDate.toLocaleDateString(Qt.locale("tr_TR"),"MMMM yyyy").toLocaleUpperCase("tr-TR"); color: root.foreground; font.pixelSize: 19; Layout.fillWidth: true }
                AppButton { text: "Bugün"; onClicked: { root.monthDate=new Date(); root.selectedDate=CalendarData.today } }
                AppButton { text: "‹"; tip: "Önceki ay"; onClicked: root.shiftMonth(-1) }
                AppButton { text: "›"; tip: "Sonraki ay"; onClicked: root.shiftMonth(1) }
            }
            GridLayout {
                columns: 7; columnSpacing: 4; rowSpacing: 4; Layout.fillWidth: true; Layout.fillHeight: true
                Repeater { model: ["Pzt","Sal","Çar","Per","Cum","Cmt","Paz"]; delegate: Text { required property string modelData; text: modelData; color: UiStyle.color("#9FB2A8",UiStyle.dark); horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true; Layout.preferredHeight: 24; font.pixelSize: 11 } }
                Repeater {
                    model: { const revision=CalendarData.revision; return CalendarData.month(root.monthDate.getFullYear(),root.monthDate.getMonth()+1) }
                    delegate: Button {
                        id: cell; required property var modelData
                        objectName: "day"+modelData.date
                        Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 40
                        Accessible.name: CalendarData.dateLabel(modelData.date)
                        contentItem: Item {
                            Text { anchors.centerIn: parent; text: cell.modelData.day; font.pixelSize: 17; font.weight: cell.modelData.today ? Font.Bold : Font.Normal; color: root.selectedDate === cell.modelData.date ? "#082C17" : cell.modelData.current ? root.foreground : UiStyle.color("#71847A",UiStyle.dark) }
                            Rectangle { anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: 6; width: 5; height: 5; radius: 3; color: root.selectedDate === cell.modelData.date ? "#082C17" : UiStyle.accent; visible: cell.modelData.count > 0 }
                        }
                        background: Rectangle { radius: 10; color: root.selectedDate === cell.modelData.date ? UiStyle.accent : cell.hovered ? UiStyle.color("#304039",UiStyle.dark) : UiStyle.color("#111C19",UiStyle.dark); border.width: cell.modelData.today ? 1 : 0; border.color: UiStyle.accent }
                        onClicked: root.selectedDate=modelData.date
                        onDoubleClicked: root.editEvent(null)
                    }
                }
            }
            Text { text: "Bir gün seç; etkinlik eklemek için çift tıkla."; color: UiStyle.color("#9FB2A8",UiStyle.dark); font.pixelSize: 11 }
        }
        Rectangle {
            Layout.preferredWidth: 265; Layout.fillHeight: true; color: UiStyle.color("#17231F",UiStyle.dark); radius: 14
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 18; spacing: 14
                Text { text: CalendarData.dateLabel(root.selectedDate); color: root.foreground; font.pixelSize: 18; wrapMode: Text.Wrap; Layout.fillWidth: true }
                Text { text: "GÜNÜN PLANI"; color: UiStyle.color("#63DE8A",UiStyle.dark); font.pixelSize: 10 }
                AppButton { text: "Etkinlik ekle"; iconName: "list-add"; accent: true; Layout.fillWidth: true; onClicked: root.editEvent(null) }
                Text { visible: events.count === 0; text: "Bugün için etkinlik yok.\nKendine biraz zaman ayır."; color: UiStyle.color("#9FB2A8",UiStyle.dark); font.pixelSize: 13; Layout.fillWidth: true; wrapMode: Text.Wrap }
                ListView {
                    id: events; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 8
                    model: { const revision=CalendarData.revision; return CalendarData.events(root.selectedDate) }
                    delegate: ItemDelegate {
                        id: eventItem; required property var modelData; width: ListView.view.width; height: 80
                        contentItem: Column { spacing: 7; Text { text: eventItem.modelData.time.length ? eventItem.modelData.time : "Tüm gün"; color: UiStyle.color("#63DE8A",UiStyle.dark); font.pixelSize: 12 } Text { width: parent.width; text: eventItem.modelData.title; color: root.foreground; font.pixelSize: 14; elide: Text.ElideRight } }
                        background: Rectangle { radius: 8; color: eventItem.hovered ? UiStyle.color("#304039",UiStyle.dark) : UiStyle.color("#111C19",UiStyle.dark) }
                        onClicked: root.editEvent(modelData)
                    }
                    ScrollBar.vertical: ScrollBar {}
                }
                Text { visible: CalendarData.error.length > 0; text: CalendarData.error; color: UiStyle.color("#FF9292",UiStyle.dark); wrapMode: Text.Wrap; Layout.fillWidth: true }
                Text { text: "Etkinlikler bu cihazda saklanır."; color: UiStyle.color("#9FB2A8",UiStyle.dark); font.pixelSize: 10 }
            }
        }
    }
    Dialog {
        id: editor; objectName: "eventEditor"; title: root.editingId.length ? "Etkinliği düzenle" : "Yeni etkinlik"
        anchors.centerIn: parent; width: 410; modal: true
        ColumnLayout {
            width: parent.width; spacing: 12
            TextField { id: eventTitle; objectName: "eventTitle"; Layout.fillWidth: true; placeholderText: "Etkinlik adı"; maximumLength: 200; Accessible.name: "Etkinlik adı" }
            TextField { id: eventDate; Layout.fillWidth: true; placeholderText: "YYYY-AA-GG"; maximumLength: 10; Accessible.name: "Tarih" }
            TextField { id: eventTime; Layout.fillWidth: true; placeholderText: "Saat: 14:30 (tüm gün için boş bırak)"; maximumLength: 5; Accessible.name: "Etkinlik saati" }
            Text { text: root.editorError; visible: text.length > 0; color: UiStyle.color("#FF9292",UiStyle.dark); wrapMode: Text.Wrap; Layout.fillWidth: true }
            RowLayout {
                AppButton { text: "Sil"; visible: root.editingId.length > 0; onClicked: deletion.open() }
                Item { Layout.fillWidth: true }
                AppButton { text: "Vazgeç"; onClicked: editor.close() }
                AppButton { text: "Kaydet"; accent: true; onClicked: { if(CalendarData.saveEvent(root.editingId,eventDate.text,eventTime.text,eventTitle.text)) { root.selectedDate=eventDate.text; editor.close() } else root.editorError=CalendarData.error } }
            }
        }
        onOpened: eventTitle.forceActiveFocus()
    }
    Dialog { id: deletion; title: "Etkinlik silinsin mi?"; anchors.centerIn: parent; modal: true; standardButtons: Dialog.Yes | Dialog.No; onAccepted: { if(CalendarData.removeEvent(root.editingId)) editor.close(); else root.editorError=CalendarData.error } }
}
