cmake_minimum_required(VERSION 3.16)

project(DutyManager VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

add_executable(DutyManager
    src/main.cpp
    src/MainWindow.cpp
    src/DatabaseManager.cpp
    src/PersonnelWidget.cpp
    src/DutyTypesWidget.cpp
)

target_include_directories(DutyManager PRIVATE include)

target_link_libraries(DutyManager PRIVATE Qt6::Widgets Qt6::Sql)

# Копіювання схеми БД в директорію збірки для зручності
configure_file(db/schema.sql ${CMAKE_BINARY_DIR}/db/schema.sql COPYONLY)
