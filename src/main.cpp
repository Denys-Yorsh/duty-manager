#include <QApplication>
#include "MainWindow.h"
#include "DatabaseManager.h"
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    
    // Встановлюємо стиль Fusion для чистої та передбачуваної отрисовки (прибирає системні артефакти Windows)
    a.setStyle(QStyleFactory::create("Fusion"));

    // Встановлюємо шлях до БД поруч із файлом .exe
    QString appDir = QCoreApplication::applicationDirPath();
    QString dbPath = appDir + "/duty_manager.db";
    QString schemaPath = ":/db/schema.sql";

    if (!DatabaseManager::instance().openDatabase(dbPath)) {
        QMessageBox::critical(nullptr, "Помилка БД", "Не вдалося відкрити базу даних по шляху:\n" + dbPath);
        return 1;
    }

    if (!DatabaseManager::instance().initSchema(schemaPath)) {
        QMessageBox::critical(nullptr, "Помилка БД", "Не вдалося ініціалізувати структуру таблиць.");
        return 1;
    }

    MainWindow w;

    // Глобальний стиль: помаранчева рамка для виділення без системних полосок
    a.setStyleSheet(
        "QTableView, QTableWidget { "
        "   selection-background-color: transparent; "
        "   selection-color: black; "
        "   outline: none; "
        "} "
        "QTableView::item, QTableWidget::item { "
        "   outline: none; "
        "   padding: 2px; "
        "} "
        "/* Рамка навколо виділеної строки (для списків) */ "
        "QTableView::item:selected, QTableWidget::item:selected { "
        "   border-top: 1px solid #F39200; "
        "   border-bottom: 1px solid #F39200; "
        "} "
        "QTableView::item:selected:first, QTableWidget::item:selected:first { "
        "   border-left: 1px solid #F39200; "
        "} "
        "QTableView::item:selected:last, QTableWidget::item:selected:last { "
        "   border-right: 1px solid #F39200; "
        "} "
        "/* Повна рамка для окремих ячейок (для графіка) */ "
        "#scheduleTable::item:selected { "
        "   border: 1px solid #F39200; "
        "} "
        "QHeaderView::section { "
        "   border: 1px solid #dcdcdc; "
        "   padding: 4px; "
        "   text-align: center; "
        "}"
    );

    w.show();

    int result = a.exec();
    DatabaseManager::instance().closeDatabase();
    return result;
}
