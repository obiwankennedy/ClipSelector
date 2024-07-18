import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.qmlmodels

TableView {
    id: _root

    model: MainController.clipModel
    resizableColumns: true
    alternatingRows: true
    columnSpacing: 3
    rowSpacing: 3


    MusicSelector {
        id: musicSelector
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
    }

    columnWidthProvider: function(column) {
        return (_root.width-(_root.columnSpacing*3)) / 6;
    }

    Layout.fillWidth: true
    editTriggers : TableView.SingleTapped
    DelegateChooser {
        id: chooser
        role: "type"
        DelegateChoice {
            column: 0
            Text {
                text: begin
                TableView.editDelegate: TextField {
                    anchors.fill: parent
                    text: begin
                    horizontalAlignment: TextInput.AlignHCenter
                    verticalAlignment: TextInput.AlignVCenter
                    Component.onCompleted: selectAll()

                    TableView.onCommit: {
                        begin = text
                    }
                }
            }
        }
        DelegateChoice {
            column: 1
            Text {
                text: end
                TableView.editDelegate: TextField {
                    anchors.fill: parent
                    text: end
                    horizontalAlignment: TextInput.AlignHCenter
                    verticalAlignment: TextInput.AlignVCenter
                    Component.onCompleted: selectAll()

                    TableView.onCommit: {
                        end = text
                    }
                }
            }
        }
        DelegateChoice {
            column: 2
            Text {
                text: "%1s".arg((end-begin)/1000)
            }
        }
        DelegateChoice {
            column: 3
            Text {
                text: label
                TableView.editDelegate: TextField {
                    anchors.fill: parent
                    text: label
                    horizontalAlignment: TextInput.AlignHCenter
                    verticalAlignment: TextInput.AlignVCenter
                    Component.onCompleted: selectAll()

                    TableView.onCommit: {
                        label = text
                    }
                }
            }
        }
        DelegateChoice {
            column: 4
            Button {
                text: music ? music : "music"
                onClicked: {
                    musicSelector.indexClip = row
                    musicSelector.open()
                }
            }
        }
        DelegateChoice {
            column: 5
            Text {
                text: offset
                TableView.editDelegate: TextField {
                    anchors.fill: parent
                    text: offset
                    horizontalAlignment: TextInput.AlignHCenter
                    verticalAlignment: TextInput.AlignVCenter
                    Component.onCompleted: selectAll()

                    TableView.onCommit: {
                        offset = text
                    }
                }
            }
        }
    }
    delegate: chooser

    Rectangle {
        anchors.fill: parent
        color: "green"
        opacity: 0.2
    }

    /*Timer {
        running: true
        repeat: true
        onTriggered: console.log("Tableview:",_root.x," w:",_root.width," h:",_root.height)
    }*/
}
