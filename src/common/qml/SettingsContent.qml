// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: root
    property string selectedCategory: "System"
    property string feedback: ""
    readonly property color foreground: Controller.darkTheme?"#F2F7F4":"#18261F"
    readonly property var categories: [
        {id:"System",name:qsTr("Sistem"),icon:"computer-symbolic"},{id:"Display",name:qsTr("Ekran"),icon:"video-display-symbolic"},
        {id:"Network",name:qsTr("Wi-Fi ve ağ"),icon:"network-wireless-symbolic"},{id:"Bluetooth",name:"Bluetooth",icon:"bluetooth-symbolic"},
        {id:"Sound",name:qsTr("Ses"),icon:"audio-volume-high-symbolic"},{id:"Power",name:qsTr("Güç ve oturum"),icon:"system-shutdown-symbolic"},
        {id:"Storage",name:qsTr("Depolama"),icon:"drive-harddisk-symbolic"},{id:"Keyboard & Mouse",name:qsTr("Klavye ve fare"),icon:"input-keyboard-symbolic"},
        {id:"Personalization",name:qsTr("Kişiselleştirme"),icon:"preferences-desktop-wallpaper-symbolic"},{id:"Applications",name:qsTr("Uygulamalar"),icon:"view-app-grid-symbolic"},
        {id:"Privacy",name:qsTr("Gizlilik"),icon:"changes-prevent-symbolic"},{id:"About",name:qsTr("Hakkında"),icon:"help-about-symbolic"}
    ]
    function navigate(page) { selectedCategory=page; feedback=""; if(scroll.contentItem) scroll.contentItem.contentY=0 }
    onSelectedCategoryChanged: { feedback=""; if(scroll.contentItem) scroll.contentItem.contentY=0 }
    Connections { target:Controller; function onActionFailed(message){ root.feedback=message } }
    FileDialog { id:wallpaperPicker; property bool lockScreen:false; title:lockScreen?qsTr("Kilit ekranı duvar kâğıdı"):qsTr("Duvar kâğıdı seç"); nameFilters:[qsTr("Resimler (*.png *.jpg *.jpeg *.webp *.svg)")]; onAccepted: { if(lockScreen) Controller.lockWallpaperUrl=selectedFile.toString(); else Controller.wallpaperUrl=selectedFile.toString() } }
    Rectangle {
        id:sidebar; anchors.left:parent.left; anchors.top:parent.top; anchors.bottom:parent.bottom; width:Math.min(200,root.width*0.27); color:Controller.darkTheme?"#111E1C":"#E4EEE8"
        ColumnLayout {
            anchors.fill:parent; anchors.margins:10; spacing:8
            TextField { id:search; objectName:"settingsSearch"; Layout.fillWidth:true; Layout.preferredHeight:35; placeholderText:qsTr("Ayarlarda ara…"); color:root.foreground; placeholderTextColor:Controller.darkTheme?"#9FB6B3":"#586B60"; font.pixelSize:11; background:Rectangle { radius:7; color:Controller.darkTheme?"#1B302B":"#F5FAF7"; border.width:1; border.color:Controller.darkTheme?"#3B5650":"#BDCEC4" } }
            ListView {
                Layout.fillWidth:true; Layout.fillHeight:true; spacing:3; clip:true
                model:root.categories.filter(function(page){return page.name.toLowerCase().includes(search.text.toLowerCase()) || page.id.toLowerCase().includes(search.text.toLowerCase())})
                ScrollBar.vertical:ScrollBar {}
                delegate:Rectangle {
                    required property var modelData; width:ListView.view.width; height:35; radius:7
                    color:root.selectedCategory===modelData.id?Qt.alpha(Controller.accentColor,0.3):(hover.containsMouse?(Controller.darkTheme?"#243C35":"#D5E7DB"):"transparent")
                    Image { anchors.left:parent.left; anchors.leftMargin:11; anchors.verticalCenter:parent.verticalCenter; width:18; height:18; sourceSize:Qt.size(22,22); source:"image://theme/"+modelData.icon+(Controller.darkTheme?"?theme=dark":"?theme=light") }
                    Text { anchors.fill:parent; anchors.leftMargin:39; anchors.rightMargin:6; verticalAlignment:Text.AlignVCenter; text:modelData.name; color:root.foreground; font.family:"Inter"; font.pixelSize:11; elide:Text.ElideRight }
                    MouseArea { id:hover; anchors.fill:parent; hoverEnabled:true; cursorShape:Qt.PointingHandCursor; onClicked:root.navigate(modelData.id) }
                }
            }
            SettingsControls.Caption { text:Product.name+" "+Product.version; font.pixelSize:9 }
        }
    }
    Rectangle {
        anchors.left:sidebar.right; anchors.right:parent.right; anchors.top:parent.top; anchors.bottom:parent.bottom; color:Controller.darkTheme?"#0B1818":"#EFF5F1"
        Image { anchors.fill:parent; source:Controller.wallpaperUrl.length?Controller.wallpaperUrl:Product.wallpaperUrl; fillMode:Image.PreserveAspectCrop; sourceSize:Qt.size(1200,800); opacity:Controller.darkTheme?0.24:0.09; clip:true }
        SettingsScrollView {
            id:scroll; objectName:"settingsScroll"; anchors.fill:parent; anchors.margins:12; contentHeight:body.implicitHeight
            ColumnLayout {
                id:body; width:scroll.availableWidth; height:implicitHeight; spacing:12
                SettingsControls.Title { text:{ const page=root.categories.find(function(item){return item.id===root.selectedCategory}); return page?page.name:root.selectedCategory } font.pixelSize:24; font.weight:Font.Bold }
                SettingsControls.Caption { visible:Controller.mockMode; text:qsTr("Tasarım önizlemesi · sistem kontrolleri benzetimdir") }
                SettingsControls.Caption { visible:root.feedback.length>0; text:root.feedback; color:Controller.darkTheme?"#FFB3A3":"#A43138" }
                Loader {
                    id:pageLoader; objectName:"settingsPageLoader"; Layout.fillWidth:true; Layout.preferredHeight:item?item.implicitHeight:0
                    sourceComponent:root.selectedCategory==="Display"?display:root.selectedCategory==="Personalization"?personalization:root.selectedCategory==="Network"?network:["System","Storage","About"].includes(root.selectedCategory)?system:other
                }
            }
        }
    }
    Component { id:display; DisplayPage { onChooseWallpaper:{ wallpaperPicker.lockScreen=false; wallpaperPicker.open() } } }
    Component { id:personalization; PersonalizationPage { onChooseWallpaper: function(lockScreen){ wallpaperPicker.lockScreen=lockScreen; wallpaperPicker.open() } } }
    Component { id:network; NetworkPage {} }
    Component { id:system; SystemPages { category:root.selectedCategory; onNavigate:page=>root.navigate(page) } }
    Component { id:other; OtherSettingsPage { category:root.selectedCategory } }
}
