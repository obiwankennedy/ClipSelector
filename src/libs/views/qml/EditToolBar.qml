import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia
import Core

RowLayout {
    id: toolbar
    required property MediaPlayer player

    property TutorialController tutoCtrl: MainController.tutoCtrl

    property int iconSize: 32
    // Tutorial mode shows the narration / editing tools, music clip mode
    // keeps the clip tools.
    property bool tutorialMode: MainController.projectCtrl.projectType === ProjectController.Tutorial

    ToolButton {
        icon.source: "qrc:/qt/qml/Views/assets/icons/pause.svg"
        icon.width: 32
        icon.height: 32
        onClicked: MainController.videoCtrl.pause()
    }
    ToolButton {
        icon.source: "qrc:/qt/qml/Views/assets/icons/play.svg"
        icon.width: 32
        icon.height: 32
        onClicked: {
            MainController.videoCtrl.play(VideoController.PlayingMode.NormalMode)
        }
    }
    ToolButton {
        icon.source: "qrc:/qt/qml/Views/assets/icons/stop.svg"
        icon.width: 32
        icon.height: 32
        onClicked: MainController.videoCtrl.stop()
    }
    ToolSeparator {

    }

    // --- Music clip mode tools ---
    ToolButton {
        visible: !toolbar.tutorialMode
        icon.source: "qrc:/qt/qml/Views/assets/icons/add.svg"
        icon.width: 32
        icon.height: 32
        icon.color: "green"
        ToolTip.text: qsTr("Add new clip")
        onClicked: MainController.videoCtrl.clipModel.insertClip(toolbar.player.position,toolbar.player.position+30000)
    }
    ToolButton {
        visible: !toolbar.tutorialMode
        icon.source: "qrc:/qt/qml/Views/assets/icons/add.svg"
        icon.width: 32
        icon.height: 32
        icon.color: "blue"
        ToolTip.text: qsTr("Extend current clip")
        onClicked: MainController.videoCtrl.clipModel.rangeIntoCurrentClip(toolbar.player.position,toolbar.player.position+30000)
    }
    ToolSeparator {
        visible: !toolbar.tutorialMode
    }
    ToolButton {
        id: playClip
        visible: !toolbar.tutorialMode
        icon.source: "qrc:/qt/qml/Views/assets/icons/play.svg"
        icon.width: 32
        icon.height: 32
        checkable: true
        onClicked: MainController.videoCtrl.play(VideoController.PlayingMode.ClipMode)
    }

    // --- Tutorial mode tools ---
    ToolButton {
        id: addComment
        visible: toolbar.tutorialMode
        icon.source: "qrc:/qt/qml/Views/assets/icons/add.svg"
        icon.width: 32
        icon.height: 32
        icon.color: "green"
        ToolTip.text: qsTr("Add a text comment at the current position")
        onClicked: MainController.tutoCtrl.addCommentAtCurrentPosition(toolbar.tutoCtrl.currentPositionMs, toolbar.tutoCtrl.currentPositionMs+30000)
    }
    ToolButton {
        visible: toolbar.tutorialMode
        icon.source: "qrc:/qt/qml/Views/assets/icons/fast-forward.svg"
        icon.width: 32
        icon.height: 32
        ToolTip.text: qsTr("Speed up (2x, 5s at the current position)")
        onClicked: MainController.tutoCtrl.addSpeedEffect(toolbar.tutoCtrl.currentPositionMs, toolbar.tutoCtrl.currentPositionMs+30000)
    }
    ToolButton {
        visible: toolbar.tutorialMode
        icon.source: "qrc:/qt/qml/Views/assets/icons/cutter.svg"
        icon.width: 32
        icon.height: 32
        ToolTip.text: qsTr("Cut (remove 3s at the current position)")
        onClicked: MainController.tutoCtrl.addCutEffect(toolbar.tutoCtrl.currentPositionMs, toolbar.tutoCtrl.currentPositionMs+30000)
    }

    Label {
        property int min: toolbar.player.position/1000/60
        property int sec: (toolbar.player.position/1000) - (min*60)
        text: qsTr("Time: %1ms - %3min%2s").arg(toolbar.player.position).arg(sec).arg(min)
    }
    Label {
        Layout.fillWidth: true
        horizontalAlignment: Qt.AlignRight
        text: qsTr("Render:")
    }

    ToolButton {
        icon.source: "qrc:/qt/qml/Views/assets/icons/render.svg"
        icon.width: 32
        icon.height: 32
        icon.color: "transparent"
        onClicked: runMakeClip.open()
    }
}
