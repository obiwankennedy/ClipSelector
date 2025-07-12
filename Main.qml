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

    Timer {
        id: timer
        repeat: true
        interval: 1000*60*10//10mins
        onTriggered: MainController.saveModel(save.currentFile)
    }


    FileDialog {
        id: save
        currentFolder: open.currentFolder
        fileMode: FileDialog.SaveFile
        onAccepted: {
            MainController.saveModel(save.currentFile)
            timer.start()
        }
    }

    Component.onCompleted: {
        MainController.fileName = "file:///media/renaud/share/2024-01-27_15-17-07.mkv"
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("File")
            Action {
                text: qsTr("Open")
                onTriggered:open.open() // MainController.fileName = "file:///media/renaud/other/document/dcs/2024-06-18_17-41-45.mkv"
                // //
            }
            Action {
                text: qsTr("Save")
                onTriggered: {
                    if(MainController.fileName)
                        MainController.saveModel(MainController.fileName)
                    else
                        save.open()
                }
            }
            Action {
                text: qsTr("Save As")
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
