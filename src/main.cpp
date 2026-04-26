#include <QApplication>
#include "MainWindow.h"
#include "DatabaseManager.h"
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // Встановлюємо шлях до БД поруч із файлом .exe (або в папці документів, якщо немає прав запису)
    QString appDir = QCoreApplication::applicationDirPath();
    QString dbPath = appDir + "/duty_manager.db";
    
    // Якщо шлях до ресурсів починається з ":", Qt шукає файл всередині скомпільованого .exe
    QString schemaPath = ":/db/schema.sql";

    if (!DatabaseManager::instance().openDatabase(dbPath)) {
        QMessageBox::critical(nullptr, "Помилка БД", "Не вдалося відкрити базу даних по шляху:\n" + dbPath);
        return 1;
    }

    if (!DatabaseManager::instance().initSchema(schemaPath)) {
        QMessageBox::critical(nullptr, "Помилка БД", "Не вдалося ініціалізувати структуру таблиць.\nПеревірте наявність вбудованої схеми.");
        return 1;
    }

    MainWindow w;
    w.show();

    int result = a.exec();

    DatabaseManager::instance().closeDatabase();
    
    return result;
}
