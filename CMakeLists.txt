cmake_minimum_required(VERSION 3.16)

project(DutyManager VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql PrintSupport)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/DatabaseManager.cpp
    src/PersonnelWidget.cpp
    src/DutyTypesWidget.cpp
    src/StatusDialog.cpp
    src/ScheduleWidget.cpp
    src/AutoGenerator.cpp
)

set(HEADERS
    include/MainWindow.h
    include/DatabaseManager.h
    include/PersonnelWidget.h
    include/DutyTypesWidget.h
    include/StatusDialog.h
    include/ScheduleWidget.h
    include/AutoGenerator.h
)

# Додаємо app.rc до списку файлів для Windows
if(WIN32)
    set(RES_FILES app.rc)
endif()

add_executable(DutyManager WIN32 ${SOURCES} ${HEADERS} resources.qrc ${RES_FILES})

target_include_directories(DutyManager PRIVATE include)

target_link_libraries(DutyManager PRIVATE 
    Qt6::Widgets 
    Qt6::Sql 
    Qt6::PrintSupport
)

configure_file(db/schema.sql ${CMAKE_BINARY_DIR}/db/schema.sql COPYONLY)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
