import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Views

SplitView {
    orientation: Qt.Horizontal
    SplitView {
        orientation: Qt.Vertical
        SplitView.fillWidth: true
        SplitView.fillHeight: true
        VideoPlayer {
            id: videoPlayer
            SplitView.fillWidth: true
            SplitView.fillHeight: true
        }
        EditToolBar {
            player: videoPlayer.mediaPlayer
            SplitView.fillWidth: true
        }
        TimeLinePanel {
            player: videoPlayer.mediaPlayer
            SplitView.fillWidth: true
        }
    }
    // Right panel: timed events table + generated audio files.
    EffectSidePanel {
        SplitView.fillHeight: true
        SplitView.preferredWidth: 340
        SplitView.minimumWidth: 240
    }
}
