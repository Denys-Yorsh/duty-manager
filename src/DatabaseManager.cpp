#include "DatabaseManager.h"
#include <QFile>
#include <QTextStream>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

/**
 * @brief Отримує єдиний екземпляр класу DatabaseManager.
 * @return Посилання на екземпляр DatabaseManager.
 */
DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

/**
 * @brief Конструктор класу DatabaseManager.
 * @param parent Батьківський об'єкт QObject.
 */
DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent) {
}

/**
 * @brief Відкриває підключення до бази даних SQLite.
 * @param dbName Шлях до файлу бази даних.
 * @return true, якщо підключення успішне, інакше false.
 */
bool DatabaseManager::openDatabase(const QString& dbName) {
    // Використовуємо стандартне з'єднання SQLite
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbName);

    if (!m_db.open()) {
        qCritical() << "Помилка: не вдалося встановити з'єднання з базою даних:" << m_db.lastError().text();
        return false;
    }
    return true;
}

/**
 * @brief Закриває підключення до бази даних.
 */
void DatabaseManager::closeDatabase() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

/**
 * @brief Ініціалізує схему бази даних із SQL-файлу.
 * Розбиває скрипт на окремі запити за символом крапки з комою та виконує їх.
 * @param schemaPath Шлях до SQL-файлу схеми.
 * @return true, якщо всі запити виконані успішно, інакше false.
 */
bool DatabaseManager::initSchema(const QString& schemaPath) {
    QFile file(schemaPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Не вдалося відкрити файл схеми:" << schemaPath;
        return false;
    }

    QTextStream in(&file);
    QString sqlScript = in.readAll();
    
    // Більш надійне розбиття скрипта на окремі запити
    // Примітка: для складних скриптів з тригерами або процедурами може знадобитися складніший парсер
    QStringList queries = sqlScript.split(";", Qt::SkipEmptyParts);

    QSqlQuery query;
    for (const QString& q : queries) {
        QString trimmedQuery = q.trimmed();
        if (trimmedQuery.isEmpty()) continue;
        
        if (!query.exec(trimmedQuery)) {
            qCritical() << "Помилка виконання запиту:" << trimmedQuery;
            qCritical() << query.lastError().text();
            return false;
        }
    }
    return true;
}
