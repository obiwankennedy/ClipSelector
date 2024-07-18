import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Views

Dialog {
    id: root
    width: 800
    height: 600
    standardButtons: Dialog.Ok | Dialog.Cancel
    property string selectedSong
    property int indexClip

    ColumnLayout {
        anchors.fill: parent

        TextField {
            id: patternEdit
            Layout.fillWidth: true
            onTextEdited: MainController.setPattern(patternEdit.text)
        }

        ListView {
            id: view
            model: MainController.wantedMusics
            Layout.fillHeight: true
            Layout.fillWidth: true
            highlightFollowsCurrentItem: true
            highlight: Rectangle {
                color: "blue"
                opacity: 0.4
            }

            delegate: MouseArea {
                width: view.width
                height: lyt.implicitHeight+4
                required property int index
                required property string title
                required property string artist
                required property string path

                RowLayout {
                    id: lyt
                    anchors.fill: parent
                    Label {
                        text: title
                        Layout.fillWidth: true
                    }
                    Label {
                        text: artist
                        Layout.fillWidth: true
                    }
                }
                onClicked: {
                    root.selectedSong = path
                    view.currentIndex = index
                }
            }
        }

    }
    onAccepted: {
        MainController.setMusic(root.indexClip, root.selectedSong)
    }
}
