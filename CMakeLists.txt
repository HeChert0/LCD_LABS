cmake_minimum_required(VERSION 3.20)
project(LCD_LABS LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Найти Qt (Qt 6.8+)
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Qml)

qt_add_resources(LCD_LABS_RESOURCES
    resources/resources.qrc
)

# Собираемый исполняемый файл — перечисляем все QML/ресурсы здесь, чтобы CMake/Qt их подхватил
qt_add_executable(LCD_LABS
    main.cpp
    qml/MainMenuPage.qml
    qml/SpriteAvatar.qml
    qml/Main.qml
    qml/labs/lab1/Lab1Page.qml
    labs/lab1/PowerManager.cpp
    labs/lab1/PowerManager.h
    ${LCD_LABS_RESOURCES}


)

target_link_libraries(LCD_LABS PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Powrprof # для SetSuspendState
    Setupapi #SetupDi
)
