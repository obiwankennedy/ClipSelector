import QtQuick
import QtQuick.Controls
import QtMultimedia
import QtQuick.Dialogs
import QtCore
import Views

ApplicationWindow {
    width: 640
    height: 700
    visible: true
    title: qsTr("ClipSelector")

    onClosing: MainController.writeSettings()

    FileDialog {
        id: open
        currentFolder: StandardPaths.standardLocations(StandardPaths.MoviesLocation)[0]
        onAccepted: {
            if(open.currentFile.toString().endsWith("json"))
            {
                MainController.projectFilePath = open.currentFile
                MainController.openJson(MainController.projectFilePath)
            }
            else
                MainController.videoFilePath = open.currentFile
        }
    }

    Timer {
        id: timer
        repeat: true
        interval: 1000*60*10//10mins
        onTriggered: {
            if(MainController.projectFilePath && !MainController.busy)
                MainController.saveModel(save.projectFilePath)
        }
    }


    FileDialog {
        id: save
        currentFolder: open.currentFolder
        fileMode: FileDialog.SaveFile
        nameFilters: ["Project file (*.json)"]
        onAccepted: {
            MainController.projectFilePath = save.currentFile
            MainController.saveModel(MainController.projectFilePath)
            timer.start()
        }
    }



    Popup {
        id: settingPopup
        anchors.centerIn: Overlay.overlay
        width: 200
        height: 200
        SettingsContent {
            anchors.fill: parent
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
                    if(MainController.projectFilePath)
                        MainController.saveModel(MainController.projectFilePath)
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

        Menu {
            title: qsTr("Edition")
            Action {
                text: qsTr("Settings")
                onTriggered: settingPopup.open()
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
