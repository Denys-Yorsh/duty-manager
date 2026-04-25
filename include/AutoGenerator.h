#ifndef AUTOGENERATOR_H
#define AUTOGENERATOR_H

#include <QDate>
#include <QList>
#include <QString>
#include <QMap>

struct Candidate {
    int id;
    int rankPriority;
    int dutyCount; // Кількість нарядів за місяць (для рівномірності)
};

class AutoGenerator {
public:
    AutoGenerator(int month, int year);
    bool run(); // Запуск процесу
    QString lastError() const { return m_error; }

private:
    int m_month;
    int m_year;
    QString m_error;

    // Допоміжні методи
    bool isAvailable(int personId, const QString &currentStatus, const QDate &date);
    bool meetsRankRequirement(int personId, int dutyTypeId);
    bool hasRestInterval(int personId, const QDate &date, int minDays = 2);
};

#endif // AUTOGENERATOR_H
