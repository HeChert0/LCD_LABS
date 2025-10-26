import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtMultimedia
import com.company.CameraManager 1.0

Page {
    id: root
    title: "Camera Surveillance"

    property bool cameraWarning: false

    Component.onCompleted: {
        if (CameraManager.cameraAvailable) {
            CameraManager.startCamera()
        }
    }

    Component.onDestruction: {
        CameraManager.stopCamera()
    }

    // Обработка сигналов для анимации
    Connections {
        target: CameraManager
        function onCameraDetected() {
            cameraWarning = true
            warningTimer.restart()
        }
        function onStealthPhotoTaken() {
            statusText.text = "📸 Stealth photo captured!"
            statusTimer.restart()
        }
        function onStealthRecordingStarted() {
            statusText.text = "🔴 Stealth recording started..."
            statusTimer.stop()
        }
        function onStealthRecordingStopped() {
            statusText.text = "⏹️ Stealth recording stopped"
            statusTimer.restart()
        }
        function onHotkeyPressed(action) {
            hotkeyText.text = "Hotkey: " + action
            hotkeyTimer.restart()
        }
    }

    Timer {
        id: warningTimer
        interval: 3000
        onTriggered: cameraWarning = false
    }

    Timer {
        id: statusTimer
        interval: 3000
        onTriggered: statusText.text = ""
    }

    Timer {
        id: hotkeyTimer
        interval: 2000
        onTriggered: hotkeyText.text = ""
    }

    Image {
        anchors.fill: parent
        source: (typeof projectDir !== "undefined" && projectDir !== "") ?
                projectDir + "resources/images/MM_bg.png" : "qrc:/images/MM_bg.png"
        fillMode: Image.PreserveAspectCrop
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        // Заголовок
        RowLayout {
            Layout.fillWidth: true

            Button {
                text: "← Back"
                onClicked: stackView.pop()

                background: Rectangle {
                    color: parent.pressed ? "#666666" : "#444444"
                    radius: 5
                }

                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                text: "Camera Surveillance System"
                color: "white"
                font.pixelSize: 24
                font.bold: true
            }

            Item { Layout.fillWidth: true }
        }

        // Панель информации
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            color: "#000000"
            opacity: 0.8
            radius: 10

            GridLayout {
                anchors.fill: parent
                anchors.margins: 15
                columns: 2
                rowSpacing: 5
                columnSpacing: 20

                Label {
                    text: "Camera:"
                    color: "#00BCD4"
                    font.bold: true
                }
                Label {
                    text: CameraManager.cameraName || "Not detected"
                    color: CameraManager.cameraAvailable ? "#4CAF50" : "#F44336"
                }

                Label {
                    text: "Status:"
                    color: "#00BCD4"
                    font.bold: true
                }
                Label {
                    text: CameraManager.cameraActive ?
                          (CameraManager.recording ? "🔴 Recording" : "✅ Active") : "⏹️ Stopped"
                    color: "white"
                }

                Label {
                    text: "Photos:"
                    color: "#00BCD4"
                    font.bold: true
                }
                Label {
                    text: CameraManager.photoCount
                    color: "white"
                }

                Label {
                    text: "Videos:"
                    color: "#00BCD4"
                    font.bold: true
                }
                Label {
                    text: CameraManager.videoCount +
                          (CameraManager.recording ? " (Recording: " + CameraManager.recordingTime + ")" : "")
                    color: "white"
                }
            }
        }

        // Видео превью
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "black"
            radius: 10

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                anchors.margins: 2
                fillMode: VideoOutput.PreserveAspectFit

                Component.onCompleted: {
                    CameraManager.videoSink = videoOutput.videoSink
                }
            }

            // Оверлей при неактивной камере
            Rectangle {
                anchors.fill: parent
                color: "#000000"
                opacity: CameraManager.cameraActive ? 0 : 0.8
                visible: opacity > 0

                Label {
                    anchors.centerIn: parent
                    text: CameraManager.cameraAvailable ? "Camera Stopped" : "No Camera Found"
                    color: "#FFA500"
                    font.pixelSize: 24
                }
            }

            // Индикатор записи
            Rectangle {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 10
                width: 120
                height: 40
                color: "#FF0000"
                radius: 20
                visible: CameraManager.recording

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 5

                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: "white"

                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            running: CameraManager.recording
                            NumberAnimation { to: 0; duration: 500 }
                            NumberAnimation { to: 1; duration: 500 }
                        }
                    }

                    Label {
                        text: "REC " + CameraManager.recordingTime
                        color: "white"
                        font.bold: true
                    }
                }
            }
        }

        // Панель управления
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            color: "#000000"
            opacity: 0.85
            radius: 10

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                // Основные кнопки
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Button {
                        text: CameraManager.cameraActive ? "Stop Camera" : "Start Camera"
                        Layout.fillWidth: true
                        enabled: CameraManager.cameraAvailable
                        onClicked: CameraManager.cameraActive ?
                                  CameraManager.stopCamera() : CameraManager.startCamera()

                        background: Rectangle {
                            color: parent.enabled ?
                                  (CameraManager.cameraActive ? "#F44336" : "#4CAF50") : "#666666"
                            radius: 5
                        }

                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Button {
                        text: "📸 Photo"
                        Layout.fillWidth: true
                        enabled: CameraManager.cameraActive && !CameraManager.recording
                        onClicked: CameraManager.takePhoto()

                        background: Rectangle {
                            color: parent.enabled ? "#2196F3" : "#666666"
                            radius: 5
                        }

                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Button {
                        text: CameraManager.recording ? "⏹️ Stop" : "🔴 Record"
                        Layout.fillWidth: true
                        enabled: CameraManager.cameraActive
                        onClicked: CameraManager.recording ?
                                  CameraManager.stopRecording() : CameraManager.startRecording()

                        background: Rectangle {
                            color: parent.enabled ?
                                  (CameraManager.recording ? "#FF9800" : "#F44336") : "#666666"
                            radius: 5
                        }

                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Button {
                        text: "🕵️ Stealth"
                        Layout.fillWidth: true
                        checkable: true
                        checked: CameraManager.stealthMode
                        onClicked: CameraManager.toggleStealthMode()

                        background: Rectangle {
                            color: parent.checked ? "#9C27B0" : "#673AB7"
                            radius: 5
                        }

                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                // Информация о горячих клавишах
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#1A1A1A"
                    radius: 5

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 20

                        Label {
                            text: "Hotkeys:"
                            color: "#FFD700"
                            font.bold: true
                        }

                        Label {
                            text: "Ctrl+Shift+P - Stealth Photo"
                            color: "#00BCD4"
                            font.pixelSize: 11
                        }

                        Label {
                            text: "Ctrl+Shift+R - Start Recording"
                            color: "#00BCD4"
                            font.pixelSize: 11
                        }

                        Label {
                            text: "Ctrl+Shift+S - Stop Recording"
                            color: "#00BCD4"
                            font.pixelSize: 11
                        }

                        Label {
                            text: "Ctrl+Shift+H - Show/Hide"
                            color: "#00BCD4"
                            font.pixelSize: 11
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            id: hotkeyText
                            color: "#4CAF50"
                            font.bold: true
                        }
                    }
                }
            }
        }

        // Статус бар
        Label {
            id: statusText
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            color: "#4CAF50"
            font.pixelSize: 14
            font.bold: true
        }
    }

    // Анимированный персонаж - предупреждение о камере
    CameraWarningSprite {
        id: warningSprite
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 20
        visible: cameraWarning
        width: 140
        height: 188
    }
}
