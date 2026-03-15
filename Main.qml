import QtQuick
import QtQuick.Controls.Basic
import QtMultimedia

Window {
    width: 400
    height: 450
    visible: true
    title: "Video Sender (Server)"
    color: "#1e1e1e"

    MediaDevices { id: devices }

    Camera {
        id: camera
        active: sender.active
        cameraDevice: devices.defaultVideoInput
    }

    VideoOutput {
        id: localOutput
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 20
        width: 320
        height: 240
        fillMode: VideoOutput.PreserveAspectFit
    }

    CaptureSession {
        camera: camera
        videoOutput: localOutput
    }

    Component.onCompleted: {
        sender.sourceSink = localOutput.videoSink;
    }

    Column {
        anchors.top: localOutput.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 20
        spacing: 15

        ComboBox {
            id: cameraSelector
            width: 250
            model: devices.videoInputs
            textRole: "description"
            onActivated: (index) => {
                camera.stop()
                camera.cameraDevice = model[index]
                if (sender.active) camera.start()
            }
        }

        Button {
            text: sender.active ? "STOP STREAMING" : "START STREAMING"
            width: 250
            height: 40
            background: Rectangle {
                color: sender.active ? "#d32f2f" : "#1976d2"
                radius: 5
            }
            contentItem: Text {
                text: parent.text
                color: "white"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                if (!camera.cameraDevice) camera.cameraDevice = devices.defaultVideoInput;
                sender.active = !sender.active;
            }
        }
    }
}
