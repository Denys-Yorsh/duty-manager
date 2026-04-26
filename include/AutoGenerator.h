#ifndef AUTOGENERATOR_H
#define AUTOGENERATOR_H

#include <QDate>
#include <QString>

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
};

#endif // AUTOGENERATOR_H
