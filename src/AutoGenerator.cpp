#include "AutoGenerator.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QSet>
#include <algorithm>

AutoGenerator::AutoGenerator(int month, int year) : m_month(month), m_year(year) {}

bool AutoGenerator::run() {
    QDate firstDay(m_year, m_month, 1);
    int days = firstDay.daysInMonth();
    QString monthStr = QString("%1").arg(m_month, 2, 10, QChar('0'));
    QString yearStr = QString::number(m_year);

    // 1. Отримуємо типи нарядів з пріоритетами звань
    struct DutyType {
        int id;
        QString name;
        int personCount;
        int minRankPriority;
        int maxRankPriority;
    };
    QList<DutyType> dutyTypes;
    QSqlQuery qDuty("SELECT dt.id, dt.name, dt.person_count, r1.priority, r2.priority "
                    "FROM duty_types dt "
                    "LEFT JOIN ranks r1 ON dt.min_rank_id = r1.id "
                    "LEFT JOIN ranks r2 ON dt.max_rank_id = r2.id");
    while (qDuty.next()) {
        dutyTypes.append({
            qDuty.value(0).toInt(),
            qDuty.value(1).toString(),
            qDuty.value(2).toInt(),
            qDuty.value(3).toInt(),
            qDuty.value(4).toInt()
        });
    }

    if (dutyTypes.isEmpty()) {
        m_error = "Типи нарядів не знайдені.";
        return false;
    }

    // Очищуємо старий графік (автоматичний) за вибраний місяць
    QSqlQuery qDel;
    qDel.prepare("DELETE FROM schedule WHERE strftime('%m', duty_date) = ? AND strftime('%Y', duty_date) = ? AND is_manual = 0");
    qDel.addBindValue(monthStr);
    qDel.addBindValue(yearStr);
    qDel.exec();

    // 2. Отримуємо особовий склад з пріоритетом звання
    struct Person {
        int id;
        QString name;
        QString isActive;
        int rankPriority;
        int monthlyDutyCount = 0;
        QDate lastDutyDate;
    };
    QList<Person> personnel;
    QSqlQuery qPers("SELECT p.id, p.full_name, p.is_active, r.priority "
                    "FROM personnel p "
                    "LEFT JOIN ranks r ON p.rank_id = r.id");
    while (qPers.next()) {
        personnel.append({
            qPers.value(0).toInt(), 
            qPers.value(1).toString(),
            qPers.value(2).toString(),
            qPers.value(3).toInt()
        });
    }

    if (personnel.isEmpty()) {
        m_error = "Особовий склад порожній.";
        return false;
    }

    // Попередній підрахунок нарядів для рівномірного розподілу
    for (int i = 0; i < personnel.size(); ++i) {
        QSqlQuery qCount;
        qCount.prepare("SELECT COUNT(*), MAX(duty_date) FROM schedule WHERE person_id = ? "
                       "AND strftime('%m', duty_date) = ? AND strftime('%Y', duty_date) = ?");
        qCount.addBindValue(personnel[i].id);
        qCount.addBindValue(monthStr);
        qCount.addBindValue(yearStr);
        if (qCount.exec() && qCount.next()) {
            personnel[i].monthlyDutyCount = qCount.value(0).toInt();
            if (!qCount.value(1).isNull()) {
                personnel[i].lastDutyDate = QDate::fromString(qCount.value(1).toString(), Qt::ISODate);
            }
        }
    }

    // 3. Основний цикл генерації по днях
    for (int d = 1; d <= days; ++d) {
        QDate currentDate(m_year, m_month, d);
        QSet<int> assignedToday;

        for (const auto& duty : dutyTypes) {
            int needed = duty.personCount > 0 ? duty.personCount : 1;
            QList<int> availableIndices;

            for (int i = 0; i < personnel.size(); ++i) {
                // ПЕРЕВІРКА ЗВАННЯ: боєць має бути в межах допуску наряду
                if (personnel[i].rankPriority < duty.minRankPriority || 
                    personnel[i].rankPriority > duty.maxRankPriority) {
                    continue; 
                }

                // Перевірка відпочинку: не два дні поспіль
                if (personnel[i].lastDutyDate.isValid() && personnel[i].lastDutyDate.addDays(1) >= currentDate) continue;
                
                // Перевірка зайнятості сьогодні в іншому наряді
                if (assignedToday.contains(personnel[i].id)) continue;
                
                // Перевірка статусів (відпустка, лікарняний тощо)
                if (!isAvailable(personnel[i].id, personnel[i].isActive, currentDate)) continue;

                availableIndices.append(i);
            }

            // Сортування: в наряд іде той, у кого найменше виходів за місяць
            std::sort(availableIndices.begin(), availableIndices.end(), [&](int a, int b) {
                return personnel[a].monthlyDutyCount < personnel[b].monthlyDutyCount;
            });

            int count = 0;
            for (int idx : availableIndices) {
                if (count >= needed) break;

                QSqlQuery qIns;
                qIns.prepare("INSERT INTO schedule (duty_date, person_id, duty_type_id, is_manual) VALUES (?, ?, ?, 0)");
                qIns.addBindValue(currentDate.toString(Qt::ISODate));
                qIns.addBindValue(personnel[idx].id);
                qIns.addBindValue(duty.id);
                
                if (qIns.exec()) {
                    personnel[idx].monthlyDutyCount++;
                    personnel[idx].lastDutyDate = currentDate;
                    assignedToday.insert(personnel[idx].id);
                    count++;
                }
            }
        }
    }

    return true;
}

bool AutoGenerator::isAvailable(int personId, const QString &currentStatus, const QDate &date) {
    // Перевірка таблиці статусів по датах
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM personnel_statuses WHERE person_id = ? AND ? BETWEEN start_date AND end_date");
    q.addBindValue(personId);
    q.addBindValue(date.toString(Qt::ISODate));
    
    if (q.exec() && q.next()) {
        if (q.value(0).toInt() > 0) return false;
    }

    // Перевірка глобального статусу "Звільнений"
    if (currentStatus != "в наявності") {
        if (currentStatus == "Звільнений" || currentStatus == "Виключений") return false;
    }

    return true;
}
