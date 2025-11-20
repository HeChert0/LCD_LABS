import QtQuick 6.8

Item {
    id: root

    property url source: (typeof projectDir !== "undefined" && projectDir !== "") ?
                         projectDir + "resources/images/mayatnik.png" :
                         "qrc:/images/mayatnik.png"

    property var frameWidths: [145, 145, 155, 165, 150, 115, 90, 105, 140, 154]
    property int frameHeight: 172
    property int frameIndex: 0

    width: 165
    height: frameHeight

    Image {
        id: spriteSheet
        source: root.source
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
        interval: 120
        repeat: true
        running: true
        onTriggered: {
            root.frameIndex = (root.frameIndex + 1) % root.frameWidths.length;
            canvas.requestPaint();
        }
    }
}
