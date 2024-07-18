import QtQuick
import QtQuick.Controls
import QtMultimedia
import QtQuick.Dialogs
import QtCore
import Views

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("ClipSelector")

    FileDialog {
        id: open
        currentFolder: StandardPaths.standardLocations(StandardPaths.MoviesLocation)[0]
        onAccepted: MainController.fileName = open.currentFile
    }

    FileDialog {
        id: save
        currentFolder: open.currentFolder
        fileMode: FileDialog.SaveFile
        onAccepted: MainController.saveModel(save.currentFile)
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("File")
            Action {
                text: qsTr("Open")
                onTriggered: open.open() //MainController.fileName = "file:///media/renaud/other/document/dcs/2024-05-24_03-36-01.mp4"
                //
            }
            Action {
                text: qsTr("Save")
                onTriggered: save.open()
            }
            MenuSeparator{

            }
            Action {
                text: qsTr("Quit")
                onTriggered: Qt.exit(0)
            }
        }

    }

    Item {
        anchors.fill: parent

        VideoPlayer {
            anchors.fill: parent
        }

    }
}
