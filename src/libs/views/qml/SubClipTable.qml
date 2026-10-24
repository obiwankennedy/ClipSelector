import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.qmlmodels

ListView {
    id: _root

    property alias video: offsetSelector.video

    model: MainController.clipModel
    //resizableColumns: true
    //alternatingRows: true
    //columnSpacing: 3
    //rowSpacing: 6

    MusicSelector {
        id: musicSelector
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
    }

    OffsetSelector {
        id: offsetSelector
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
    }

    header: RowLayout {
        id: headerLyt
        width: _root.width
        property real usableWidth:  (_root.width - 40)
        Label {
            text: qsTr("Name")
            Layout.fillWidth: true
            horizontalAlignment: Qt.AlignHCenter
            Layout.leftMargin:  40
            Layout.minimumWidth:  headerLyt.usableWidth * 0.4
            Layout.maximumWidth:  headerLyt.usableWidth * 0.4
        }
        Label {
            text: qsTr("Duration")
            horizontalAlignment: Qt.AlignHCenter
            Layout.minimumWidth:  headerLyt.usableWidth * 0.1
            Layout.maximumWidth:  headerLyt.usableWidth * 0.1
        }
        Label {
            text: qsTr("music")
            horizontalAlignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.minimumWidth:  headerLyt.usableWidth * 0.4
            Layout.maximumWidth:  headerLyt.usableWidth * 0.4
        }
        Label {
            text: qsTr("OffSet")
            horizontalAlignment: Qt.AlignHCenter
            Layout.minimumWidth:  headerLyt.usableWidth * 0.1
            Layout.maximumWidth:  headerLyt.usableWidth * 0.1
        }
    }

    /*columnWidthProvider: function(column) {
        return (_root.width-(_root.columnSpacing*4)) / 5;
    }*/

    Layout.fillWidth: true
    delegate: ColumnLayout {
        id: deLyt
        width: _root.width
        required property RangeModel ranges
        required property int duration
        required property string label
        required property string music
        required property int offset
        required property int index
        required property var model
        property real usableWidth:  (_root.width - 40)
        RowLayout {
            Layout.fillWidth: true
            ToolButton {
                id: detail
                text: "▶"
                checkable: true
                Layout.minimumWidth:  40
                Layout.maximumWidth:  40
                rotation: checked ? 90 : 0
                flat: true
                background: Item {}
            }

            TextField {
                Layout.fillWidth: true
                text: deLyt.label
                Layout.minimumWidth:  deLyt.usableWidth * 0.4
                Layout.maximumWidth:  deLyt.usableWidth * 0.4
                onEditingFinished: {
                    console.log("clip name changed: ",text)
                    model.label = text
                }
            }
            Label {
                text: deLyt.duration
                Layout.preferredWidth: 100
                Layout.minimumWidth:  deLyt.usableWidth * 0.1
                Layout.maximumWidth:  deLyt.usableWidth * 0.1
            }
            Button {
                text: deLyt.music
                Layout.fillWidth: true
                Layout.minimumWidth:  deLyt.usableWidth * 0.4
                Layout.maximumWidth:  deLyt.usableWidth * 0.4
                onClicked:{
                    musicSelector.indexClip = deLyt.index
                    musicSelector.open()
                }
            }
            Button {
                text: deLyt.offset
                Layout.minimumWidth:  deLyt.usableWidth * 0.1
                Layout.maximumWidth:  deLyt.usableWidth * 0.1
                onClicked:{
                    offsetSelector.indexClip = deLyt.index
                    offsetSelector.song = model.music
                    offsetSelector.clipDuration = deLyt.duration
                    offsetSelector.open()
                }
            }
        }
        RowLayout {
            Layout.leftMargin: 100
            Layout.fillWidth: true
            visible: detail.checked
            Label {
                Layout.fillWidth: true
                text: qsTr("Begin")
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("End")
            }
            Label {
                text: qsTr("Duration")
                Layout.preferredWidth: 100
            }
        }
        ListView {
            id: listSubClip
            visible: detail.checked
            Layout.leftMargin: 100
            Layout.fillWidth: true
            Layout.preferredHeight: 20 * count
            model: deLyt.ranges
            delegate: RowLayout {
                id: subClipDelegate
                width: listSubClip.width
                required property int begin
                required property int end
                required property int duration
                required property var model
                TextField {
                    Layout.fillWidth: true
                    text: subClipDelegate.begin
                    onEditingFinished: {
                        console.log("subclip begin changed: ",text)
                        model.begin = text
                    }
                }
                TextField {
                    Layout.fillWidth: true
                    text: subClipDelegate.end
                    onEditingFinished: {
                        console.log("subclip End changed: ",text)
                        model.end = text
                    }
                }
                Label {
                    text: subClipDelegate.duration
                    Layout.preferredWidth: 100

                }
            }
        }
    }

    //editTriggers : TableView.SingleTapped
    /*delegate: TreeViewDelegate {

        Rectangle {
            color: clipColor
            Timer {
                running: parent.visible
                repeat: true
                onTriggered: console.log("dispaly",parent.color," ",clipColor)
            }

            visible: column === 3
            anchors.fill: parent
        }
    }*/

    Rectangle {
        anchors.fill: parent
        color: "green"
        opacity: 0.2
    }
}
