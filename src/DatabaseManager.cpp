#include "DatabaseManager.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::openDatabase(const QString& dbName) {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbName);

    if (!m_db.open()) {
        qCritical() << "Error: connection with database failed:" << m_db.lastError().text();
        return false;
    }
    return true;
}

void DatabaseManager::closeDatabase() {
    m_db.close();
}

bool DatabaseManager::initSchema(const QString& schemaPath) {
    QFile file(schemaPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Could not open schema file:" << schemaPath;
        return false;
    }

    QTextStream in(&file);
    QString sqlScript = in.readAll();
    QStringList queries = sqlScript.split(";", Qt::SkipEmptyParts);

    QSqlQuery query;
    for (const QString& q : queries) {
        if (q.trimmed().isEmpty()) continue;
        if (!query.exec(q)) {
            qCritical() << "Error executing query:" << q.trimmed();
            qCritical() << query.lastError().text();
            return false;
        }
    }
    return true;
}
