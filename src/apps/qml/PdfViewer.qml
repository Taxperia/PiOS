// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Pdf
Item {
    id: root
    objectName: "pdfPage"
    readonly property color foreground: UiStyle.color("#F2F7F4",UiStyle.dark)
    property bool sidebar: true
    property bool find: false
    property bool fitWidth: true
    function fit() { if(pdfDocument.status === PdfDocument.Ready) view.scaleToWidth(view.width-32,view.height-24) }
    function zoom(factor) { fitWidth=false; view.renderScale=Math.min(8,Math.max(0.1,view.renderScale*factor)) }
    PdfDocument {
        id: pdfDocument; objectName: "pdfDocument"; source: Documents.source
        onPasswordRequired: { password.clear(); passwordDialog.open() }
        onStatusChanged: if(pdfDocument.status === PdfDocument.Ready) { root.fitWidth=true; Qt.callLater(root.fit) }
    }
    Connections { target: Documents; function onSourceChanged() { pdfDocument.password="" } }
    FileDialog { id: picker; title: "PDF aç"; nameFilters: ["PDF belgeleri (*.pdf)"]; onAccepted: Documents.open(selectedFile) }
    ColumnLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 62; color: UiStyle.color("#111C19",UiStyle.dark)
            RowLayout {
                anchors.fill: parent; anchors.margins: 12; spacing: 8
                AppButton { iconName: "document-open"; text: "Aç"; onClicked: picker.open() }
                AppButton { text: "☰"; tip: "Sayfa listesini göster / gizle"; onClicked: { root.sidebar=!root.sidebar; Qt.callLater(root.fit) } }
                Text { text: Documents.name.length ? Documents.name : "PDF Görüntüleyici"; color: root.foreground; Layout.fillWidth: true; elide: Text.ElideMiddle; font.pixelSize: 13 }
                AppButton { text: "−"; tip: "Uzaklaştır"; enabled: pdfDocument.pageCount>0; onClicked: root.zoom(0.8) }
                AppButton { text: Math.round(view.renderScale*100)+"%"; tip: "Gerçek boyut"; onClicked: { root.fitWidth=false; view.resetScale() } }
                AppButton { text: "+"; tip: "Yakınlaştır"; enabled: pdfDocument.pageCount>0; onClicked: root.zoom(1.25) }
                AppButton { text: "Sığdır"; enabled: pdfDocument.pageCount>0; onClicked: { root.fitWidth=true; root.fit() } }
                AppButton { iconName: "edit-find"; tip: "Belgede ara (Ctrl+F)"; onClicked: { root.find=!root.find; if(root.find) search.forceActiveFocus() } }
            }
        }
        RowLayout {
            visible: root.find; Layout.fillWidth: true; Layout.margins: 10
            TextField { id: search; objectName: "pdfSearch"; Layout.fillWidth: true; placeholderText: "Belgede ara…"; selectByMouse: true; onTextChanged: searchDelay.restart(); onAccepted: view.searchForward() }
            Timer { id: searchDelay; interval: 200; onTriggered: view.searchString=search.text }
            AppButton { text: "↑"; tip: "Önceki eşleşme"; onClicked: view.searchBack() }
            AppButton { text: "↓"; tip: "Sonraki eşleşme"; onClicked: view.searchForward() }
            AppButton { text: "Kapat"; onClicked: { root.find=false; search.clear(); view.searchString="" } }
        }
        Text { visible: Documents.error.length > 0; text: Documents.error; Layout.fillWidth: true; Layout.margins: 10; color: UiStyle.color("#FF9292",UiStyle.dark); wrapMode: Text.Wrap }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
            Rectangle {
                visible: root.sidebar && pdfDocument.pageCount>0; Layout.preferredWidth: 155; Layout.fillHeight: true; color: UiStyle.color("#101A17",UiStyle.dark)
                ListView {
                    id: pages; anchors.fill: parent; anchors.margins: 10; clip: true; spacing: 8; model: pdfDocument.pageCount; currentIndex: view.currentPage
                    onCurrentIndexChanged: positionViewAtIndex(currentIndex,ListView.Contain)
                    delegate: ItemDelegate {
                        id: thumbnail; required property int index; width: ListView.view.width; height: 176
                        background: Rectangle { radius: 8; color: thumbnail.index === view.currentPage ? UiStyle.color("#315F46",UiStyle.dark) : thumbnail.hovered ? UiStyle.color("#304039",UiStyle.dark) : "transparent"; border.width: thumbnail.index === view.currentPage ? 1 : 0; border.color: UiStyle.accent }
                        contentItem: Column {
                            spacing: 5
                            Rectangle {
                                width: parent.width; height: 135; color: "white"; radius: 2
                                Image { anchors.fill: parent; anchors.margins: 2; source: Documents.source; currentFrame: thumbnail.index; sourceSize.width: 120; asynchronous: true; fillMode: Image.PreserveAspectFit; cache: false }
                            }
                            Text { width: parent.width; text: "Sayfa "+(thumbnail.index+1); color: root.foreground; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 11 }
                        }
                        onClicked: view.goToPage(index)
                    }
                    ScrollBar.vertical: ScrollBar {}
                }
            }
            Item {
                Layout.fillWidth: true; Layout.fillHeight: true
                SafePdfMultiPageView { id: view; objectName: "pdfView"; anchors.fill: parent; document: pdfDocument; visible: pdfDocument.status === PdfDocument.Ready; onExternalLinkRequested: url => Documents.openLink(url); onWidthChanged: if(root.fitWidth) Qt.callLater(root.fit) }
                BusyIndicator { anchors.centerIn: parent; running: pdfDocument.status === PdfDocument.Loading }
                Column {
                    anchors.centerIn: parent; width: Math.min(460,parent.width-50); spacing: 18; visible: pdfDocument.status === PdfDocument.Null || pdfDocument.status === PdfDocument.Error
                    Image { anchors.horizontalCenter: parent.horizontalCenter; width: 70; height: 70; source: "image://theme/application-pdf" }
                    Text { width: parent.width; text: pdfDocument.status === PdfDocument.Error ? pdfDocument.error : "Belgelerine daha yakından bak"; color: root.foreground; font.pixelSize: 24; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap }
                    Text { width: parent.width; text: "PDF aç, metin ara ve sayfalar arasında kolayca gezin."; color: UiStyle.color("#9FB2A8",UiStyle.dark); font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap }
                    AppButton { anchors.horizontalCenter: parent.horizontalCenter; text: "PDF dosyası aç"; accent: true; onClicked: picker.open() }
                }
                DropArea { anchors.fill: parent; onDropped: function(drop) { if(drop.hasUrls && drop.urls.length && Documents.open(drop.urls[0])) drop.acceptProposedAction() } }
            }
        }
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 48; color: UiStyle.color("#111C19",UiStyle.dark)
            RowLayout {
                anchors.fill: parent; anchors.margins: 7; spacing: 8
                AppButton { text: "‹"; tip: "Önceki sayfa"; enabled: view.currentPage>0; onClicked: view.goToPage(view.currentPage-1) }
                SpinBox { id: pageNumber; objectName: "pdfPageNumber"; from: 1; to: Math.max(1,pdfDocument.pageCount); value: Math.max(1,view.currentPage+1); enabled: pdfDocument.pageCount>0; editable: true; onValueModified: view.goToPage(value-1); Accessible.name: "Sayfa numarası" }
                Text { text: "/ "+pdfDocument.pageCount; color: root.foreground; font.pixelSize: 12 }
                AppButton { text: "›"; tip: "Sonraki sayfa"; enabled: view.currentPage<pdfDocument.pageCount-1; onClicked: view.goToPage(view.currentPage+1) }
                Item { Layout.fillWidth: true }
                AppButton { text: "Seçileni kopyala"; enabled: view.selectedText.length>0; onClicked: view.copySelectionToClipboard() }
                AppButton { text: "↻"; tip: "Sayfaları döndür"; enabled: pdfDocument.pageCount>0; onClicked: { view.pageRotation=(view.pageRotation+90)%360; if(root.fitWidth) Qt.callLater(root.fit) } }
            }
        }
    }
    Dialog {
        id: passwordDialog; title: "Bu PDF parola istiyor"; anchors.centerIn: parent; modal: true; width: 360; standardButtons: Dialog.Ok | Dialog.Cancel
        TextField { id: password; width: parent.width; echoMode: TextInput.Password; placeholderText: "Belgenin parolası"; onAccepted: passwordDialog.accept() }
        onAccepted: { pdfDocument.password=password.text; password.clear() }
        onRejected: password.clear()
        onOpened: password.forceActiveFocus()
    }
    Shortcut { sequence: "Ctrl+O"; onActivated: picker.open() }
    Shortcut { sequence: "Ctrl+F"; onActivated: { root.find=true; search.forceActiveFocus() } }
    Shortcut { sequence: "Ctrl+C"; enabled: !search.activeFocus; onActivated: view.copySelectionToClipboard() }
    Shortcut { sequence: "Ctrl++"; onActivated: root.zoom(1.25) }
    Shortcut { sequence: "Ctrl+-"; onActivated: root.zoom(0.8) }
}
