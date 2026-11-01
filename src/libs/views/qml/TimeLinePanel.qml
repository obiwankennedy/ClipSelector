import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia
import Core

ColumnLayout {
    id: root
    required property MediaPlayer player

    // Timeline zoom: 1.0 = the whole video fits the panel width.
    property real zoom: 1.0

    // Pixels per second at the current zoom.
    function secondInPixel() {
        return root.player.duration > 0
             ? timelineFlick.width * root.zoom / (root.player.duration/1000)
             : 0
    }

    function setZoomFactor(newZoom) {
        newZoom = Math.max(1.0, Math.min(32.0, newZoom))
        if (newZoom === zoom || root.player.duration <= 0)
            return
        // Keep the point at the center of the viewport fixed while zooming.
        var center = timelineFlick.contentX + timelineFlick.width / 2
        var frac = timelineFlick.width * zoom > 0 ? center / (timelineFlick.width * zoom) : 0
        zoom = newZoom
        timelineFlick.contentX = Math.min(Math.max(0,
                                                    frac * timelineFlick.width * newZoom - timelineFlick.width / 2),
                                           Math.max(0, timelineFlick.width * newZoom - timelineFlick.width))
    }

    function zoomIn() { setZoomFactor(zoom * 2) }
    function zoomOut() { setZoomFactor(zoom / 2) }

    // Scroll the timeline so the playhead stays visible.
    function ensurePlayheadVisible() {
        var x = (root.player.position/1000) * secondInPixel()
        var cw = root.player.duration > 0 ? timelineFlick.width * root.zoom : 0
        if (x < timelineFlick.contentX)
            timelineFlick.contentX = Math.max(0, x - 10)
        else if (x > timelineFlick.contentX + timelineFlick.width)
            timelineFlick.contentX = Math.min(Math.max(0, cw - timelineFlick.width),
                                               x + 10 - timelineFlick.width)
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 4
        Slider {
            id: timeLineSlider
            Layout.fillWidth: true
            from: 0
            to: root.player.duration
            value: root.player.position
            enabled: root.player.seekable
            onMoved: root.player.position = timeLineSlider.value
        }
        Button {
            text: "\u2212"
            onClicked: root.zoomOut()
        }
        Button {
            text: "+"
            onClicked: root.zoomIn()
        }
        Button {
            text: qsTr("Reset")
            visible: Math.abs(root.zoom - 1.0) > 0.001
            onClicked: root.setZoomFactor(1.0)
        }
    }

    Flickable {
        id: timelineFlick
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredHeight: 200
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: root.secondInPixel() * (root.player.duration/1000)
        contentHeight: height

        // Mouse wheel zooms the timeline.
        WheelHandler {
            onWheel: {
                if (wheel.angleDelta.y > 0)
                    root.zoomIn()
                else if (wheel.angleDelta.y < 0)
                    root.zoomOut()
            }
        }

        TimeLineBackGround {
            id: bottomPanel
            width: timelineFlick.contentWidth
            height: timelineFlick.contentHeight
            color: "transparent"
            position: root.player.position
            duration: root.player.duration
            secondInPixel: root.secondInPixel()
            onPositionChanged: root.ensurePlayheadVisible()

            onCleanSelection: {
                console.log("Clear selection")
                MainController.videoCtrl.clearSelection()
            }


            Repeater {
                id: rep
                delegateModelAccess: DelegateModel.ReadWrite
                model: MainController.videoCtrl.clipListModel
                TimeLineToken {
                    id: delegate

                    required property real begin
                    required property real end
                    required property color clipColor
                    required property string label
                    color: clipColor
                    radius: 4

                    function updateData() {
                        begin = delegate.x * 1000 / bottomPanel.secondInPixel
                        end = (delegate.x + width) * 1000 / bottomPanel.secondInPixel
                    }

                    secondInPixel: bottomPanel.secondInPixel

                    height: 20
                    x: (begin/1000)*bottomPanel.secondInPixel
                    width: ((end-begin)/1000)*bottomPanel.secondInPixel
                    onTimeChanged: {
                        updateData()
                    }
                }
            }

            Repeater {
                id: rep2
                delegateModelAccess: DelegateModel.ReadWrite
                model: MainController.tutoCtrl.effects
                TimeLineEffectToken {
                    id: effect

                    required property real begin
                    required property real end
                    required property int rowIndex
                    required property VideoTimedEvent effectCtrl
                    required property color eventColor

                    timedEvent: effectCtrl
                    radius: 4

                    function updateData() {
                        begin = effect.x * 1000 / bottomPanel.secondInPixel
                        end = (effect.x + width) * 1000 / bottomPanel.secondInPixel
                    }

                    secondInPixel: bottomPanel.secondInPixel

                    height: 28
                    x: (begin/1000)*bottomPanel.secondInPixel
                    width: ((end-begin)/1000)*bottomPanel.secondInPixel
                    onTimeChanged: {
                        updateData()
                    }
                    onEditionRequested: {
                        effectEditor.token = effect
                        effectEditor.open()
                    }
                }
            }
        }
    }

    // Inline editor for the editable property of a timeline effect token
    // (comment text, speed factor). Double-clicking a token opens it.
    Dialog {
        id: effectEditor
        property var token
        anchors.centerIn: Overlay.overlay
        width: 460
        padding: 12
        onAccepted: {
            if(token)
                token.value = effectField.text
        }

        ColumnLayout {
            spacing: 8
            Label {
                text: qsTr("Edit effect")
            }
            // Multi-line: audio comments can be several sentences long, and
            // the text is wrapped to the dialog width. A TextArea inserts a
            // line break on Enter (a TextField would not), and the dialog is
            // accepted with the OK button.
            TextArea {
                id: effectField
                Layout.fillWidth: true
                Layout.preferredHeight: 96
                text: effectEditor.token ? effectEditor.token.value : ""
                wrapMode: TextArea.Wrap
                activeFocusOnPress: true
                selectByMouse: true
                background: Rectangle {
                    radius: 8
                    color: "white"
                    border.color: "black"
                    border.width: 2
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Item {
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("OK")
                    onClicked: effectEditor.accept()
                }
                Button {
                    text: qsTr("Cancel")
                    onClicked: effectEditor.reject()
                }
            }
        }
    }

    // A new video resets the timeline to the fitted view.
    Connections {
        target: root.player
        function onDurationChanged() {
            root.setZoomFactor(1.0)
        }
    }
}
