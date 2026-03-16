import QtQuick
import QtQuick.Controls.Basic
import VideoReceiver 1.0 // Убедись, что URI совпадает с CMake

Window {
    width: 800
    height: 600
    visible: true
    title: "Video Receiver"
    color: "#1a1a1a"

    QmlImageItem {
        id: videoDisplay
        anchors.fill: parent
        image: receiver.currentFrame
    }

    Rectangle {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 20
        width: 300
        height: 120
        color: "#CC000000"
        radius: 10
        border.color: "#33ffffff"

        Column {
            anchors.centerIn: parent
            spacing: 10

            TextField {
                id: ipField
                width: 250
                placeholderText: "Введите IP (например, 127.0.0.1)"
                text: receiver.serverIp
                color: "white"
                background: Rectangle {
                    color: "#333"
                    border.color: ipField.activeFocus ? "#0078d4" : "#666"
                }
                onTextChanged: receiver.serverIp = text
            }

            Button {
                text: "ПОДКЛЮЧИТЬСЯ"
                width: 250
                onClicked: {
                    console.log("Запрос трансляции на: " + receiver.serverIp)
                    receiver.startStreaming()
                }
            }
        }
    }

    Text {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 10
        color: "white"
        text: "Статус: " + (receiver.currentFrame.width > 0 ? "Стриминг..." : "Ожидание...")
    }
}
