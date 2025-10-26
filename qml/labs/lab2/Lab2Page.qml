import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import com.company.PciManager 1.0

Page {
    id: root
    title: "PCI Configuration Space"

    background: null

    Component.onCompleted: {
        PciManager.startServer()
    }

    Component.onDestruction: {
        PciManager.stopServer()
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
                text: "Конфигурационное пространство PCI"
                color: "white"
                font.pixelSize: 24
                font.bold: true
            }

            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: infoGrid.implicitHeight + 20
            color: "#000000"
            opacity: 0.7
            radius: 5

            GridLayout {
                id: infoGrid
                anchors.fill: parent
                anchors.margins: 10
                columns: 2
                columnSpacing: 20
                rowSpacing: 10

                Label {
                    text: "Статус:"
                    font.bold: true
                    color: "white"
                }
                Label {
                    text: PciManager.serverStatus
                    color: PciManager.serverRunning ? "#4CAF50" : "#F44336"
                    font.bold: true
                }

                Label {
                    text: "IP адрес сервера:"
                    font.bold: true
                    color: "white"
                }
                Label {
                    text: PciManager.getLocalIP() + ":12345"
                    font.family: "monospace"
                    color: "#FFD700"
                }

                Label {
                    text: "Подключенный клиент:"
                    font.bold: true
                    color: "white"
                }
                Label {
                    text: PciManager.clientIP || "Нет подключений"
                    color: PciManager.clientIP ? "#4CAF50" : "#FFA500"
                }

                Label {
                    text: "Найдено устройств:"
                    font.bold: true
                    color: "white"
                }
                Label {
                    text: PciManager.deviceCount
                    font.bold: true
                    color: "#00BCD4"
                    font.pixelSize: 16
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                text: PciManager.serverRunning ? "Остановить сервер" : "Запустить сервер"
                Layout.preferredWidth: 150

                background: Rectangle {
                    color: PciManager.serverRunning ? "#F44336" : "#4CAF50"
                    radius: 5
                    opacity: parent.pressed ? 0.8 : 1.0
                }

                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (PciManager.serverRunning) {
                        PciManager.stopServer()
                    } else {
                        PciManager.startServer()
                    }
                }
            }

            Button {
                text: "Очистить список"
                Layout.preferredWidth: 150
                enabled: PciManager.deviceCount > 0

                background: Rectangle {
                    color: parent.enabled ? "#FF9800" : "#666666"
                    radius: 5
                    opacity: parent.pressed ? 0.8 : 1.0
                }

                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: PciManager.clearDevices()
            }

            Item { Layout.fillWidth: true }

            Label {
                text: "IP для Windows XP: " + PciManager.getLocalIP()
                color: "#FFD700"
                font.italic: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#000000"
            opacity: 0.8
            radius: 5

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10

                Label {
                    text: "Список PCI устройств"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 16
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 35
                    color: "#1976D2"
                    radius: 3

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10

                        Label {
                            text: "Bus"
                            font.bold: true
                            color: "white"
                            Layout.preferredWidth: 60
                        }
                        Label {
                            text: "Device"
                            font.bold: true
                            color: "white"
                            Layout.preferredWidth: 60
                        }
                        Label {
                            text: "Function"
                            font.bold: true
                            color: "white"
                            Layout.preferredWidth: 70
                        }
                        Label {
                            text: "Vendor ID"
                            font.bold: true
                            color: "white"
                            Layout.preferredWidth: 100
                        }
                        Label {
                            text: "Device ID"
                            font.bold: true
                            color: "white"
                            Layout.preferredWidth: 100
                        }
                        Label {
                            text: "Производитель"
                            font.bold: true
                            color: "white"
                            Layout.fillWidth: true
                        }
                    }
                }

                ListView {
                    id: deviceList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: PciManager.devices

                    delegate: Rectangle {
                        width: deviceList.width
                        height: 30
                        color: "transparent"

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: 1
                            color: "#FFFFFF20"
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10

                            Label {
                                text: modelData.bus
                                color: "white"
                                Layout.preferredWidth: 60
                            }
                            Label {
                                text: modelData.device
                                color: "white"
                                Layout.preferredWidth: 60
                            }
                            Label {
                                text: modelData.function;
                                color: "white"
                                Layout.preferredWidth: 70
                            }
                            Label {
                                text: "0x" + modelData.vendorID
                                font.family: "monospace"
                                color: "#00BCD4"
                                Layout.preferredWidth: 100
                            }
                            Label {
                                text: "0x" + modelData.deviceID
                                font.family: "monospace"
                                color: "#00BCD4"
                                Layout.preferredWidth: 100
                            }
                            Label {
                                text: PciManager.getVendorName(modelData.vendorID)
                                color: "#4CAF50"
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        text: "Нет данных\nОжидание подключения от Windows XP..."
                        horizontalAlignment: Text.AlignHCenter
                        color: "#FFA500"
                        font.pixelSize: 16
                        visible: deviceList.count === 0
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            color: "#000000"
            opacity: 0.8
            radius: 5

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10

                Label {
                    text: "Журнал событий"
                    color: "white"
                    font.bold: true
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    TextArea {
                        id: logArea
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextArea.Wrap
                        font.family: "monospace"
                        font.pixelSize: 11
                        color: "#00FF00"
                        background: null

                        Connections {
                            target: PciManager
                            function onLogMessage(message) {
                                var time = new Date().toLocaleTimeString()
                                logArea.append("[" + time + "] " + message)
                            }
                            function onErrorOccurred(error) {
                                var time = new Date().toLocaleTimeString()
                                logArea.append("<font color='#FF5252'>[" + time + "] ОШИБКА: " + error + "</font>")
                            }
                        }
                    }
                }
            }
        }
    }
}
