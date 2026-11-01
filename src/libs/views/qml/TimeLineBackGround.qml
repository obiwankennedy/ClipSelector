import QtQuick

Rectangle {
    id: root
    color: "transparent"
    required property real position
    required property real duration

    // Pixels per second (set by the panel; includes the zoom factor).
    property real secondInPixel: 0

    // Height of the bottom strip that carries the time labels.
    readonly property real labelStripHeight: 20

    // Tick spacing in seconds: the first "nice" step whose labels are at
    // least ~45 px apart. With the timeline fitted to the panel this gives
    // 10 / 20 / 30 s for typical videos, finer steps when zoomed in and
    // coarser steps for long videos, so the labels never overlap.
    readonly property real tickStep: {
        var steps = [1, 2, 5, 10, 15, 30, 60, 120, 300, 600]
        for (var i = 0; i < steps.length; i++)
            if (steps[i] * secondInPixel >= 45)
                return steps[i]
        return 600
    }

    signal cleanSelection()

    Rectangle {
        id: clipLine
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.labelStripHeight
        color: "#cccccc"
        radius: 10
        border.width: 1
        border.color: "black"
        Rectangle {
            color: "green"
            height: parent.height
            width: 2
            x: (root.position/1000)*root.secondInPixel
        }
    }

    // Red tick every `tickStep` seconds, with the time label (10s, 20s,
    // 30s, ...) in the bottom strip just to the right of the line.
    Repeater {
        id: repeater
        model: root.duration > 0 ? Math.floor(root.duration / 1000 / root.tickStep) : 0
        delegate:  Item {
            height: root.height
            x: (index + 1) * root.tickStep * root.secondInPixel
            Rectangle {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.bottomMargin: root.labelStripHeight
                color: "red"
                opacity: 0.9
                width: 1
            }
            Text {
                id: timeLbl
                anchors.bottom: parent.bottom
                x: 4
                text: "%1s".arg((index + 1) * root.tickStep)
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.cleanSelection()
    }
}
