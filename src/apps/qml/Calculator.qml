// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: root
    objectName: "calculatorPage"
    readonly property color foreground: UiStyle.color("#F2F7F4",UiStyle.dark)
    property bool solved: false
    function enter(key) {
        Calc.clearError()
        if(key === "=") { const value=Calc.evaluate(expression.text); if(value.length) { expression.text=value; solved=true } return }
        if(key === "C") { expression.clear(); solved=false; return }
        if(key === "⌫") { expression.text=expression.text.slice(0,-1); solved=false; return }
        if(key === "±") { expression.text="-("+(expression.text.length?expression.text:"0")+")"; solved=false; return }
        if(key === "√") { expression.text="sqrt("+(expression.text.length?expression.text:"0")+")"; solved=false; return }
        if(solved && /[0-9.(]/.test(key)) expression.clear()
        solved=false; expression.text+=key
    }
    Component.onCompleted: expression.forceActiveFocus()
    RowLayout {
        anchors.fill: parent; anchors.margins: 22; spacing: 20
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
            Text { text: "Hesap Makinesi"; color: root.foreground; font.pixelSize: 27; font.weight: Font.DemiBold }
            Text { text: "Günlük işlemler, parantezler ve yüzde hesabı"; color: UiStyle.color("#9FB2A8",UiStyle.dark); font.pixelSize: 12 }
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 122; radius: 12; color: UiStyle.color("#17231F",UiStyle.dark); border.color: UiStyle.color("#465750",UiStyle.dark)
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 16
                    Text { text: Calc.error.length ? Calc.error : "STANDART"; color: Calc.error.length ? UiStyle.color("#FF9292",UiStyle.dark) : UiStyle.color("#9FB2A8",UiStyle.dark); font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.Wrap }
                    TextField { id: expression; objectName: "calculatorExpression"; Layout.fillWidth: true; maximumLength: 512; placeholderText: "0"; horizontalAlignment: Text.AlignRight; font.pixelSize: 30; selectByMouse: true; background: Item {} onAccepted: root.enter("="); Accessible.name: "İşlem"; onTextEdited: root.solved=false }
                }
            }
            RowLayout {
                spacing: 6
                Repeater {
                    model: ["MC","MR","MS","M+"]
                    delegate: AppButton { required property string modelData; text: modelData; Layout.fillWidth: true; onClicked: modelData === "MC" ? Calc.clearMemory() : modelData === "MR" ? expression.text=Calc.memory : Calc.setMemory(expression.text,modelData === "M+") }
                }
                Text { text: "M: " + Calc.memory; color: UiStyle.color("#9FB2A8",UiStyle.dark); font.pixelSize: 10; elide: Text.ElideRight; Layout.maximumWidth: 90 }
            }
            GridLayout {
                columns: 4; columnSpacing: 7; rowSpacing: 7; Layout.fillWidth: true; Layout.fillHeight: true
                Repeater {
                    model: ["C","(",")","⌫","√","^","%","÷","7","8","9","×","4","5","6","−","1","2","3","+","±","0",".","="]
                    delegate: AppButton { required property string modelData; objectName: "calcKey"+modelData; text: modelData; font.pixelSize: 21; accent: modelData === "="; Layout.fillWidth: true; Layout.fillHeight: true; onClicked: root.enter(modelData) }
                }
            }
            Text { text: "Yüzde: 50% = 0,5  ·  Kopyala: Ctrl+C"; color: UiStyle.color("#9FB2A8",UiStyle.dark); font.pixelSize: 10 }
        }
        Rectangle {
            Layout.preferredWidth: 200; Layout.fillHeight: true; visible: root.width >= 650; radius: 12; color: UiStyle.color("#111C19",UiStyle.dark)
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14; spacing: 12
                Text { text: "Geçmiş"; color: root.foreground; font.pixelSize: 17; font.weight: Font.DemiBold }
                Text { visible: Calc.history.length === 0; Layout.fillWidth: true; text: "İşlemlerin burada görünecek."; color: UiStyle.color("#9FB2A8",UiStyle.dark); font.pixelSize: 12; wrapMode: Text.Wrap }
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: Calc.history; spacing: 6
                    delegate: ItemDelegate {
                        id: entry; required property var modelData; width: ListView.view.width; height: 66
                        background: Rectangle { radius: 7; color: entry.hovered ? UiStyle.color("#304039",UiStyle.dark) : "transparent" }
                        contentItem: Column { spacing: 6; Text { width: parent.width; text: entry.modelData.expression+" ="; color: UiStyle.color("#9FB2A8",UiStyle.dark); font.pixelSize: 11; elide: Text.ElideLeft } Text { width: parent.width; text: entry.modelData.result; color: UiStyle.color("#63DE8A",UiStyle.dark); font.pixelSize: 19; elide: Text.ElideLeft } }
                        onClicked: { expression.text=modelData.result; root.solved=true }
                    }
                    ScrollBar.vertical: ScrollBar {}
                }
                AppButton { text: "Geçmişi temizle"; Layout.fillWidth: true; onClicked: Calc.clearHistory() }
            }
        }
    }
    Shortcut { sequence: "Escape"; onActivated: root.enter("C") }
    Shortcut { sequence: "Ctrl+C"; enabled: !expression.activeFocus; onActivated: Calc.copy(expression.text) }
    Shortcut { sequence: "Return"; enabled: !expression.activeFocus; onActivated: root.enter("=") }
}
