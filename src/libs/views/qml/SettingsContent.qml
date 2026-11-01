import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import Views
import Core

Pane {
    id: root

    // FileDialog/FolderDialog report their selection as a url; normalize it
    // to a plain local path for the C++ settings.
    function localPath(u) {
        var s = u.toString()
        if (s.startsWith("file://"))
            s = s.slice(7)
        return s
    }

    FileDialog {
        id: openFile
        fileMode: FileDialog.OpenFile
    }
    FileDialog {
        id: openScript
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Python scripts (*.py)")]
    }
    FolderDialog {
        id: openModelDir
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label {
            text: qsTr("MUSIC CLIP")
            font.bold: true
        }
        GridLayout {
            columns: 3
            Label {
                text: qsTr("ClipMaker path:")
            }
            TextField {
                Layout.fillWidth: true
                text: MainController.renderCtrl.clipMakeScript
                onEditingFinished: {
                    MainController.renderCtrl.clipMakeScript = Qt.url(text)
                }
            }
            Button {
                text: qsTr("Browser")
                onClicked: openFile.open()
            }
        }

        Label {
            text: qsTr("TUTORIAL (Piper TTS)")
            font.bold: true
        }
        GridLayout {
            columns: 3
            Label {
                text: qsTr("Language:")
            }
            ComboBox {
                id: langBox
                Layout.fillWidth: true
                model: MainController.tutoCtrl.languages
                onActivated: MainController.tutoCtrl.language = model[index]
                Component.onCompleted: currentIndex = Math.max(0, model.indexOf(MainController.tutoCtrl.language))
            }
            Item {}

            Label {
                text: qsTr("Voice:")
            }
            ComboBox {
                id: voiceBox
                Layout.fillWidth: true
                model: MainController.tutoCtrl.voices
                onActivated: MainController.tutoCtrl.voice = model[index]
                onModelChanged: currentIndex = Math.max(0, model.indexOf(MainController.tutoCtrl.voice))
            }
            Item {}

            Label {
                text: qsTr("Piper script:")
            }
            TextField {
                id: scriptField
                Layout.fillWidth: true
                // The voice (language) models and the python interpreter are
                // derived from the directory containing this script.
                text: MainController.tutoCtrl.ttsEngine.scriptPath
                onEditingFinished: MainController.tutoCtrl.ttsEngine.scriptPath = text
            }
            Button {
                text: qsTr("Browser")
                onClicked: openScript.open()
            }

            Label {
                text: qsTr("Model dir (voices):")
            }
            TextField {
                id: modelDirField
                Layout.fillWidth: true
                text: MainController.tutoCtrl.ttsEngine.modelDirectory
                onEditingFinished: MainController.tutoCtrl.ttsEngine.modelDirectory = text
            }
            Button {
                text: qsTr("Browser")
                onClicked: openModelDir.open()
            }

            Label {
                text: qsTr("Python:")
            }
            TextField {
                id: pythonField
                Layout.fillWidth: true
                text: MainController.tutoCtrl.ttsEngine.pythonPath
                readOnly: true
            }
            Item {}
        }
    }

    Connections {
        target: openFile
        function onAccepted() {
            MainController.renderCtrl.clipMakeScript= openFile.selectedFile
        }
    }

    Connections {
        target: openScript
        function onAccepted() {
            MainController.tutoCtrl.ttsEngine.scriptPath= localPath(openScript.selectedFile)
        }
    }

    Connections {
        target: openModelDir
        function onAccepted() {
            MainController.tutoCtrl.ttsEngine.modelDirectory= localPath(openModelDir.selectedFolder)
        }
    }

    background: Rectangle {
        border.width: 1
        border.color: "#616161"
        radius: 8
    }
}
