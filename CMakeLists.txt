cmake_minimum_required(VERSION 3.20)
project(LCD_LABS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Найти Qt (Qt 6.8+)
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Qml)

# Собираемый исполняемый файл — перечисляем все QML/ресурсы здесь, чтобы CMake/Qt их подхватил
qt_add_executable(LCD_LABS
    main.cpp
    qml/Main.qml
    qml/SpriteAvatar.qml
    resources/resources.qrc
)

# Линкуем нужные Qt модули
target_link_libraries(LCD_LABS PRIVATE Qt6::Quick Qt6::Qml Qt6::QuickControls2)

# (Опционально) Если используешь Ninja + MSVC — проверь правильность переменной CMAKE_PREFIX_PATH при конфигурировании
# Пример вызова (из PowerShell или CMD):
# mkdir build
# cmake -S . -B build -G "Ninja" -DCMAKE_PREFIX_PATH="C:\Qt\6.8.3\msvc2019_64\lib\cmake"
# cmake --build build
