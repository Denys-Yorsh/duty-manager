#ifndef STATISTICSCONTROLLER_H
#define STATISTICSCONTROLLER_H

#include <QObject>
#include <QVariantList>
#include <QString>

/**
 * @class StatisticsController
 * @brief Контролер для збору та відображення статистики нарядів.
 * Підраховує кількість виходів на наряди для кожного військовослужбовця за вибраний період.
 */
class StatisticsController : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Конструктор контролера.
     * @param parent Батьківський об'єкт.
     */
    explicit StatisticsController(QObject *parent = nullptr);

    /**
     * @brief Отримує статистику за вказаний місяць або рік.
     * @param month Місяць (1-12).
     * @param year Рік.
     * @param isYearly Якщо true, ігнорує параметр month і рахує за весь рік.
     * @return Список QVariantMap з даними статистики.
     */
    Q_INVOKABLE QVariantList getStatistics(int month, int year, bool isYearly);

    /**
     * @brief Отримує розподіл звань у нарядах для кругової діаграми.
     * @param month Місяць (1-12).
     * @param year Рік.
     * @param isYearly Якщо true, рахує за весь рік.
     * @return Список QVariantMap з мітками та значеннями у відсотках.
     */
    Q_INVOKABLE QVariantList getRankDistribution(int month, int year, bool isYearly);

    /**
     * @brief Експортує статистику в PDF.
     * @param data Дані для експорту.
     * @param filePath Шлях до файлу.
     */
    Q_INVOKABLE void exportToPdf(const QVariantList &data, const QString &filePath);

    /**
     * @brief Експортує статистику в Excel.
     * @param data Дані для експорту.
     * @param filePath Шлях до файлу.
     */
    Q_INVOKABLE void exportToExcel(const QVariantList &data, const QString &filePath);

    /**
     * @brief Отримує список років, для яких є записи у графіку.
     */
    Q_INVOKABLE QVariantList getAvailableYears();

    /**
     * @brief Отримує список місяців для вказаного року.
     */
    Q_INVOKABLE QVariantList getAvailableMonths(int year);

signals:
    /** Сигнал про зміну статистичних даних */
    void statisticsChanged();

    /** Сигнали результату експорту */
    void exportFinished(bool success, const QString &message);
};

#endif // STATISTICSCONTROLLER_H
