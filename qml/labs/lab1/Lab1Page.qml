// qml/labs/lab1/Lab1Page.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import com.company.PowerManager 1.0

Item {
    id: root
    property StackView stackView: parent
    property var powerManager: PowerManager

    Rectangle {
        anchors.fill: parent;
        color: "black"
    }


    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        Label {
            text: "Лабораторная работа №1: Энергопитание"
            font.pixelSize: 24
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        GridLayout {
            columns: 2
            columnSpacing: 20
            Layout.fillWidth: true

            Label { text: "Тип энергопитания:"; color: "lightgray"; font.pixelSize: 18 }
            Label { text: powerManager.powerSourceType; color: "white"; font.pixelSize: 18 }

            Label { text: "Тип батареи:"; color: "lightgray"; font.pixelSize: 18 }
            Label { text: powerManager.batteryType; color: "white"; font.pixelSize: 18 }

            Label { text: "Уровень заряда:"; color: "lightgray"; font.pixelSize: 18 }
            Label { text: (powerManager.batteryLevel === -1 ? "Нет данных" : powerManager.batteryLevel + "%"); color: "white"; font.pixelSize: 18 }

            Label { text: "Режим энергосбережения:"; color: "lightgray"; font.pixelSize: 18 }
            Label { text: powerManager.powerSavingMode; color: "white"; font.pixelSize: 18 }

            Label { text: "Время работы (полный заряд):"; color: "lightgray"; font.pixelSize: 18 }
            Label { text: powerManager.batteryFullLifeTime; color: "white"; font.pixelSize: 18 }

            Label { text: "Оставшееся время работы:"; color: "lightgray"; font.pixelSize: 18 }
            Label { text: powerManager.batteryLifeTime; color: "white"; font.pixelSize: 18 }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 20

            Button {
                text: "Сон"
                onClicked: powerManager.sleep()
                width: win.width * 0.12
                height: win.height * 0.08
                background: Rectangle { color: "darkgreen"; radius: 5 }
                contentItem: Text { text: parent.text; font.pixelSize: parent.height * 0.5; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: "black" }
            }
            Button {
                text: "Гибернация"
                onClicked: powerManager.hibernate()
                width: win.width * 0.12
                height: win.height * 0.08
                background: Rectangle { color: "darkgreen"; radius: 5 }
                contentItem: Text { text: parent.text; font.pixelSize: parent.height * 0.5; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: "black" }
            }
        }

        Item { Layout.fillHeight: true } // Распорка, чтобы поднять контент вверх

        Button {
            id: backButton
            text: "Назад"
            Layout.alignment: Qt.AlignHCenter
            // Стиль
            width: win.width * 0.12
            height: win.height * 0.08
            background: Rectangle { color: "darkgreen"; radius: 5 }
            contentItem: Text { text: parent.text; font.pixelSize: parent.height * 0.5; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: "black" }

            onClicked: {
                stackView.pop();
            }
        }
    }
}
