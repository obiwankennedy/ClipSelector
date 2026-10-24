import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

SplitView {
    orientation: Qt.Vertical
    VideoPlayer {
        id: videoPlayer
        SplitView.fillWidth: true
        SplitView.fillHeight: true
    }
    EditToolBar {
        player: videoPlayer.mediaPlayer
        SplitView.fillWidth: true
    }
    TimeLine {
        player: videoPlayer.mediaPlayer
        SplitView.fillWidth: true
    }
}
