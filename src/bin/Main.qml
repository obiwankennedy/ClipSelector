import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QtQuick.Dialogs
import QtCore
import Views
import Core


ApplicationWindow {
    id: root
    width: 1020
    height: 860
    visible: true
    title: MainController.projectCtrl.videoFilePath ? qsTr("%1 - ClipSelector").arg(MainController.projectCtrl.videoFilePath) : qsTr("ClipSelector")

    onClosing: {
        if(MainController.projectCtrl.projectFilePath)
            MainController.saveModel(MainController.projectCtrl.projectFilePath)
        MainController.writeSettings()
    }

    function openFile(path) {
        console.log("Open file:",path)
        if(path.endsWith("json"))
        {
            MainController.openProject(path)
        }
        else
        {
            MainController.projectCtrl.openVideo(path)
        }
    }

    // FileDialog reports its selection as a url; normalize it to a plain
    // local path.
    function localPath(u) {
        var s = u.toString()
        if (s.startsWith("file://"))
            s = s.slice(7)
        return s
    }

    FileDialog {
        id: open
        currentFolder: StandardPaths.standardLocations(StandardPaths.MoviesLocation)[0]
        onAccepted: openFile(localPath(open.selectedFile))
    }

    Timer {
        id: timer
        repeat: true
        interval: 1000*60*10//10mins
        onTriggered: {
            if(MainController.projectCtrl.projectFilePath && !MainController.busy)
                MainController.saveModel(MainController.projectCtrl.projectFilePath)
        }
    }


    FileDialog {
        id: save
        currentFolder: open.currentFolder
        fileMode: FileDialog.SaveFile
        nameFilters: ["Project file (*.json)"]
        onAccepted: {
            MainController.projectCtrl.projectFilePath = localPath(save.selectedFile)
            MainController.saveModel(MainController.projectCtrl.projectFilePath)
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
                text: MainController.projectCtrl.videoFilePath
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
            id: menuFile
            title: qsTr("File")
            Action {
                text: qsTr("Open")
                onTriggered:open.open() // MainController.fileName = "file:///media/renaud/other/document/dcs/2024-06-18_17-41-45.mkv"
                // //
            }
            Action {
                text: qsTr("Save")
                onTriggered: {
                    if(MainController.projectCtrl.projectFilePath)
                        MainController.saveModel(MainController.projectCtrl.projectFilePath)
                    else
                        save.open()
                }
            }
            Action {
                text: qsTr("Save As")
                onTriggered: save.open()
            }
            Menu {
                title: qsTr("Recent")
                Menu {
                    id: recentVideosMenu
                    title: qsTr("Videos")
                    Repeater {
                        model: MainController.projectCtrl.recentVideos
                        MenuItem {
                            required property string filePath
                            required property string fileName

                            text: fileName
                            onTriggered: {
                                openFile(filePath)
                                menuFile.close()

                            }
                            /*Timer {
                                running: true
                                repeat: true
                                onTriggered: console.log("Textmenu: "+text)
                            }*/
                        }
                        onItemAdded: (pos, item) => {
                         recentVideosMenu.insertItem(pos,item)
                     }
                    }
                    MenuItem {
                        visible: MainController.projectCtrl.recentVideos.count > 0
                        text: qsTr("Clear")
                        onTriggered: MainController.projectCtrl.clearRecentVideos()
                    }
                }
                Menu {
                    id: recentProjectsMenu
                    title: qsTr("Projects")
                    Repeater {
                        model: MainController.projectCtrl.recentProjects
                        MenuItem {
                            text: display
                            onTriggered: {
                                openFile(path)
                                menuFile.close()
                            }
                        }
                        onItemAdded: (pos, item) => {
                            recentProjectsMenu.insertItem(pos,item)
                        }
                    }
                    MenuItem {
                        visible: MainController.projectCtrl.recentProjects.count > 0
                        text: qsTr("Clear")
                        onTriggered: MainController.projectCtrl.clearRecentProjects()
                    }
                }
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
                checked: MainController.projectCtrl.projectType === MainController.projectCtrl.Tutorial
                onTriggered: MainController.projectCtrl.projectType = MainController.projectCtrl.Tutorial
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        TopToolBar {
            Layout.fillWidth: true
        }
        EditorContent {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
