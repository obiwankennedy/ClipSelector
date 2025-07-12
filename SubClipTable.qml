import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.qmlmodels

TableView {
    id: _root

    property alias video: offsetSelector.video

    model: MainController.clipModel
    resizableColumns: true
    alternatingRows: true
    columnSpacing: 3
    rowSpacing: 6



    MusicSelector {
        id: musicSelector
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
    }

    OffsetSelector {
        id: offsetSelector
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
            ColumnLayout {
                id: lyt
                implicitHeight: repeater.count * 20
                Repeater {
                    id: repeater
                    model: ranges
                    onCountChanged: _root.forceLayout()
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            text: begin
                            onTextEdited: {
                                begin = text
                            }
                           /* TableView.editDelegate: TextField {
                                anchors.fill: parent
                                text: begin
                                horizontalAlignment: TextInput.AlignHCenter
                                verticalAlignment: TextInput.AlignVCenter
                                Component.onCompleted: selectAll()

                                TableView.onCommit: {
                                    begin = text
                                }
                            }*/
                        }
                        TextField {
                            text: end
                            onTextEdited: {
                                end = text
                            }

                            /*TableView.editDelegate: TextField {
                                anchors.fill: parent
                                text: end
                                horizontalAlignment: TextInput.AlignHCenter
                                verticalAlignment: TextInput.AlignVCenter
                                Component.onCompleted: selectAll()

                                TableView.onCommit: {
                                    modelData.begin = text
                                }
                            }*/
                        }
                    }

                }
            }
        }
        DelegateChoice {
            column: 1
            Text {
                text: "%1s".arg(duration)
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
        DelegateChoice {
            column: 2
            Text {
                text: label
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
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
            column: 3
            Button {
                text: music ? music : "music"
                onClicked: {
                    //music = "/media/renaud/musique/bangles/emule/Bangles - Walk Like An Egyptian.mp3"
                    musicSelector.indexClip = row
                    musicSelector.open()
                }
            }
        }
        DelegateChoice {
            column: 4

            Button {
                text: offset ? offset : "offset"
                onClicked: {
                    offsetSelector.model = ranges
                    offsetSelector.indexClip = row
                    offsetSelector.song = music
                    offsetSelector.open()
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
}
