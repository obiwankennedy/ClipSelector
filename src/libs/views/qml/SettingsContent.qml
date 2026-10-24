import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import Views

Item {
    id: root
    FileDialog {
        id: openFile
        fileMode: FileDialog.OpenFile
    }
    FileDialog {
        id: openFolder
        fileMode: FileDialog.OpenDirectory
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
                text: MainController.clipMakeScript
                onEditingFinished: {
                    MainController.clipMakeScript = Qt.url(text)
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
                model: TutorialController.languages
                textRole: "name"
                onActivated: TutorialController.language = model[index]
                Component.onCompleted: currentIndex = Math.max(0, model.indexOf(TutorialController.language))
            }
            Item {}

            Label {
                text: qsTr("Voice:")
            }
            ComboBox {
                id: voiceBox
                model: TutorialController.voices
                textRole: "name"
                onActivated: TutorialController.voice = model[index]
                onModelChanged: currentIndex = Math.max(0, model.indexOf(TutorialController.voice))
            }
            Item {}

            Label {
                text: qsTr("Model dir:")
            }
            TextField {
                text: TutorialController.ttsEngine.modelDirectory
                onEditingFinished: TutorialController.ttsEngine.modelDirectory = text
            }
            Button {
                text: qsTr("Browser")
                onClicked: openFolder.open()
            }
        }

        Label {
            text: qsTr("Piper script:")
        }
        TextField {
            text: TutorialController.ttsEngine.scriptPath
            onEditingFinished: TutorialController.ttsEngine.scriptPath = text
        }
        Label {
            text: qsTr("Python:")
        }
        TextField {
            text: TutorialController.ttsEngine.pythonPath
            onEditingFinished: TutorialController.ttsEngine.pythonPath = text
        }
    }

    Connections {
        target: openFolder
        function onAccepted() {
            TutorialController.ttsEngine.modelDirectory = openFolder.currentFile
        }
    }
}
