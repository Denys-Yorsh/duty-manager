# ==============================================================================
# МІНІМАЛЬНІ ВИМОГИ ТА НАЛАШТУВАННЯ ПРОЄКТУ
# ==============================================================================

# Вказуємо мінімально необхідну версію CMake для збірки проєкту.
cmake_minimum_required(VERSION 3.16)

# Приглушуємо попередження про використання приватних модулів Qt (необхідно для QXlsx)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

# Оголошуємо назву проєкту, його версію та мову програмування (C++).
project(DutyManager VERSION 2.0.0 LANGUAGES CXX)

# Задаємо стандарт C++20 для всього проєкту.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================================================================
# АВТОМАТИЧНА ОБРОБКА ІНСТРУМЕНТІВ QT (MOC, RCC, UIC)
# ==============================================================================

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ==============================================================================
# ПОШУК ТА ПІДКТЮЧЕННЯ ЗОВНІШНІХ БІБЛІОТЕК
# ==============================================================================

find_package(Qt6 REQUIRED COMPONENTS 
    Widgets          # Класичний графічний інтерфейс (GUI)
    Sql              # Робота з базами даних (SQLite)
    PrintSupport     # Можливості друку та експорту в PDF
    Gui              # Базові компоненти рендерингу графіки та вікон
    Quick            # Двигун для роботи з QML інтерфейсом
    Qml              # Мова QML та її середовище виконання
    QuickControls2   # Готові стилізовані елементи керування для QML
    Charts           # Побудова діаграм та графіків
)

# Підключаємо вихідні файли сторонньої бібліотеки QXlsx для роботи з Excel (.xlsx).
# ВАЖЛИВО: Використовуємо EXCLUDE_FROM_ALL, щоб CMake не виконував внутрішні правила
# install() самій бібліотеці QXlsx, які викликали помилку пошуку заголовочних файлів при CPack.
add_subdirectory(3rdparty/QXlsx/QXlsx EXCLUDE_FROM_ALL)

# ==============================================================================
# СПИСКИ ВИХІДНИХ ФАЙЛІВ ТА ЗАГОЛОВКІВ ПРОЄКТУ
# ==============================================================================

set(SOURCES
    src/main.cpp                  # Точка входу в програму
    src/MainWindow.cpp            # Головне вікно програми
    src/DatabaseManager.cpp       # Управління підключенням і запитами до SQLite
    src/PersonnelController.cpp   # Логіка управління особистим складом / черговими
    src/DutyTypesController.cpp  # Логіка управління типами нарядів / чергувань
    src/ScheduleController.cpp   # Логіка формування та керування розкладом
    src/AutoGenerator.cpp         # Алгоритми автоматичного генератора нарядів
    src/StatisticsController.cpp  # Обрахунок та підготовка статистики
)

set(HEADERS
    include/MainWindow.h
    include/DatabaseManager.h
    include/PersonnelController.h
    include/DutyTypesController.h
    include/ScheduleController.h
    include/AutoGenerator.h
    include/StatisticsController.h
)

# Перевіряємо, чи відбувається збірка під платформу Windows.
if(WIN32)
    # Додаємо файл ресурсів Windows (.rc), який містить іконку .exe та метадані файлу.
    set(RES_FILES app.rc)
endif()

# ==============================================================================
# СТВОРЕННЯ ВИКОНУВАНОГО ФАЙЛУ (EXECUTABLE)
# ==============================================================================

add_executable(DutyManager WIN32 
    ${SOURCES}      # Включаємо вихідний C++ код
    ${HEADERS}      # Включаємо заголовочні файли
    resources.qrc   # Включаємо файл ресурсів Qt (іконки, QML-файли, стилі)
    ${RES_FILES}    # Включаємо Windows-ресурси (якщо платформа WIN32)
)

# Вказуємо шлях до заголовочних файлів (.h).
target_include_directories(DutyManager PRIVATE
    include                     # Власні заголовочні файли проєкту
    3rdparty/QXlsx/QXlsx/header # Заголовочні файли бібліотеки QXlsx
)

# Лінкуємо модулі Qt6 та бібліотеку QXlsx.
target_link_libraries(DutyManager PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::PrintSupport
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::Charts
    QXlsx
)

# ==============================================================================
# КОПІЮВАННЯ ЛОКАЛЬНИХ РЕСУРСІВ У ДИРЕКТОРІЮ ЗБІРКИ (ДЛЯ РОЗРОБКИ ТА НАЛАГОДЖЕННЯ)
# ==============================================================================

configure_file(db/schema.sql ${CMAKE_BINARY_DIR}/db/schema.sql COPYONLY)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})

# ==============================================================================
# НАЛАШТУВАННЯ УСТАНОВКИ ТА СТВОРЕННЯ ІНСТАЛЯТОРА EXE (CPACK / NSIS)
# ==============================================================================

if(WIN32)
    include(GNUInstallDirs)

    # --------------------------------------------------------------------------
    # 1. Основні правила інсталяції бінарників та ресурсів проєкту
    # --------------------------------------------------------------------------

    # Встановлюємо сам виконуваний файл DutyManager.exe у папку "bin".
    install(TARGETS DutyManager DESTINATION bin)

    # Примітка: Явний install(TARGETS QXlsx) видалено, оскільки QXlsx лінкується 
    # статично або збирається як частина executable, і його target не повинен експортуватись.

    # Копіюємо базу даних та асети в папку встановленого додатка.
    install(FILES db/schema.sql DESTINATION bin/db)
    install(DIRECTORY assets/ DESTINATION bin/assets)

    # --------------------------------------------------------------------------
    # 2. Пряме пакування системних DLL та плагінів Qt6 для крос-компіляції (Fedora MinGW)
    # --------------------------------------------------------------------------

    # Базовий системний шлях MinGW у Fedora Linux
    set(MINGW_SYSROOT "/usr/x86_64-w64-mingw32/sys-root/mingw")

    # Перелік усіх динамічних бібліотек Qt6, необхідних для роботи додатка
    file(GLOB QT6_CORE_DLLS
        "${MINGW_SYSROOT}/bin/Qt6Core.dll"
        "${MINGW_SYSROOT}/bin/Qt6Gui.dll"
        "${MINGW_SYSROOT}/bin/Qt6Widgets.dll"
        "${MINGW_SYSROOT}/bin/Qt6Sql.dll"
        "${MINGW_SYSROOT}/bin/Qt6PrintSupport.dll"
        "${MINGW_SYSROOT}/bin/Qt6Quick.dll"
        "${MINGW_SYSROOT}/bin/Qt6Qml.dll"
        "${MINGW_SYSROOT}/bin/Qt6QmlModels.dll"
        "${MINGW_SYSROOT}/bin/Qt6QuickControls2.dll"
        "${MINGW_SYSROOT}/bin/Qt6QuickLayouts.dll"
        "${MINGW_SYSROOT}/bin/Qt6Charts.dll"
        "${MINGW_SYSROOT}/bin/Qt6OpenGL.dll"
        "${MINGW_SYSROOT}/bin/Qt6Network.dll"
    )

    # Перелік системних бібліотек runtime-компілятора GCC / MinGW
    file(GLOB MINGW_RUNTIME_DLLS
        "${MINGW_SYSROOT}/bin/libgcc_s_seh-1.dll"
        "${MINGW_SYSROOT}/bin/libstdc++-6.dll"
        "${MINGW_SYSROOT}/bin/libwinpthread-1.dll"
    )

    # Копіюємо знайдені `.dll` безпосередньо у папку `bin` інсталятора
    install(FILES ${QT6_CORE_DLLS} ${MINGW_RUNTIME_DLLS} DESTINATION bin OPTIONAL)

    # Встановлюємо критично важливі плагіни Qt6 (Платформа, База даних SQLite, Стилі)
    install(DIRECTORY "${MINGW_SYSROOT}/lib/qt6/plugins/platforms" DESTINATION bin FILES_MATCHING PATTERN "*.dll")
    install(DIRECTORY "${MINGW_SYSROOT}/lib/qt6/plugins/sqldrivers" DESTINATION bin FILES_MATCHING PATTERN "*.dll")
    install(DIRECTORY "${MINGW_SYSROOT}/lib/qt6/plugins/styles" DESTINATION bin OPTIONAL FILES_MATCHING PATTERN "*.dll")
    install(DIRECTORY "${MINGW_SYSROOT}/lib/qt6/plugins/imageformats" DESTINATION bin OPTIONAL FILES_MATCHING PATTERN "*.dll")

    # Копіюємо QML-модулі та стилі QtQuickControls2, якщо вони використовуються додатоком
    if(EXISTS "${MINGW_SYSROOT}/lib/qt6/qml")
        install(DIRECTORY "${MINGW_SYSROOT}/lib/qt6/qml" DESTINATION bin)
    endif()

    # --------------------------------------------------------------------------
    # 3. Метадані та параметри інсталятора (CPack NSIS)
    # --------------------------------------------------------------------------

    set(CPACK_PACKAGE_NAME "DutyManager")
    set(CPACK_PACKAGE_VENDOR "Denys Yorsh")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Duty Manager Application")

    # Вказуємо генератор NSIS замість WIX для збірки .exe інсталятора.
    set(CPACK_GENERATOR "NSIS")

    # Налаштування відображення в майстрах установки Windows.
    set(CPACK_NSIS_DISPLAY_NAME "Duty Manager")
    set(CPACK_NSIS_PACKAGE_NAME "Duty Manager Setup")
    set(CPACK_NSIS_CONTACT "Denys Yorsh")
    set(CPACK_NSIS_MODIFY_PATH ON)

    # Автоматичне створення ярлика на Робочому столі при установці.
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$DESKTOP\\\\Duty Manager.lnk' '$INSTDIR\\\\bin\\\\DutyManager.exe'"
    )

    # Автоматичне видалення ярлика з Робочому столу при деінсталяції.
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$DESKTOP\\\\Duty Manager.lnk'"
    )

    # --------------------------------------------------------------------------
    # 4. Включення модуля CPack (має йти суворо ПІСЛЯ оголошення змінних CPACK_*)
    # --------------------------------------------------------------------------
    include(CPack)
endif()
