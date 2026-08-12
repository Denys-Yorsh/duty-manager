#ifndef PERSONNELCONTROLLER_H
#define PERSONNELCONTROLLER_H

#include <QObject>
#include <QVariantList>

/**
 * @class PersonnelController
 * @brief Контролер для управління даними особового складу та їх статусами.
 * Забезпечує взаємодію між UI та базою даних для таблиці personnel.
 */
class PersonnelController : public QObject {
    Q_OBJECT
    /**
     * @property selectedPersonId
     * @brief ID вибраного військовослужбовця в інтерфейсі.
     */
    Q_PROPERTY(int selectedPersonId READ selectedPersonId WRITE setSelectedPersonId NOTIFY selectedPersonIdChanged)
    
    /**
     * @property currentSelectedPersonName
     * @brief ПІБ вибраного військовослужбовця.
     */
    Q_PROPERTY(QString currentSelectedPersonName READ currentSelectedPersonName NOTIFY currentSelectedPersonNameChanged)

public:
    /**
     * @brief Конструктор контролера.
     * @param parent Батьківський об'єкт.
     */
    explicit PersonnelController(QObject *parent = nullptr);

    /**
     * @brief Отримує повний список особового складу.
     * @return Список QVariantMap з даними військовослужбовців.
     */
    Q_INVOKABLE QVariantList getPersonnelList();

    /**
     * @brief Додає нового військовослужбовця.
     * @param rank Назва звання.
     * @param name ПІБ.
     * @param position Посада.
     * @param notes Примітки.
     * @return true, якщо успішно.
     */
    Q_INVOKABLE bool addPerson(const QString &rank, const QString &name, const QString &position, const QString &notes);

    /**
     * @brief Видаляє військовослужбовця за ID.
     * @param id ID запису.
     * @return true, якщо успішно.
     */
    Q_INVOKABLE bool deletePerson(int id);

    /**
     * @brief Оновлює дані військовослужбовця.
     * @param id ID запису.
     * @param rank Назва звання.
     * @param name ПІБ.
     * @param position Посада.
     * @param notes Примітки.
     * @return true, якщо успішно.
     */
    Q_INVOKABLE bool updatePerson(int id, const QString &rank, const QString &name, const QString &position, const QString &notes);

    /**
     * @brief Оновлює тільки примітки військовослужбовця (використовується для статусів).
     * @param id ID запису.
     * @param notes Нові примітки.
     * @return true, якщо успішно.
     */
    Q_INVOKABLE bool updatePersonNotes(int id, const QString &notes);

    /**
     * @brief Отримує список доступних звань.
     * @return Список звань.
     */
    Q_INVOKABLE QVariantList getRanks();

    /**
     * @brief Отримує список статусів для конкретного військовослужбовця.
     * @param personId ID в/с.
     * @return Список статусів.
     */
    Q_INVOKABLE QVariantList getStatuses(int personId);

    /**
     * @brief Транзакційно зберігає список статусів для військовослужбовця.
     * Видаляє всі старі записи та записує нові.
     * @param personId ID в/с.
     * @param statusesList Список статусів у форматі QVariantList (QVariantMap).
     * @return true, якщо успішно.
     */
    Q_INVOKABLE bool saveStatuses(int personId, const QVariantList &statusesList);

    /**
     * @brief Оновлює поточний примітку в/с на основі його статусів на сьогодні.
     * @param personId ID в/с.
     */
    Q_INVOKABLE void updateCurrentStatus(int personId);

    /**
     * @brief Глобальна перевірка та оновлення статусів для всього особового складу.
     * Використовується при запуску програми та зміні вкладок.
     */
    Q_INVOKABLE void updateAllPersonnelStatuses();

    /** @return Поточний вибраний ID */
    int selectedPersonId() const { return m_selectedPersonId; }
    /** Встановлює вибраний ID */
    void setSelectedPersonId(int id);

    /** @return ПІБ вибраного в/с */
    QString currentSelectedPersonName() const { return m_currentSelectedPersonName; }

signals:
    /** Сигнал про зміну даних в таблиці personnel */
    void personnelChanged();
    /** Сигнал про зміну вибраного ID */
    void selectedPersonIdChanged();
    /** Сигнал про зміну імені вибраного в/с */
    void currentSelectedPersonNameChanged();

private:
    int m_selectedPersonId = -1; ///< ID поточного вибраного запису
    QString m_currentSelectedPersonName = ""; ///< Ім'я поточного вибраного запису
};

#endif // PERSONNELCONTROLLER_H
