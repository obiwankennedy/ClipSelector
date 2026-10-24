import QtQuick
import QtQuick.Controls
import QtMultimedia
import Views
import Core

Pane {
    id: root

    property MediaPlayer mediaPlayer: player

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
    contentItem: VideoOutput {
        id: video
        anchors.fill: parent
    }
}
