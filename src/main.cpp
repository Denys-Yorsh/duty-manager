#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QDebug>
#include "DatabaseManager.h"
#include "PersonnelController.h"
#include "DutyTypesController.h"
#include "ScheduleController.h"
#include "StatisticsController.h"

using namespace Qt::StringLiterals;

/**
 * @brief Точка входу в програму.
 * Виконує ініціалізацію бази даних, створення основних контролерів
 * та запуск QML-двигуна.
 */
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Встановлюємо метадані програми
    app.setApplicationName("Графік нарядів ВЧ");
    app.setOrganizationName("Військова Частина");

    // Встановлюємо стиль Fusion для уніфікованого вигляду на всіх платформах
    QQuickStyle::setStyle("Fusion");

    // Визначаємо шлях до БД та схеми
    QString appDir = QApplication::applicationDirPath();
    QString dbPath = appDir + "/duty_manager.db";
    QString schemaPath = ":/db/schema.sql";

    // Відкриваємо та ініціалізуємо базу даних
    if (!DatabaseManager::instance().openDatabase(dbPath)) {
        qCritical() << "Не вдалося відкрити базу даних:" << dbPath;
        return 1;
    }

    if (!DatabaseManager::instance().initSchema(schemaPath)) {
        qCritical() << "Не вдалося ініціалізувати схему БД.";
        return 1;
    }

    QQmlApplicationEngine engine;

    // Створюємо контролери бізнес-логіки
    PersonnelController personnelController;
    DutyTypesController dutyTypesController;
    ScheduleController scheduleController;
    StatisticsController statisticsController;

    // Реєструємо контролери в контексті QML
    engine.rootContext()->setContextProperty("DatabaseManager", &DatabaseManager::instance());
    engine.rootContext()->setContextProperty("PersonnelController", &personnelController);
    engine.rootContext()->setContextProperty("DutyTypesController", &dutyTypesController);
    engine.rootContext()->setContextProperty("ScheduleController", &scheduleController);
    engine.rootContext()->setContextProperty("StatisticsController", &statisticsController);

    // Глобальне оновлення статусів особового складу на старті
    personnelController.updateAllPersonnelStatuses();

    // Завантажуємо основний QML-інтерфейс
    const QUrl url(u"qrc:/qml/main.qml"_s);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);

    engine.load(url);

    // Запуск циклу обробки подій
    int result = app.exec();
    
    // Закриття бази даних перед виходом
    DatabaseManager::instance().closeDatabase();
    
    return result;
}
