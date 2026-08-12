#ifndef AUTOGENERATOR_H
#define AUTOGENERATOR_H

#include <QDate>
#include <QString>

/**
 * @class AutoGenerator
 * @brief Клас для автоматичної генерації графіка нарядів.
 * Використовує алгоритм розподілу нарядів на основі пріоритетів звань,
 * кількості виходів за місяць та обов'язкових днів відпочинку.
 */
class AutoGenerator {
public:
    /**
     * @brief Конструктор генератора.
     * @param month Місяць для генерації.
     * @param year Рік для генерації.
     * @param startDay День місяця, з якого починати генерацію (за замовчуванням 1).
     */
    AutoGenerator(int month, int year, int startDay = 1);

    /**
     * @brief Запускає процес автоматичної генерації.
     * @return true, якщо генерація пройшла успішно, false у разі помилки.
     */
    bool run();

    /**
     * @brief Повертає текст останньої помилки.
     * @return Рядок з описом помилки.
     */
    QString lastError() const { return m_error; }

private:
    int m_month; ///< Місяць генерації
    int m_year; ///< Рік генерації
    int m_startDay; ///< День початку
    QString m_error; ///< Опис помилки

    /**
     * @brief Перевіряє доступність військовослужбовця на конкретну дату.
     * Враховує як глобальний статус, так і записи в таблиці personnel_statuses.
     * @param personId ID в/с.
     * @param currentNotes Поточні примітки (статус) з таблиці personnel.
     * @param date Дата для перевірки.
     * @return true, якщо людина може заступати в наряд.
     */
    bool isAvailable(int personId, const QString &currentNotes, const QDate &date);
};

#endif // AUTOGENERATOR_H
