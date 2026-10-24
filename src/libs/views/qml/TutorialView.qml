import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import QtMultimedia
import Views

// TUTORIAL mode: load a source video, add narrated comments on the timeline,
// generate Piper audio, produce an SRT and render the final video.
Item {
    id: root

    MediaPlayer {
        id: player
        source: TutorialController.videoFilePath
        audioOutput: AudioOutput {}
        videoOutput: videoScreen
        onPositionChanged: TutorialController.currentPositionMs = player.position
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        // Top: source video + player controls.
        RowLayout {
            Layout.fillWidth: true
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 320
                Layout.minimumHeight: 240
                color: "black"
                VideoOutput {
                    id: videoScreen
                    anchors.fill: parent
                }
            }
            ColumnLayout {
                Layout.fillHeight: true
                spacing: 6
                ToolButton {
                    icon.name: "media-playback-start"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: player.play()
                }
                ToolButton {
                    icon.name: "media-playback-pause"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: player.pause()
                }
                ToolButton {
                    icon.name: "media-playback-stop"
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: player.stop()
                }
                Label {
                    text: qsTr("Position: %1s").arg((player.position / 1000).toFixed(1))
                    Layout.alignment: Qt.AlignHCenter
                }
                Button {
                    text: qsTr("Load Video")
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: openVideo.open()
                }
                Button {
                    text: qsTr("Add Comment at Playhead")
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: CommentDialog.open()
                }
            }
        }

        // Comments table.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            RowLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 4
                Label {
                    text: qsTr("Time (ms)")
                    Layout.preferredWidth: 110
                }
                Label {
                    text: qsTr("Comment text")
                    Layout.fillWidth: true
                }
                Label {
                    text: qsTr("Audio (s)")
                    Layout.preferredWidth: 80
                }
                Label {
                    text: qsTr("Status")
                    Layout.preferredWidth: 90
                }
                Item {
                    Layout.preferredWidth: 140
                }
            }

            ListView {
                id: commentList
                anchors.top: parent.top
                anchors.topMargin: 24
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 2
                model: TutorialController.comments
                delegate: RowLayout {
                    width: commentList.width
                    required property var model
                    required property string id
                    required property string text
                    required property int startTimeMs
                    required property int audioDurationMs
                    required property int status
                    required property string error

                    property string statusText: {
                        switch(status) {
                        case 0: return qsTr("Pending")
                        case 1: return qsTr("Generating")
                        case 2: return qsTr("Ready")
                        case 3: return qsTr("Outdated")
                        case 4: return qsTr("Error")
                        default: return ""
                        }
                    }

                    TextField {
                        Layout.preferredWidth: 110
                        text: delegate.startTimeMs
                        onEditingFinished: delegate.model.startTimeMs = Number(text)
                    }
                    TextField {
                        Layout.fillWidth: true
                        text: delegate.text
                        onEditingFinished: delegate.model.text = text
                    }
                    Label {
                        Layout.preferredWidth: 80
                        text: (delegate.audioDurationMs / 1000).toFixed(1)
                        horizontalAlignment: Qt.AlignHCenter
                    }
                    Label {
                        Layout.preferredWidth: 90
                        text: delegate.statusText
                        ToolTip.text: delegate.error
                        horizontalAlignment: Qt.AlignHCenter
                    }
                    RowLayout {
                        Layout.preferredWidth: 140
                        Button {
                            text: qsTr("Play")
                            Layout.preferredWidth: 60
                            onClicked: {
                                player.position = delegate.startTimeMs
                                player.play()
                            }
                        }
                        ToolButton {
                            icon.name: "edit-delete"
                            onClicked: TutorialController.removeComment(delegate.id)
                        }
                    }
                }
            }
        }

        // Workflow toolbar.
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Button {
                text: qsTr("Generate Audio")
                enabled: !TutorialController.busy
                onClicked: TutorialController.generateAudio()
            }
            Button {
                text: qsTr("Generate Subtitles")
                enabled: !TutorialController.busy
                onClicked: TutorialController.generateSubtitle()
            }
            Switch {
                id: keepAudioSw
                text: qsTr("Keep original audio")
                checked: true
            }
            Switch {
                id: embedSubsSw
                text: qsTr("Embed subtitles")
                checked: true
            }
            Button {
                text: qsTr("Render Video")
                enabled: !TutorialController.busy
                onClicked: TutorialController.renderVideo(keepAudioSw.checked, embedSubsSw.checked)
            }
            Button {
                text: qsTr("Cancel")
                enabled: TutorialController.busy
                onClicked: TutorialController.renderer.cancel()
            }
        }

        // Status / progress bar.
        RowLayout {
            Layout.fillWidth: true
            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: Math.max(1, TutorialController.total)
                value: TutorialController.current
                visible: TutorialController.busy
            }
            Label {
                Layout.fillWidth: true
                text: TutorialController.statusMessage
                wrapMode: Text.Wrap
            }
        }
    }

    FileDialog {
        id: openVideo
        fileMode: FileDialog.OpenFile
        nameFilters: ["Video files (*.mp4 *.mkv *.mov *.avi *.webm)"]
        onAccepted: TutorialController.videoFilePath = openVideo.currentFile
    }

    Popup {
        id: commentDialog
        property string text
        anchors.centerIn: Overlay.overlay
        width: 420
        padding: 12
        ColumnLayout {
            Label {
                text: qsTr("New comment text")
            }
            TextField {
                id: commentField
                Layout.fillWidth: true
                placeholderText: qsTr("What should be said at this position?")
                onTextChanged: commentDialog.text = text
            }
            RowLayout {
                Layout.fillWidth: true
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Add")
                    onClicked: {
                        if(commentField.text.trim().length > 0)
                            TutorialController.addCommentAtCurrentPosition(commentField.text)
                        commentField.text = ""
                        commentDialog.close()
                    }
                }
                Button {
                    text: qsTr("Cancel")
                    onClicked: commentDialog.close()
                }
            }
        }
    }
}
