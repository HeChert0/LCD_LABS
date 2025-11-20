import QtQuick 6.8

Item {
    id: root

    property url source: (typeof projectDir !== "undefined" && projectDir !== "") ?
                         projectDir + "resources/images/tuda-suda.png" :
                         "qrc:/images/tuda-suda.png"

    property var frameWidths: [60, 55, 50, 45, 40, 40, 55, 60, 60, 60, 55, 60]
    property int frameHeight: 66
    property int frameIndex: 0

    width: 60
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
            for (var i = 0; i < root.frameIndex; ++i) sx += frameWidths[i];
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
        running: true
        onTriggered: {
            root.frameIndex = (root.frameIndex + 1) % root.frameWidths.length;
            canvas.requestPaint();
        }
    }
}
