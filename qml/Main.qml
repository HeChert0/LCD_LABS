import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: win
    visible: true
    title: "LCD_LABS"
    width: 1024
    height: 576
    color: "transparent"

    Component.onCompleted: {
            var component = Qt.createComponent("qrc:/qml/labs/lab2/Lab2Page.qml")
            if (component.status === Component.Ready) {
                console.log("Lab2Page preloaded")
            }
            console.log("Main window loaded");
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
