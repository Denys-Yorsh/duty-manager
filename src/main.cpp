#include <QApplication>
#include "MainWindow.h"
#include "DatabaseManager.h"
#include <QDir>
#include <QMessageBox>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // Ініціалізація бази даних
    QString dbPath = "duty_manager.db";
    QString schemaPath = "db/schema.sql";

    if (!DatabaseManager::instance().openDatabase(dbPath)) {
        QMessageBox::critical(nullptr, "Помилка БД", "Не вдалося відкрити базу даних.");
        return 1;
    }

    if (!DatabaseManager::instance().initSchema(schemaPath)) {
        QMessageBox::critical(nullptr, "Помилка БД", "Не вдалося ініціалізувати структуру таблиць.");
        return 1;
    }

    MainWindow w;
    w.show();

    int result = a.exec();

    DatabaseManager::instance().closeDatabase();
    
    return result;
}
