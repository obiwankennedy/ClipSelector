import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QtQuick.Dialogs
import Views

ApplicationWindow {
    width: 640
    height: 700
    visible: true
    title: qsTr("ClipSelector")

    onClosing: {
        MainController.writeSettings()
        TutorialController.writeSettings()
    }

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

    Dialog {
        id: runMakeClip
        anchors.centerIn: Overlay.overlay

        //height: lyt.implicitHeight + 30

        GridLayout {
            id: lyt
            columns: 2
            Label {
                text: qsTr("VideFile:")
            }
            Label {
                text: MainController.videoFilePath
            }

            Switch {
                id: keepSizeSw
                text: qsTr("Keep Size")
            }
            Switch {
                id: labelSw
                text: qsTr("Label")
            }
            Switch {
                id: mergeSw
                text: qsTr("Merge")
            }
            Switch {
                id: nomusicSw
                text: qsTr("No music")
            }
        }
         standardButtons: Dialog.Ok | Dialog.Cancel
         onAccepted: MainController.runClipMaker(keepSizeSw.checked, labelSw.checked, mergeSw.checked, nomusicSw.checked)
    }



    Popup {
        id: settingPopup
        anchors.centerIn: Overlay.overlay
        width: 460
        height: 420
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
            Action {
                text: qsTr("Tutorial")
                checked: MainController.appMode === MainController.Tutorial
                onTriggered: MainController.appMode = MainController.Tutorial
            }
        }
    }

    EditorContent {
        anchors.fill: parent
    }
}
