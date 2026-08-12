#ifndef DUTYTYPESCONTROLLER_H
#define DUTYTYPESCONTROLLER_H

#include <QObject>
#include <QVariantList>

/**
 * @class DutyTypesController
 * @brief Контролер для управління видами нарядів.
 * Дозволяє переглядати, додавати, редагувати та видаляти типи нарядів у базі даних.
 */
class DutyTypesController : public QObject {
    Q_OBJECT
    /**
     * @property selectedDutyTypeId
     * @brief ID вибраного типу наряду в інтерфейсі.
     */
    Q_PROPERTY(int selectedDutyTypeId READ selectedDutyTypeId WRITE setSelectedDutyTypeId NOTIFY selectedDutyTypeIdChanged)

public:
    /**
     * @brief Конструктор контролера.
     * @param parent Батьківський об'єкт.
     */
    explicit DutyTypesController(QObject *parent = nullptr);

    /**
     * @brief Отримує повний список видів нарядів.
     * @return Список QVariantMap з даними нарядів.
     */
    Q_INVOKABLE QVariantList getDutyTypes();

    /**
     * @brief Додає новий вид наряду.
     * @param name Назва наряду.
     * @param minRank Назва мінімального звання.
     * @param maxRank Назва максимального звання.
     * @param restDays Кількість днів відпочинку.
     * @param personCount Кількість осіб у наряді.
     * @return true, якщо успішно.
     */
    Q_INVOKABLE bool addDutyType(const QString &name, const QString &minRank, const QString &maxRank, int restDays, int personCount);

    /**
     * @brief Оновлює існуючий вид наряду.
     * @param id ID запису.
     * @param name Назва наряду.
     * @param minRank Назва мінімального звання.
     * @param maxRank Назва максимального звання.
     * @param restDays Кількість днів відпочинку.
     * @param personCount Кількість осіб у наряді.
     * @return true, якщо успішно.
     */
    Q_INVOKABLE bool updateDutyType(int id, const QString &name, const QString &minRank, const QString &maxRank, int restDays, int personCount);

    /**
     * @brief Видаляє вид наряду за ID.
     * @param id ID запису.
     * @return true, якщо успішно.
     */
    Q_INVOKABLE bool deleteDutyType(int id);

    /** @return Поточний вибраний ID */
    int selectedDutyTypeId() const { return m_selectedDutyTypeId; }
    /** Встановлює вибраний ID */
    void setSelectedDutyTypeId(int id);

signals:
    /** Сигнал про зміну списку видів нарядів */
    void dutyTypesChanged();
    /** Сигнал про зміну вибраного ID */
    void selectedDutyTypeIdChanged();

private:
    int m_selectedDutyTypeId = -1; ///< ID поточного вибраного типу наряду
};

#endif // DUTYTYPESCONTROLLER_H
