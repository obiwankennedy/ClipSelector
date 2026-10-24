import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia
import Core

ColumnLayout {
    id: root
    required property MediaPlayer player

    Slider {
        id: timeLine
        Layout.fillWidth: true
        from: 0
        to: player.duration
        value: player.position
        enabled: player.seekable
        onMoved: player.position = timeLine.value
    }

    StackLayout {
        id: layout
        currentIndex: bar.currentIndex
        Layout.fillHeight: true

        Rectangle {
            id: bottomPanel
            color: "transparent"
            Rectangle {
                id: clipLine
                property real secondInPixel: clipLine.width/(player.duration/1000)
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: statusBar.top
                color: "#cccccc"
                radius: 10
                border.width: 1
                border.color: "black"

                Repeater {
                    id: rep
                    model: MainController.clipListModel
                    Rectangle {
                        required property real begin
                        required property real end
                        required property color clipColor
                        required property string label
                        color: clipColor
                        border.width: 1
                        border.color: Qt.darker(clipColor)
                        radius: 4

                        height: 20
                        x: (begin/1000)*clipLine.secondInPixel
                        width: ((end-begin)/1000)*clipLine.secondInPixel

                        // Duration (seconds) shown in the middle of the clip.
                        Rectangle {
                            id: durationBadge
                            anchors.centerIn: parent
                            width: Math.max(implicitWidth + 8, 16)
                            height: 14
                            radius: 7
                            color: "white"
                            opacity: 0.55
                            visible: parent.width > 18
                            Text {
                                anchors.centerIn: parent
                                text: Math.round((parent.end - parent.begin) / 1000) + "s"
                                font.pixelSize: 10
                                color: "black"
                            }
                        }

                        // Small cross centered on the clip (kept on top so the clip stays visible).
                        Item {
                            id: crossMarker
                            anchors.centerIn: parent
                            width: 14
                            height: 14
                            z: 5
                            visible: parent.width > 18
                            Rectangle {
                                anchors.centerIn: parent
                                width: parent.width
                                height: 1
                                color: "white"
                            }
                            Rectangle {
                                anchors.centerIn: parent
                                width: 1
                                height: parent.height
                                color: "white"
                            }
                        }

                        // Draggable: drag left/right edges to resize, Shift+drag to move.
                        MouseArea {
                            id: dragArea
                            anchors.fill: parent
                            hoverEnabled: true
                            property int edge: 0 // 0: move/none, 1: left, 2: right
                            property real grabBegin: 0
                            property real grabEnd: 0
                            property real grabX: 0
                            acceptedButtons: Qt.LeftButton
                            onPressed: {
                                grabBegin = parent.begin
                                grabEnd = parent.end
                                grabX = mouse.x
                                if(mouse.x < 8)
                                    edge = 1
                                else if(mouse.x > parent.width - 8)
                                    edge = 2
                                else
                                    edge = 0
                                cursorShape = (edge !== 0) ? Qt.SizeHorCursor
                                                           : ((mouse.modifiers & Qt.ShiftModifier) ? Qt.SizeAllCursor : Qt.ArrowCursor)
                            }
                            onPositionChanged: {
                                if(!pressed) {
                                    cursorShape = (mouse.x < 8 || mouse.x > parent.width - 8) ? Qt.SizeHorCursor
                                                    : ((mouse.modifiers & Qt.ShiftModifier) ? Qt.SizeAllCursor : Qt.ArrowCursor)
                                    return
                                }
                                var deltaMs = (mouse.x - grabX) * 1000 / clipLine.secondInPixel
                                var maxMs = player.duration
                                if(edge === 1) {
                                    var nb = Math.max(0, Math.min(grabBegin + deltaMs, grabEnd - 1000))
                                    if(nb !== parent.begin)
                                        model.begin = nb
                                } else if(edge === 2) {
                                    var ne = Math.min(maxMs, Math.max(grabEnd + deltaMs, grabBegin + 1000))
                                    if(ne !== parent.end)
                                        model.end = ne
                                } else if(mouse.modifiers & Qt.ShiftModifier) {
                                    var span = grabEnd - grabBegin
                                    var nbegin = Math.max(0, Math.min(grabBegin + deltaMs, maxMs - span))
                                    if(nbegin !== parent.begin) {
                                        model.begin = nbegin
                                        model.end = nbegin + span
                                    }
                                }
                            }
                            onReleased: {
                                edge = 0
                                cursorShape = Qt.ArrowCursor
                            }
                        }
                    }
                }

                Rectangle {

                    color: "green"
                    height: parent.height
                    width: 2
                    x: (player.position/1000)*clipLine.secondInPixel
                }
            }

            Repeater {
                id: repeater
                model: clipLine.width / (clipLine.secondInPixel * 10) + 1
                delegate:  Item {
                    property int minuteCount: 10
                    x: index * clipLine.secondInPixel * minuteCount
                    height: clipLine.height + timeLbl.height
                    Rectangle {
                        id: line
                        anchors.top: parent.top
                        anchors.bottom: timeLbl.top
                        color: "red"
                        opacity: 0.9
                        width: 1
                    }
                    Text {
                        id: timeLbl
                        height: implicitHeight
                        anchors.bottom: parent.bottom
                        anchors.horizontalCenter: line.horizontalCenter
                        text: "%1s".arg(index * minuteCount)
                    }
                }
            }

            Text {
                id: statusBar
                anchors.bottom: parent.bottom
                text: ""
                color: "transparent"
            }

        }
        SubClipTable {
            id: tableView
            video: MainController.videoFilePath
            Layout.preferredHeight: 300
        }
    }
    TabBar {
        id: bar
        Layout.fillWidth: true
        TabButton {
            text: qsTr("TimeLine")
            width: 200
        }
        TabButton {
            text: qsTr("Tableview")
            width: 200
        }
    }
}
