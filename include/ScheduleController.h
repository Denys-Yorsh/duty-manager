#ifndef SCHEDULECONTROLLER_H
#define SCHEDULECONTROLLER_H

#include <QObject>
#include <QVariantList>
#include <QDate>

/**
 * @class ScheduleController
 * @brief Контролер для управління графіком нарядів.
 * Відповідає за отримання структури графіка, призначення людей на наряди та експорт даних.
 */
class ScheduleController : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Конструктор контролера.
     * @param parent Батьківський об'єкт.
     */
    explicit ScheduleController(QObject *parent = nullptr);

    /**
     * @brief Отримує структуру графіка (типи нарядів та кількість місць).
     * Додає інформацію для об'єднання комірок у заголовках.
     */
    Q_INVOKABLE QVariantList getScheduleStructure();

    /**
     * @brief Отримує дані графіка на вибраний місяць та рік.
     * @param month Місяць (1-12).
     * @param year Рік.
     * @return Список призначень.
     */
    Q_INVOKABLE QVariantList getScheduleData(int month, int year);

    /**
     * @brief Призначає військовослужбовця на конкретний наряд.
     * @param day День місяця.
     * @param month Місяць.
     * @param year Рік.
     * @param dutyTypeId ID типу наряду.
     * @param personId ID нового в/с (0 для очищення).
     * @param oldPersonId ID попереднього в/с (для заміни).
     * @return true, якщо успішно.
     */
    Q_INVOKABLE bool assignPerson(int day, int month, int year, int dutyTypeId, int personId, int oldPersonId = -1);

    /**
     * @brief Отримує список в/с, доступних для заступання в наряд.
     * @return Список доступних людей.
     */
    Q_INVOKABLE QVariantList getAvailablePersonnel();

    /**
     * @brief Запускає алгоритм автоматичної генерації графіка.
     * @param month Місяць генерації.
     * @param year Рік генерації.
     * @param startDay День, з якого починати генерацію.
     * @return true, якщо генерація успішна.
     */
    Q_INVOKABLE bool generateSchedule(int month, int year, int startDay = 1);

    /**
     * @brief Отримує список років, для яких є записи у графіку.
     * @return Список років.
     */
    Q_INVOKABLE QVariantList getAvailableYears();

    /**
     * @brief Отримує список місяців для вказаного року, для яких є записи у графіку.
     * @param year Рік.
     * @return Список місяців (1-12).
     */
    Q_INVOKABLE QVariantList getAvailableMonths(int year);

    /**
     * @brief Експортує графік у формат PDF.
     * @param month Місяць.
     * @param year Рік.
     * @param filePath Шлях для збереження файлу.
     */
    Q_INVOKABLE void exportToPdf(int month, int year, const QString &filePath);

    /**
     * @brief Експортує графік у формат Excel.
     * @param month Місяць.
     * @param year Рік.
     * @param filePath Шлях для збереження файлу.
     */
    Q_INVOKABLE void exportToExcel(int month, int year, const QString &filePath);

signals:
    /** Сигнал про зміну даних у графіку */
    void scheduleChanged();
    
    /** Сигнали результату експорту */
    void exportFinished(bool success, const QString &message);

private:
    /**
     * @brief Скорочує ПІБ до формату "Прізвище І. П."
     * @param fullName Повне ім'я.
     * @return Скорочене ім'я.
     */
    QString shortenName(const QString &fullName) const;
};

#endif // SCHEDULECONTROLLER_H
