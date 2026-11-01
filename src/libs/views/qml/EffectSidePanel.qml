import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Core

// Right-hand panel of the tutorial editor: a table of all timed events and,
// below it, the list of generated audio files with per-file regenerate /
// remove actions.
ColumnLayout {
    id: root
    spacing: 6

    // Format a duration in milliseconds as m:ss (h:mm:ss for long videos).
    function fmtTime(ms) {
        var total = Math.floor(ms / 1000)
        var h = Math.floor(total / 3600)
        var m = Math.floor((total % 3600) / 60)
        var s = total % 60
        var sec = s < 10 ? "0" + s : "" + s
        if (h > 0) {
            var min = m < 10 ? "0" + m : "" + m
            return h + ":" + min + ":" + sec
        }
        return m + ":" + sec
    }

    // --- Table of all timed events ---
    Label {
        text: qsTr("Timed events")
        Layout.fillWidth: true
        font.bold: true
    }

    ListView {
        id: eventsList
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: MainController.tutoCtrl.effects

        header: RowLayout {
            id: eventsHeader
            width: eventsList.width
            spacing: 4
            Label {
                text: qsTr("Type")
                Layout.fillWidth: true
                Layout.minimumWidth: eventsHeader.width * 0.45
            }
            Label {
                text: qsTr("Begin")
                Layout.fillWidth: true
                Layout.minimumWidth: eventsHeader.width * 0.25
            }
            Label {
                text: qsTr("End")
                Layout.fillWidth: true
                Layout.minimumWidth: eventsHeader.width * 0.25
            }
        }

        delegate: RowLayout {
            id: eventRow
            width: eventsList.width
            required property var model
            spacing: 4
            Label {
                text: eventRow.model.typeName
                Layout.fillWidth: true
                Layout.minimumWidth: eventRow.width * 0.45
            }
            Label {
                text: root.fmtTime(eventRow.model.begin)
                Layout.fillWidth: true
                Layout.minimumWidth: eventRow.width * 0.25
            }
            Label {
                text: root.fmtTime(eventRow.model.end)
                Layout.fillWidth: true
                Layout.minimumWidth: eventRow.width * 0.25
            }
        }
    }

    // --- Generated audio files ---
    Label {
        text: qsTr("Audio files")
        Layout.fillWidth: true
        font.bold: true
    }

    ListView {
        id: audioList
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: 4
        model: MainController.tutoCtrl.effects

        // Only audio comments that have a generated file are listed.
        delegate: RowLayout {
            id: audioRow
            width: audioList.width
            visible: audioRow.model.audioFile !== ""
            required property var model
            spacing: 4

            Label {
                text: audioRow.model.audioFile.split("/").pop()
                Layout.fillWidth: true
                elide: Text.ElideRight
                ToolTip.text: audioRow.model.audioFile
            }
            Label {
                text: root.fmtTime(audioRow.model.begin)
            }
            ToolButton {
                text: "\u21bb"
                ToolTip.text: qsTr("Regenerate this audio file")
                onClicked: MainController.tutoCtrl.regenerateAudio(audioRow.model.effectCtrl.id)
            }
            ToolButton {
                text: "\u2715"
                ToolTip.text: qsTr("Remove this audio file")
                onClicked: MainController.tutoCtrl.removeAudio(audioRow.model.effectCtrl.id)
            }
        }
    }
}
