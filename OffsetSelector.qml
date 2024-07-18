import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia
import Views

Dialog {
    id: root
    width: 800
    height: 600
    standardButtons: Dialog.Ok | Dialog.Cancel

    property int begin
    property int end
    property int indexClip
    property string video
    property string song
    property int offset: 0

    MediaPlayer {
        id: player
        source: root.video
        audioOutput: AudioOutput {
        }
        videoOutput: video
        onPositionChanged: MainController.position = player.position
    }

    MediaPlayer {
        id: audio
        source: root.song
        audioOutput: AudioOutput {
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
                icon.name: "media-playback-start"
                onClicked: player.play()
            }
            ToolButton {
                icon.name: "media-playback-stop"
                onClicked: player.stop()
            }
        }

    }
}
