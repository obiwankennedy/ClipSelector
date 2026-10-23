import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia
import Views


SplitView {
    orientation: Qt.Vertical
    palette {
        buttonText: "black"
    }
    Dialog {
        id: runMakeClip
        anchors.centerIn: Overlay.overlay

        //height: lyt.implicitHeight + 30

        GridLayout {
            id: lyt
            columns: 2
            Label {
                text: qsTr("VideFile:")
            }
            Label {
                text: MainController.videoFilePath
            }

            Switch {
                id: keepSizeSw
                text: qsTr("Keep Size")
            }
            Switch {
                id: labelSw
                text: qsTr("Label")
            }
            Switch {
                id: mergeSw
                text: qsTr("Merge")
            }
            Switch {
                id: nomusicSw
                text: qsTr("No music")
            }
        }
         standardButtons: Dialog.Ok | Dialog.Cancel
         onAccepted: MainController.runClipMaker(keepSizeSw.checked, labelSw.checked, mergeSw.checked, nomusicSw.checked)
    }
    MediaPlayer {
        id: player
        source: MainController.videoFilePath
        audioOutput: AudioOutput {

        }
        onSourceChanged: {
            console.log(MainController.videoFilePath)
        }
        onErrorStringChanged: console.log("error:",player.errorString)
        onMediaStatusChanged: console.log("status:",player.mediaStatus)

        videoOutput: video
        onPositionChanged: {
            if(playClip.checked)
                MainController.position = player.position
        }
    }
    Connections {
        target: MainController
        function onMovePosition(p)  {
            player.position = p;
        }
        function onPlay(){
            player.play()
        }
    }
    VideoOutput {
        id: video
        SplitView.fillHeight: true
        SplitView.fillWidth: true
        SplitView.minimumHeight: 300
    }
    ColumnLayout {
        SplitView.fillWidth: true
        SplitView.minimumHeight: 300
        RowLayout {
            id: controlPanel
            ToolButton {
                icon.name: "media-playback-pause"
                onClicked: player.pause()
            }
            ToolButton {
                icon.name: "media-playback-start"
                onClicked: player.play()
            }
            ToolButton {
                icon.name: "media-playback-stop"
                onClicked: player.stop()
            }
            ToolButton {
                icon.name: "list-add"
                onClicked: MainController.clipModel.insertClip(player.position,player.position+30000)
            }
            ToolButton {
                icon.name: "list-add"
                icon.color: "gray"
                onClicked: MainController.clipModel.rangeIntoCurrentClip(player.position,player.position+30000)
            }

            /*ToolButton {
            icon.name: "media-playback"
            icon.color: "gray"
            onClicked: MainController.clipModel.rangeIntoCurrentClip(player.position,player.position+30000)
        }*/
            ToolButton {

                id: playClip
                icon.name: "media-playback-start"
                checkable: true
                onClicked: MainController.playScenes()
            }
            Label {
                property int min: player.position/1000/60
                property int sec: (player.position/1000) - (min*60)
                text: qsTr("Time: %1ms - %3min%2s").arg(player.position).arg(sec).arg(min)
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Qt.AlignRight
                text: qsTr("Render:")
            }

            ToolButton {
                icon.name: "media-playback-start"
                icon.color: "red"
                onClicked: runMakeClip.open()
            }
        }


        Slider {
            id: timeLine
            Layout.fillWidth: true
            from: 0
            to: player.duration
            value: player.position
            enabled: player.seekable
            onMoved: player.position = timeLine.value
        }

        StackLayout {
            id: layout
            currentIndex: bar.currentIndex
            Layout.fillHeight: true

            Rectangle {
                id: bottomPanel
                color: "transparent"
                Rectangle {
                    id: clipLine
                    property real secondInPixel: clipLine.width/(player.duration/1000)
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: statusBar.top
                    color: "#cccccc"
                    radius: 10
                    border.width: 1
                    border.color: "black"

                    Repeater {
                        id: rep
                        model: MainController.clipListModel
                        Rectangle {
                            required property real begin
                            required property real end
                            required property color clipColor
                            required property string label
                            color: clipColor

                            height: 20
                            x: (begin/1000)*clipLine.secondInPixel
                            width: ((end-begin)/1000)*clipLine.secondInPixel

                            TextEdit {
                                anchors.centerIn: parent
                                text: label
                                onEditingFinished: model.label = text
                            }
                        }
                    }

                    Rectangle {

                        color: "green"
                        height: parent.height
                        width: 2
                        x: (player.position/1000)*clipLine.secondInPixel
                    }
                }

                Repeater {
                    id: repeater
                    model: clipLine.width / (clipLine.secondInPixel * 10) + 1
                    delegate:  Item {
                        property int minuteCount: 10
                        x: index * clipLine.secondInPixel * minuteCount
                        height: clipLine.height + timeLbl.height
                        Rectangle {
                            id: line
                            anchors.top: parent.top
                            anchors.bottom: timeLbl.top
                            color: "red"
                            opacity: 0.9
                            width: 1
                        }
                        Text {
                            id: timeLbl
                            height: implicitHeight
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: line.horizontalCenter
                            text: "%1s".arg(index * minuteCount)
                        }
                    }
                }

                Text {
                    id: statusBar
                    anchors.bottom: parent.bottom
                    text: ""
                    color: "transparent"
                }

            }
            SubClipTable {
                id: tableView
                video: MainController.videoFilePath
                Layout.preferredHeight: 300
            }
        }
        TabBar {
            id: bar
            Layout.fillWidth: true
            TabButton {
                text: qsTr("TimeLine")
                width: 200
            }
            TabButton {
                text: qsTr("Tableview")
                width: 200
            }
        }
    }
}
