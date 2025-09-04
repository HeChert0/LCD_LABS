// SpriteAvatar.qml (Новая, стабильная версия)
import QtQuick 6.8

Item {
    id: root

    // --- Свойства для управления извне ---
    property url moveSource: (typeof projectDir !== "undefined" && projectDir !== "") ? projectDir + "resources/images/MM_move.png" : "qrc:/images/MM_move.png"
    property url idleSource: (typeof projectDir !== "undefined" && projectDir !== "") ? projectDir + "resources/images/MM_stat.png" : "qrc:/images/MM_stat.png"
    property bool playing: false
    property bool flipped: false

    // --- Внутренние данные анимации ---
    property var moveFrameWidths: [55,55,55,45,35,45,55,55,55,50,35,50]
    property var idleFrameWidths: [50,50,50,50,50,50,50,50,50,50,50]
    property int frameIndex: 0

    // --- ГЛАВНОЕ ИЗМЕНЕНИЕ: Фиксированный размер компонента ---
    // Мы берем максимальную возможную ширину кадра (55px) и делаем ее постоянной.
    // Это устраняет "дергание" геометрии и конфликты.
    width: 55
    height: 70

    // --- Логика смены состояния ---
    onPlayingChanged: {
        frameIndex = 0; // Сбрасываем кадр при смене анимации (бег/покой)
        canvas.requestPaint();
    }

    // --- Загрузчики спрайт-листов ---
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

            // Определяем параметры текущего кадра
            var sx = 0;
            for (var i=0; i<root.frameIndex; ++i) sx += frameWidths[i];
            var sw = frameWidths[root.frameIndex]; // Ширина кадра в спрайт-листе
            var sh = sourceImage.height;

            // Центрируем отрисовку кадра внутри компонента фиксированного размера
            var drawX = (root.width - sw) / 2;

            ctx.save();
            if (root.flipped) {
                ctx.translate(width, 0);
                ctx.scale(-1, 1);
            }
            // Рисуем кадр (sx, 0, sw, sh) в область (drawX, 0, sw, sh)
            ctx.drawImage(sourceImage, sx, 0, sw, sh, drawX, 0, sw, sh);
            ctx.restore();
        }
    }

    // --- Таймеры для анимации ---
    Timer {
        interval: 100 // Оптимальная скорость для анимации бега
        repeat: true
        running: root.playing
        onTriggered: {
            root.frameIndex = (root.frameIndex + 1) % root.moveFrameWidths.length;
            canvas.requestPaint();
        }
    }

    Timer {
        interval: 90 // Анимация покоя
        repeat: true
        running: !root.playing
        onTriggered: {
            root.frameIndex = (root.frameIndex + 1) % root.idleFrameWidths.length;
            canvas.requestPaint();
        }
    }

    // --- Публичные методы для управления ---
    function startAnimation() {
        root.playing = true;
    }

    function stopAnimationToStatic() {
        root.playing = false;
    }
}
