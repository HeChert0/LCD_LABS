// Main.qml
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: win
    visible: true
    title: "LCD_LABS"
    width: 1024
    height: 576
    color: "black"

    Component.onCompleted: {
        console.log("Main window loaded")
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: "MainMenuPage.qml"
        onCurrentItemChanged: {
            console.log("StackView current item changed:", currentItem)
        }
    }
}
