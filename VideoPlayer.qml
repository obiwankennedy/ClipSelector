import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia
import Views


Item {
    MediaPlayer {
        id: player
        source: MainController.fileName
        audioOutput: AudioOutput {

        }
        onSourceChanged: {
            console.log(MainController.fileName)
        }
        onErrorStringChanged: console.log("error:",player.errorString)
        onMediaStatusChanged: console.log("status:",player.mediaStatus)

        videoOutput: video
        onPositionChanged: {
            if(playClip.checked)
                MainController.position = player.position
        }
    }

    ColumnLayout {
        id: lyt
        anchors.fill: parent
        VideoOutput {
            id: video
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 300
        }
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
                onClicked: MainController.clipModel.rangeIntoCurrentClip(player.position,player.position+30000)
            }
            ToolButton {

                id: playClip
                icon.name: "media-playback-start"
                checkable: true
                onClicked: MainController.playScenes()
            }
            Label {
                text: qsTr("Time: %1ms").arg(player.position)
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
            implicitHeight: 100//scrollView.implicitHeight
            ScrollView {
                id: scrollView
                implicitHeight: 100//clipLine.implicitHeight*2
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOn
                ScrollBar.horizontal.interactive: true
                Rectangle {
                    id: clipLine
                    property real secondInPixel: clipLine.implicitWidth/(player.duration/1000)
                    implicitHeight: 20
                    implicitWidth: scrollView.width*2
                    color: "green"
                    radius: 10


                    Repeater {
                        id: rep
                        model: MainController.clipModel
                        Repeater {
                            required property QtObject ranges
                            required property color clipColor
                            model: ranges
                            delegate: Rectangle {
                                id: range
                                required property real begin
                                required property real end

                                color: clipColor

                                height: 20
                                x: (begin/1000)*clipLine.secondInPixel
                                width: ((end-begin)/1000)*clipLine.secondInPixel
                            }
                        }
                    }

                    Rectangle {
                        color: "white"
                        height: parent.height
                        width: 2
                        x: (player.position/1000)*clipLine.secondInPixel
                    }
                }
            }
            SubClipTable {
                id: tableView
                video: MainController.fileName
                implicitHeight: 100
            }

        }


        TabBar {
            id: bar
            Layout.fillWidth: true
            TabButton {
                text: qsTr("TimeLine")
            }
            TabButton {
                text: qsTr("Tableview")
            }
        }
    }




}
