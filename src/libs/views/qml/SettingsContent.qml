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
}
