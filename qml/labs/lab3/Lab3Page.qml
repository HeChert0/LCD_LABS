import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import com.company.HddManager 1.0

Page {
    id: root
    title: "HDD Information"

    background: null

    Component.onCompleted: {
        HddManager.startServer()
    }

    Component.onDestruction: {
        HddManager.stopServer()
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

        RowLayout {
            Layout.fillWidth: true

            Button {
                text: "← Назад"
                font.pixelSize: 14
                onClicked: stackView.pop()

                background: Rectangle {
                    color: parent.pressed ? "#666666" : "#444444"
                    radius: 5
                }

                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font: parent.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                text: "Информация о жёстких дисках"
                color: "white"
                font.pixelSize: 24
                font.bold: true
            }

            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#000000"
            opacity: 0.7
            radius: 5

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 20

                Label {
                    text: "Статус: " + HddManager.serverStatus
                    color: HddManager.serverRunning ? "#4CAF50" : "#F44336"
                    font.bold: true
                }

                Rectangle {
                    width: 1
                    height: 30
                    color: "#FFFFFF40"
                }

                Label {
                    text: "IP: " + HddManager.getLocalIP() + ":12346"
                    color: "#FFD700"
                    font.family: "monospace"
                }

                Rectangle {
                    width: 1
                    height: 30
                    color: "#FFFFFF40"
                }

                Label {
                    text: "Клиент: " + (HddManager.clientIP || "не подключен")
                    color: HddManager.clientIP ? "#4CAF50" : "#FFA500"
                }

                Rectangle {
                    width: 1
                    height: 30
                    color: "#FFFFFF40"
                }

                Label {
                    text: "Дисков: " + HddManager.driveCount
                    color: "#00BCD4"
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Очистить"
                    enabled: HddManager.driveCount > 0
                    onClicked: HddManager.clearDrives()

                    background: Rectangle {
                        color: parent.enabled ? "#FF9800" : "#666666"
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
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: driveList
                anchors.fill: parent
                model: HddManager.drives
                spacing: 10

                delegate: Rectangle {
                    width: driveList.width - 20
                    height: 180
                    color: "#000000"
                    opacity: 0.85
                    radius: 8
                    border.color: "#00BCD4"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 8

                        // Заголовок диска
                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text: "Диск " + (modelData.index + 1) + ": " + modelData.model
                                color: "white"
                                font.pixelSize: 18
                                font.bold: true
                            }

                            Item { Layout.fillWidth: true }

                            Label {
                                text: modelData.interface
                                color: "#00BCD4"
                                font.pixelSize: 14
                                font.bold: true
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#FFFFFF30"
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 30
                            rowSpacing: 5

                            Label {
                                text: "Производитель:"
                                color: "#B0B0B0"
                            }
                            Label {
                                text: modelData.manufacturer
                                color: "white"
                                font.bold: true
                            }

                            Label {
                                text: "Серийный номер:"
                                color: "#B0B0B0"
                            }
                            Label {
                                text: modelData.serial
                                color: "white"
                                font.family: "monospace"
                            }

                            Label {
                                text: "Версия прошивки:"
                                color: "#B0B0B0"
                            }
                            Label {
                                text: modelData.firmware
                                color: "white"
                            }

                            Label {
                                text: "Объём:"
                                color: "#B0B0B0"
                            }
                            Label {
                                text: modelData.totalFormatted
                                color: "#4CAF50"
                                font.bold: true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Label {
                                text: "Использование:"
                                color: "#B0B0B0"
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 20
                                color: "#333333"
                                radius: 10

                                Rectangle {
                                    width: parent.width * (parseFloat(modelData.usedPercent) / 100)
                                    height: parent.height
                                    radius: 10
                                    gradient: Gradient {
                                        orientation: Gradient.Horizontal
                                        GradientStop { position: 0.0; color: "#4CAF50" }
                                        GradientStop { position: 1.0; color: "#FF9800" }
                                    }
                                }

                                Label {
                                    anchors.centerIn: parent
                                    text: modelData.usedPercent + "%"
                                    color: "white"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }

                            Label {
                                text: modelData.usedFormatted + " / " + modelData.totalFormatted
                                color: "white"
                                font.pixelSize: 12
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text: "Режимы:"
                                color: "#B0B0B0"
                            }
                            Label {
                                Layout.fillWidth: true
                                text: modelData.modes
                                color: "#FFD700"
                                font.pixelSize: 11
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    text: "Нет данных о дисках\nОжидание подключения от Windows XP..."
                    horizontalAlignment: Text.AlignHCenter
                    color: "#FFA500"
                    font.pixelSize: 16
                    visible: driveList.count === 0
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            color: "#000000"
            opacity: 0.8
            radius: 5

            ScrollView {
                anchors.fill: parent
                anchors.margins: 10

                TextArea {
                    id: logArea
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextArea.Wrap
                    font.family: "monospace"
                    font.pixelSize: 10
                    color: "#00FF00"
                    background: null

                    Connections {
                        target: HddManager
                        function onLogMessage(message) {
                            var time = new Date().toLocaleTimeString()
                            logArea.append("[" + time + "] " + message)
                        }
                        function onErrorOccurred(error) {
                            var time = new Date().toLocaleTimeString()
                            logArea.append("<font color='#FF5252'>[" + time + "] ERROR: " + error + "</font>")
                        }
                    }
                }
            }
        }
    }
}
