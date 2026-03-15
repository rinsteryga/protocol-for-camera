import QtQuick
import QtQuick.Controls.Basic
import App 1.0

Window {
    width: 640
    height: 480
    visible: true
    title: "Video Receiver (Client)"
    color: "black"

    QmlImage {
        id: udpOutput
        anchors.fill: parent
        image: receiver.currentFrame
    }

    Text {
        anchors.centerIn: parent
        color: "white"
        text: "Listening on UDP port 5555..."
        font.pixelSize: 18
        font.bold: true
        visible: receiver.currentFrame.width === 0
    }
}
