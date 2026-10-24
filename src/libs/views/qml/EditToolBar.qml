import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia

RowLayout {
    id: toolbar
    required property MediaPlayer player


    ToolButton {
        icon.name: "media-playback-pause"
        onClicked: toolbar.player.pause()
    }
    ToolButton {
        icon.name: "media-playback-start"
        onClicked: toolbar.player.play()
    }
    ToolButton {
        icon.name: "media-playback-stop"
        onClicked: toolbar.player.stop()
    }
    ToolButton {
        icon.name: "list-add"
        onClicked: MainController.clipModel.insertClip(toolbar.player.position,toolbar.player.position+30000)
    }
    ToolButton {
        icon.name: "list-add"
        icon.color: "gray"
        onClicked: MainController.clipModel.rangeIntoCurrentClip(toolbar.player.position,toolbar.player.position+30000)
    }

    ToolButton {

        id: playClip
        icon.name: "media-playback-start"
        checkable: true
        onClicked: MainController.playScenes()
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
        icon.name: "media-playback-start"
        icon.color: "red"
        onClicked: runMakeClip.open()
    }
}
