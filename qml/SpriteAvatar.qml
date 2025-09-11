import QtQuick 6.8

Item {
    id: root

    property url moveSource: (typeof projectDir !== "undefined" && projectDir !== "") ? projectDir + "resources/images/MM_move.png" : "qrc:/images/MM_move.png"
    property url idleSource: (typeof projectDir !== "undefined" && projectDir !== "") ? projectDir + "resources/images/MM_stat.png" : "qrc:/images/MM_stat.png"
    property bool playing: false
    property bool flipped: false

    property var moveFrameWidths: [55,55,55,45,35,45,55,55,55,50,35,50]
    property var idleFrameWidths: [50,50,50,50,50,50,50,50,50,50,50]
    property int frameIndex: 0

    width: 55
    height: 70

    onPlayingChanged: {
        frameIndex = 0;
        canvas.requestPaint();
    }

    Image { id: moveSheet; source: root.moveSource; visible: false; cache: true }
    Image { id: idleSheet; source: root.idleSource; visible: false; cache: true }

    Canvas {
        id: canvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();

            var sourceImage = root.playing ? moveSheet : idleSheet;
            var frameWidths = root.playing ? root.moveFrameWidths : root.idleFrameWidths;
            if (sourceImage.status !== Image.Ready || frameIndex >= frameWidths.length) return;

            var sx = 0;
            for (var i=0; i<root.frameIndex; ++i) sx += frameWidths[i];
            var sw = frameWidths[root.frameIndex];
            var sh = sourceImage.height;

            var drawX = (root.width - sw) / 2;

            ctx.save();
            if (root.flipped) {
                ctx.translate(width, 0);
                ctx.scale(-1, 1);
            }
            ctx.drawImage(sourceImage, sx, 0, sw, sh, drawX, 0, sw, sh);
            ctx.restore();
        }
    }

    Timer {
        interval: 100
        repeat: true
        running: root.playing
        onTriggered: {
            root.frameIndex = (root.frameIndex + 1) % root.moveFrameWidths.length;
            canvas.requestPaint();
        }
    }

    Timer {
        interval: 90
        repeat: true
        running: !root.playing
        onTriggered: {
            root.frameIndex = (root.frameIndex + 1) % root.idleFrameWidths.length;
            canvas.requestPaint();
        }
    }

    function startAnimation() {
        root.playing = true;
    }

    function stopAnimationToStatic() {
        root.playing = false;
    }
}
