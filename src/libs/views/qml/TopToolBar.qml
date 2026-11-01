import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Core

RowLayout {
    id: topToolBar

    Switch {
        id: projectTypeSwitch
        text: qsTr("Tutorial")
        checked: MainController.projectCtrl.projectType === ProjectController.Tutorial
        onToggled: {
            if(checked)
                MainController.projectCtrl.projectType = ProjectController.Tutorial
            else
                MainController.projectCtrl.projectType = ProjectController.MusicClip
        }
    }

    Item {
        Layout.fillWidth: true
    }
}
