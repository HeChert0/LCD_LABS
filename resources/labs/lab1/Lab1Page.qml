// qml/labs/lab1/Lab1Page.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import com.company.PowerManager 1.0

Item {
    id: root
    property StackView stackView: parent

    // --- 1. ФОН ---
    Image {
        anchors.fill: parent
        source: (typeof projectDir !== "undefined" && projectDir !== "") ?
                projectDir + "resources/images/MM_bg.png" : "qrc:/images/MM_bg.png"
        fillMode: Image.PreserveAspectCrop
    }

    // --- 2. ОБНОВЛЕННЫЕ СВОЙСТВА ДЛЯ АНИМАЦИИ (с вашими размерами) ---
    property var fireFrameWidths: [55, 50, 65, 65, 57, 43, 80, 85]
    property var pilaFrameWidths: [155, 150, 135, 135, 150, 145, 140, 142]
    property int animationFrameIndex: 0

    readonly property bool onAcPower: powerManager.powerSourceType === "От сети"
    readonly property var currentFrameWidths: onAcPower ? fireFrameWidths : pilaFrameWidths

    // --- 3. СКРЫТЫЕ IMAGE ДЛЯ ПРЕДЗАГРУЗКИ СПРАЙТОВ В ПАМЯТЬ ---
    // Это необходимо для того, чтобы Canvas мог мгновенно их отрисовывать
    Image { id: fireSpriteSheet; source: (typeof projectDir !== "undefined" && projectDir !== "") ? projectDir + "resources/images/MM_fire.png" : "qrc:/images/MM_fire.png"; visible: false }
    Image { id: pilaSpriteSheet; source: (typeof projectDir !== "undefined" && projectDir !== "") ? projectDir + "resources/images/MM_pila.png" : "qrc:/images/MM_pila.png"; visible: false }


    // --- ОСНОВНОЙ ИНТЕРФЕЙС ---
    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        Label { text: "Лабораторная работа №1: Энергопитание"; font.pixelSize: 24; color: "white"; Layout.alignment: Qt.AlignHCenter }
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
            Button { text: "Сон";
                onClicked: powerManager.sleep();
                width: root.width * 0.12;
                height: root.height * 0.08;
                background: Rectangle { color: "darkgreen"; radius: 5 }
                contentItem: Text { text: parent.text; font.pixelSize: parent.height * 0.5; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: "black" } }
            Button { text: "Гибернация";
                onClicked: powerManager.hibernate();
                width: root.width * 0.12;
                height: root.height * 0.08;
                background: Rectangle { color: "darkgreen"; radius: 5 }
                contentItem: Text { text: parent.text; font.pixelSize: parent.height * 0.5; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: "black" } }
        }
        Item { Layout.fillHeight: true }

        // --- 4. ЗАМЕНА IMAGE НА CANVAS ---
        Canvas {
            id: statusAnimationCanvas
            Layout.alignment: Qt.AlignHCenter
            // Задаем максимальные размеры, чтобы анимация не "прыгала"
            width: 160
            height: 160

            onPaint: {
                var ctx = getContext("2d");
                ctx.reset();

                var sourceImage = root.onAcPower ? fireSpriteSheet : pilaSpriteSheet;
                if (sourceImage.status !== Image.Ready || root.animationFrameIndex >= root.currentFrameWidths.length) return;

                // Рассчитываем положение и размер нужного кадра в спрайт-листе
                var sx = 0;
                for (var i=0; i < root.animationFrameIndex; ++i) {
                    sx += root.currentFrameWidths[i];
                }
                var sw = root.currentFrameWidths[root.animationFrameIndex];
                var sh = sourceImage.height;

                // Центрируем отрисовку кадра внутри Canvas
                var drawX = (width - sw) / 2;

                // Рисуем кадр
                ctx.drawImage(sourceImage, sx, 0, sw, sh, drawX, 0, sw, sh);
            }
        }

        Button {
            id: backButton
            text: "Назад"
            Layout.alignment: Qt.AlignHCenter
            width: root.width * 0.12
            height: root.height * 0.08
            background: Rectangle { color: "darkgreen"; radius: 5 }
            contentItem: Text { text: parent.text; font.pixelSize: parent.height * 0.5; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; color: "black" }
            onClicked: stackView.pop()
        }
    }

    // --- 5. ОБНОВЛЕННЫЙ ТАЙМЕР И Connections ---
    Timer {
        interval: 120 // Скорость анимации
        running: true
        repeat: true
        onTriggered: {
            root.animationFrameIndex = (root.animationFrameIndex + 1) % root.currentFrameWidths.length;
            statusAnimationCanvas.requestPaint(); // Говорим Canvas перерисоваться
        }
    }

    Connections {
        target: powerManager
        function onPowerSourceTypeChanged() {
            root.animationFrameIndex = 0;
            statusAnimationCanvas.requestPaint(); // Перерисовываем сразу при смене
        }
    }
}
