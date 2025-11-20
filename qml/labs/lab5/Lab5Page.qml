import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import com.company.UsbManager 1.0

Page {
    id: root
    title: "USB Device Monitor"

    background: Image {
        source: (typeof projectDir !== "undefined" && projectDir !== "") ?
                projectDir + "resources/images/MM_bg.png" : "qrc:/images/MM_bg.png"
        fillMode: Image.PreserveAspectCrop
    }

    Component.onCompleted: {
        // Запускаем первое сканирование при открытии страницы
        UsbManager.initialScan();
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        // --- Верхняя панель ---
        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            Button {
                text: "← Назад"
                onClicked: stackView.pop()
                // Стилизация кнопки для единообразия
                background: Rectangle { color: parent.pressed ? "#666" : "#444"; radius: 5 }
                contentItem: Text { text: parent.text; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }

            TudaSudaSprite {
                scale: 0.8
                Layout.alignment: Qt.AlignVCenter
            }

            Item { Layout.fillWidth: true }

            Label {
                text: "USB Device Monitor"
                color: "white"
                font.pixelSize: 24
                font.bold: true
            }

            Item { Layout.fillWidth: true }
        }

        // --- Список устройств и кнопка ---
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20

            // Список
            ListView {
                id: deviceListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: UsbManager.devices
                clip: true
                spacing: 10

                delegate: Rectangle {
                    width: parent.width
                    height: 60
                    color: deviceListView.currentIndex === index ? "#00BCD4" : "#000000A0"
                    radius: 8
                    border.color: deviceListView.currentIndex === index ? "white" : "#00BCD4"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 15

                        Label {
                            text: modelData.displayName
                            color: "white"
                            font.pixelSize: 16
                            // Делаем жирным, если можно извлечь
                            font.bold: modelData.isEjectable
                            elide: Text.ElideRight
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: deviceListView.currentIndex = index
                    }
                }

                ScrollIndicator.vertical: ScrollIndicator { }
            }

            // Кнопка извлечения
            Button {
                            // Текст кнопки теперь более общий
                            text: "Отключить\nустройство"
                            Layout.alignment: Qt.AlignTop
                            Layout.topMargin: 10
                            // Кнопка активна, если любое устройство выбрано
                            enabled: deviceListView.currentIndex !== -1

                            onClicked: {
                                var selectedDevice = UsbManager.devices[deviceListView.currentIndex];

                                // --- НОВАЯ ЛОГИКА ---
                                if (selectedDevice.isEjectable) {
                                    // Если это флешка, вызываем ejectDevice
                                    UsbManager.ejectDevice(selectedDevice.id);
                                } else {
                                    // Если это мышь или другое устройство, вызываем disableDevice
                                    UsbManager.disableDevice(selectedDevice.devInst);
                                }
                            }

                            background: Rectangle {
                                color: parent.enabled ? "#F44336" : "#666666"
                                radius: 5
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }


        // --- Панель логов ---
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            color: "#DDDDDD" // Светлый фон для читаемости черного текста
            radius: 5

            ScrollView {
                anchors.fill: parent
                anchors.margins: 10
                clip: true

                TextArea {
                    id: logArea
                    readOnly: true
                    color: "black" // Черный цвет текста
                    font.family: "monospace"
                    background: null

                    Connections {
                        target: UsbManager
                        function onLogMessage(message) {
                            logArea.append(new Date().toLocaleTimeString() + ": " + message)
                        }
                    }
                }
                ScrollIndicator.vertical: ScrollIndicator { }
            }
        }
    }
}
