import QtQuick
import QtQuick.Controls

Item {
    id: root
    property StackView stackView: parent
    property int hoveredButtonIndex: -1
    property bool initialized: false

    Image {
        id: bg
        anchors.fill: parent
        source: (typeof projectDir !== "undefined" && projectDir !== "") ? projectDir + "resources/images/MM_bg.png" : "qrc:/images/MM_bg.png"
        fillMode: Image.PreserveAspectCrop
        z: -1
    }

    Grid {
            id: buttonsGrid
            columns: 3
            spacing: root.width * 0.03
            rowSpacing: root.height * 0.03

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: root.height * 0.04
            z: 1

            Repeater {
                model: 6
                delegate: menuButton
            }
        }

        Component {
            id: menuButton
            Button {
                id: btn

                text: "Lab " + (index + 1)
                width: root.width * 0.12
                height: root.height * 0.08
                z: 2

                onClicked: {
                    switch(index) {
                        case 0: stackView.push("labs/lab1/Lab1Page.qml"); break;
                        case 1: stackView.push("labs/lab2/Lab2Page.qml"); break;
                        case 2: stackView.push("labs/lab3/Lab3Page.qml"); break;
                        case 3: stackView.push("labs/lab4/Lab4Page.qml"); break;
                        case 4: stackView.push("labs/lab5/Lab5Page.qml"); break;
                        case 5: stackView.push("labs/lab6/Lab6Page.qml"); break;
                    }
                }

                background: Rectangle {
                    color: btn.down ? "green" : (btn.hovered ? "lightgreen" : "darkgreen")
                    radius: 5
                    border.width: 2
                    border.color: "white"
                }

                contentItem: Text {
                    text: btn.text
                    font.pixelSize: btn.height * 0.5
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "black"
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        root.hoveredButtonIndex = index
                    }
                    onClicked: {
                        btn.clicked()
                    }
                }
            }
        }


    SpriteAvatar {
        id: avatar
        z: 3
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
        if (hoveredButtonIndex < 0 || hoveredButtonIndex >= 6) {
            return;
        }

        var targetButton = buttonsGrid.children[hoveredButtonIndex];

        if (!targetButton) return;

        var buttonPos = targetButton.mapToItem(stackView, 0, 0);
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

    Timer {
        id: initTimer
        interval: 100
        running: true
        onTriggered: {
            initialized = true;
            avatar.x = (root.width - avatar.width) / 2;
            avatar.y = (root.height - avatar.height) / 2;
            avatar.stopAnimationToStatic();
            console.log("MainMenuPage initialized")
            console.log("Root size:", root.width, root.height)
        }
    }

    onWidthChanged: {
        if (initialized) {
            avatar.x = (root.width - avatar.width) / 2;
        }
    }

    onHeightChanged: {
        if (initialized) {
            avatar.y = (root.height - avatar.height) / 2;
        }
    }

    Component.onCompleted: {
        avatar.x = (root.width - avatar.width) / 2;
        avatar.y = (root.height - avatar.height) / 2;
        avatar.stopAnimationToStatic();

        console.log("MainMenuPage loaded")
        console.log("Root size:", root.width, root.height)
    }
}
