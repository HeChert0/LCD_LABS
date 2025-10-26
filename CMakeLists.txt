cmake_minimum_required(VERSION 3.20)
project(LCD_LABS LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Qml Network Multimedia)

qt_add_resources(LCD_LABS_RESOURCES
    resources/resources.qrc
)

qt_add_executable(LCD_LABS
    main.cpp
    qml/MainMenuPage.qml
    qml/SpriteAvatar.qml
    qml/Main.qml
    qml/labs/lab1/Lab1Page.qml
    labs/lab1/PowerManager.cpp
    labs/lab1/PowerManager.h
    qml/labs/lab2/Lab2Page.qml
    labs/lab2/PciManager.cpp
    labs/lab2/PciManager.h
    qml/labs/lab3/Lab3Page.qml
    labs/lab3/HddManager.cpp
    labs/lab3/HddManager.h
    qml/labs/lab4/Lab4Page.qml
    qml/labs/lab4/CameraWarningSprite.qml
    labs/lab4/CameraManager.cpp
    labs/lab4/CameraManager.h
    ${LCD_LABS_RESOURCES}


)

target_link_libraries(LCD_LABS PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Multimedia
    Powrprof # для SetSuspendState
    Setupapi #SetupDi
)
