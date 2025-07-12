import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia
import Views

Dialog {
    id: root
    width: 1200
    height: 600
    standardButtons: Dialog.Ok | Dialog.Cancel

    property RangeModel model
    property int indexClip
    property string video
    property string song
    readonly property int clipDuration: model.duration * 1000
    onClipDurationChanged: console.log("clipDuration:",clipDuration)
    property int offset: audio.duration * offsetMask.x / spectre.width

    onVisibleChanged: {
        if(!visible)
        {
            player.stop()
            audio.stop()
        }
    }

    onAccepted: {
        MainController.setMusicOffset(root.indexClip, root.offset)
    }

    Timer {
        id: startAudio
        repeat: false
        onTriggered: audio.play()
    }

    Keys.onRightPressed: (event) => {
        player.position += 10
    }
    Keys.onLeftPressed: (event) => {
        player.position -= 10
    }

    MediaPlayer {
        id: player
        source: root.video
        audioOutput: AudioOutput {
        }
        videoOutput: video
        onPositionChanged: {
            console.log("position on change:",player.position)
            const newPos = model.closiestPositionInClip(player.position)

            if( newPos !== player.position)
                player.position = newPos
        }
    }

    MediaPlayer {
        id: audio
        source: "file://"+root.song
        audioOutput: AudioOutput {
        }
    }
    ColumnLayout {
        id: lyt
        anchors.fill: parent
        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumHeight: 300
            color: "black"
            VideoOutput {
                id: video
                anchors.fill: parent
            }
        }
        RowLayout {
            id: controlPanel
            ToolButton {
                icon.name: "media-playback-start"
                onClicked: {
                    player.play()
                    console.log("position:",root.position)
                    player.position = model.closiestPositionInClip(0)
                    console.log("position:",player.position)
                    if(root.offset<0)
                    {
                        startAudio.interval = Math.abs(root.offset)
                        startAudio.start()
                    }
                    else
                    {
                        audio.play()
                        audio.position = root.offset
                    }
                }
            }
            ToolButton {
                icon.name: "media-playback-stop"
                onClicked: {
                    audio.stop()
                    player.stop()
                }
            }
            Label {
                text: qsTr("Offset: %1s").arg(root.offset/1000)
            }

        }
        AudioSpect {
            id: spectre
            implicitHeight: 200
            Layout.fillWidth: true
            audioFile: "file://"+root.song
            Rectangle {
                id: offsetMask
                opacity: 0.4
                color: "red"
                height: parent.height
                width: root.clipDuration*parent.width/audio.duration
                x: 0
                y: 0
                onXChanged:  {
                    console.log("H:",offsetMask.height)
                }

                DragHandler {
                    yAxis.enabled: false
                    xAxis.maximum: spectre.width-10
                    xAxis.minimum: -(parent.width-10)
                }
            }
            Rectangle {
                x: parent.width*audio.position/audio.duration
                y: 0
                height: parent.height
                width: 1
                color: "black"
            }
        }
    }
}
