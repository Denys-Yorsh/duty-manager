#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

/**
 * @class DatabaseManager
 * @brief Клас для управління підключенням до бази даних SQLite та ініціалізації схеми.
 * Реалізований як Singleton для глобального доступу до БД.
 */
class DatabaseManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Отримує єдиний екземпляр класу DatabaseManager.
     * @return Посилання на екземпляр DatabaseManager.
     */
    static DatabaseManager& instance();
    
    /**
     * @brief Відкриває підключення до бази даних.
     * @param dbName Шлях до файлу бази даних.
     * @return true, якщо підключення успішне, інакше false.
     */
    Q_INVOKABLE bool openDatabase(const QString& dbName);

    /**
     * @brief Закриває підключення до бази даних.
     */
    Q_INVOKABLE void closeDatabase();

    /**
     * @brief Ініціалізує схему бази даних із SQL-файлу.
     * @param schemaPath Шлях до SQL-файлу схеми.
     * @return true, якщо ініціалізація успішна, інакше false.
     */
    Q_INVOKABLE bool initSchema(const QString& schemaPath);

private:
    /**
     * @brief Приватний конструктор для реалізації Singleton.
     */
    DatabaseManager(QObject *parent = nullptr);

    /**
     * @brief Деструктор за замовчуванням.
     */
    ~DatabaseManager() = default;

    // Заборона копіювання та присвоєння
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase m_db; ///< Об'єкт підключення до бази даних
};

#endif // DATABASEMANAGER_H
