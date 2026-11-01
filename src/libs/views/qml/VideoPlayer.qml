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
        source: MainController.projectCtrl.videoFilePath
        audioOutput: AudioOutput {

        }
        onSourceChanged: {
            console.log(MainController.videoCtrl.videoFilePath)
        }
        onErrorStringChanged: console.log("error:",player.errorString)
        onMediaStatusChanged: console.log("status:",player.mediaStatus)

        videoOutput: video
        Component.onCompleted: MainController.effectPlayback.setPlayer(player)
        onPositionChanged: {
            MainController.tutoCtrl.currentPositionMs = player.position
            if(MainController.videoCtrl.playingMode === VideoController.ClipMode)
                MainController.videoCtrl.position = player.position
        }
    }
    Connections {
        target: MainController.videoCtrl
        function onMovePosition(p)  {
            player.position = p;
        }
        function onPlayerStatusChanged(){
            const status = MainController.videoCtrl.playerStatus
            if(status == VideoController.PlayerStatus.Playing)
                player.play()
            else if(status == VideoController.PlayerStatus.Stopped)
                player.stop()
            else if(status == VideoController.PlayerStatus.Paused)
                player.pause()
        }
    }
    contentItem: VideoOutput {
        id: video
        anchors.fill: parent
    }
}
