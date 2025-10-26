import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    property url spriteSource: (typeof projectDir !== "undefined" && projectDir !== "") ?
                               projectDir + "resources/images/camera_warning.png" :
                               "qrc:/images/camera_warning.png"
    property var frameWidths: [104, 110, 104, 65, 57, 84, 88, 68, 90, 140, 118, 128, 94, 78]
    property int frameHeight: 188
    property int frameIndex: 0
    property bool playing: true

    width: 140
    height: frameHeight

    Image {
        id: spriteSheet
        source: root.spriteSource
        visible: false
        cache: true
    }

    Canvas {
        id: canvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();

            if (spriteSheet.status !== Image.Ready || frameIndex >= frameWidths.length) return;

            var sx = 0;
            for (var i = 0; i < root.frameIndex; ++i) {
                sx += frameWidths[i];
            }
            var sw = frameWidths[root.frameIndex];
            var sh = root.frameHeight;

            var drawX = (root.width - sw) / 2;
            var drawY = (root.height - sh) / 2;

            ctx.drawImage(spriteSheet, sx, 0, sw, sh, drawX, drawY, sw, sh);
        }
    }

    Timer {
        interval: 100
        repeat: true
        running: root.playing && root.visible
        onTriggered: {
            root.frameIndex = (root.frameIndex + 1) % root.frameWidths.length;
            canvas.requestPaint();
        }
    }

    SequentialAnimation on opacity {
        running: root.visible
        loops: 1
        NumberAnimation { from: 0; to: 1; duration: 300 }
        PauseAnimation { duration: 2000 }
        NumberAnimation { from: 1; to: 0; duration: 300 }
    }

    SequentialAnimation on rotation {
        running: root.visible
        loops: Animation.Infinite
        NumberAnimation { to: -5; duration: 200 }
        NumberAnimation { to: 5; duration: 200 }
    }

    Rectangle {
        anchors.bottom: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 10
        width: warningText.width + 20
        height: 30
        color: "#FF0000"
        radius: 15
        opacity: 0.9

        Label {
            id: warningText
            anchors.centerIn: parent
            text: "⚠️ Camera Active!"
            color: "white"
            font.bold: true
            font.pixelSize: 12
        }

        SequentialAnimation on scale {
            running: root.visible
            loops: Animation.Infinite
            NumberAnimation { to: 1.1; duration: 500 }
            NumberAnimation { to: 1.0; duration: 500 }
        }
    }
}
