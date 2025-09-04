// Main.qml (Исправлено: порядок отрисовки Z-order)
import QtQuick 6.8
import QtQuick.Controls 6.8

ApplicationWindow {
    id: win
    visible: true
    title: "LCD_LABS"
    width: 1024
    height: 576
    color: "black"

    property int hoveredButtonIndex: -1

    Image {
        id: bg
        anchors.fill: parent
        source: (typeof projectDir !== "undefined" && projectDir !== "") ? projectDir + "resources/images/MM_bg.png" : "qrc:/images/MM_bg.png"
        fillMode: Image.PreserveAspectCrop
    }

    // --- ПЕРЕМЕСТИЛИ ОБЪЯВЛЕНИЕ GRID ПЕРЕД AVATAR ---
    Grid {
        id: buttonsGrid
        columns: 3
        spacing: win.width * 0.03
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: win.height * 0.04

        Repeater {
            model: 6
            delegate: Button {
                id: btn
                text: "Lab " + (index + 1)
                width: win.width * 0.12
                height: win.height * 0.08

                background: Rectangle {
                    color: "darkgreen"
                    radius: 5
                }
                contentItem: Text {
                    text: btn.text
                    font.pixelSize: btn.height * 0.5
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "black"
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        win.hoveredButtonIndex = index
                    }
                }
            }
        }
    }

    // --- AVATAR ТЕПЕРЬ ОБЪЯВЛЕН ПОСЛЕ GRID, ЧТОБЫ РИСОВАТЬСЯ ПОВЕРХ ---
    SpriteAvatar {
        id: avatar
        // Положение управляется императивно
    }

    ParallelAnimation {
        id: moveAnim
        NumberAnimation { id: moveX; target: avatar; property: "x"; duration: 400; easing.type: Easing.InOutQuad }
        NumberAnimation { id: moveY; target: avatar; property: "y"; duration: 400; easing.type: Easing.InOutQuad }

        onFinished: {
            avatar.stopAnimationToStatic()
        }
    }

    onHoveredButtonIndexChanged: {
        if (hoveredButtonIndex < 0 || hoveredButtonIndex >= buttonsGrid.children.length) {
            return;
        }

        var targetButton = buttonsGrid.children[hoveredButtonIndex];
        if (!targetButton) return;

        var buttonPos = targetButton.mapToItem(win.contentItem, 0, 0);

        // --- Возвращаем старый расчет Y, так как Z-order теперь решает проблему ---
        // Отступ от кнопки (было -8, возвращаем)
        var verticalOffset = 8;

        var targetX = buttonPos.x + (targetButton.width - avatar.width) / 2;
        var targetY = buttonPos.y - avatar.height - verticalOffset;

        var avatarCenterX = avatar.x + avatar.width / 2;
        var targetCenterX = targetX + avatar.width / 2;
        avatar.flipped = (targetCenterX < avatarCenterX);

        var dx = Math.abs(targetX - avatar.x);
        var dy = Math.abs(targetY - avatar.y);
        var dist = Math.sqrt(dx*dx + dy*dy);
        var dur = Math.max(400, Math.min(1500, Math.round(dist * 4)));

        moveAnim.stop();
        moveX.duration = dur;
        moveY.duration = dur;
        moveX.to = targetX;
        moveY.to = targetY;

        avatar.startAnimation();
        moveAnim.start();
    }

    Component.onCompleted: {
        avatar.x = (win.width - avatar.width) / 2;
        avatar.y = (win.height - avatar.height) / 2;
        avatar.stopAnimationToStatic();
    }

    onWidthChanged: {
        avatar.x = (win.width - avatar.width) / 2;
    }
    onHeightChanged: {
        avatar.y = (win.height - avatar.height) / 2;
    }
}
