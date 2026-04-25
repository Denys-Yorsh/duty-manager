#include "AutoGenerator.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QSqlRecord>
#include <algorithm>

AutoGenerator::AutoGenerator(int month, int year) : m_month(month), m_year(year) {}

bool AutoGenerator::run() {
    QDate firstDay(m_year, m_month, 1);
    int days = firstDay.daysInMonth();
    QString monthStr = QString("%1").arg(m_month, 2, 10, QChar('0'));
    QString yearStr = QString::number(m_year);

    struct DutyType {
        int id;
        QString name;
        int personCount;
    };
    QList<DutyType> dutyTypes;
    QSqlQuery qDuty("SELECT id, name, person_count FROM duty_types");
    while (qDuty.next()) {
        dutyTypes.append({
            qDuty.value(0).toInt(),
            qDuty.value(1).toString(),
            qDuty.value(2).toInt()
        });
    }

    if (dutyTypes.isEmpty()) {
        m_error = "Типи нарядів не знайдені.";
        return false;
    }

    QSqlQuery qDel;
    qDel.prepare("DELETE FROM schedule WHERE strftime('%m', duty_date) = ? AND strftime('%Y', duty_date) = ? AND is_manual = 0");
    qDel.addBindValue(monthStr);
    qDel.addBindValue(yearStr);
    if (!qDel.exec()) {
        m_error = qDel.lastError().text();
        return false;
    }

    struct Person {
        int id;
        QString name;
        QString isActive;
        int monthlyDutyCount = 0;
        QDate lastDutyDate;
    };
    QList<Person> personnel;
    QSqlQuery qPers("SELECT id, full_name, is_active FROM personnel");
    while (qPers.next()) {
        personnel.append({
            qPers.value(0).toInt(), 
            qPers.value(1).toString(),
            qPers.value(2).toString()
        });
    }

    if (personnel.isEmpty()) {
        m_error = "Особовий склад порожній.";
        return false;
    }

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

    for (int d = 1; d <= days; ++d) {
        QDate currentDate(m_year, m_month, d);
        QSet<int> assignedToday;

        for (const auto& duty : dutyTypes) {
            int needed = duty.personCount;
            if (needed <= 0) needed = 1;

            QList<int> availableIndices;
            for (int i = 0; i < personnel.size(); ++i) {
                // Правило: не два дні поспіль
                if (personnel[i].lastDutyDate.isValid() && personnel[i].lastDutyDate.addDays(1) >= currentDate) {
                    continue;
                }
                
                if (assignedToday.contains(personnel[i].id)) {
                    continue;
                }

                // Перевірка доступності за датою (враховуючи Примітки та Таблицю статусів)
                if (!isAvailable(personnel[i].id, personnel[i].isActive, currentDate)) {
                    continue;
                }

                availableIndices.append(i);
            }

            std::sort(availableIndices.begin(), availableIndices.end(), [&](int a, int b) {
                return personnel[a].monthlyDutyCount < personnel[b].monthlyDutyCount;
            });

            int count = 0;
            for (int idx : availableIndices) {
                if (count >= needed) break;

                int personId = personnel[idx].id;
                
                QSqlQuery qIns;
                qIns.prepare("INSERT INTO schedule (duty_date, person_id, duty_type_id, is_manual) VALUES (?, ?, ?, 0)");
                qIns.addBindValue(currentDate.toString(Qt::ISODate));
                qIns.addBindValue(personId);
                qIns.addBindValue(duty.id);
                
                if (qIns.exec()) {
                    personnel[idx].monthlyDutyCount++;
                    personnel[idx].lastDutyDate = currentDate;
                    assignedToday.insert(personId);
                    count++;
                }
            }
        }
    }

    return true;
}

bool AutoGenerator::isAvailable(int personId, const QString &currentStatus, const QDate &date) {
    // 1. Перевіряємо таблицю статусів (найточніша інформація по датах)
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM personnel_statuses WHERE person_id = ? AND ? BETWEEN start_date AND end_date");
    q.addBindValue(personId);
    q.addBindValue(date.toString(Qt::ISODate));
    
    if (q.exec() && q.next()) {
        if (q.value(0).toInt() > 0) return false; // Однозначно зайнятий (відпустка, лікарняний тощо)
    }

    // 2. Якщо в основній таблиці статус НЕ "в наявності", але в таблиці статусів немає запису на цю дату,
    // то це може означати або що він вже повернувся, або він відсутній постійно.
    // Згідно ТЗ: "якщо не в наявності, то подивитися по даті коли буде в наявності"
    if (currentStatus != "в наявності") {
        // Перевіряємо, чи є взагалі майбутні записи, які зроблять його "в наявності"
        // (в нашій логіці: якщо на конкретну дату 'date' немає забороняючого статусу, вважаємо доступним)
        // Але якщо статус "Звільнений", то він ніколи не буде в наявності.
        if (currentStatus == "Звільнений" || currentStatus == "Виключений") return false;
    }

    return true;
}

bool AutoGenerator::meetsRankRequirement(int personId, int dutyTypeId) { return true; }
bool AutoGenerator::hasRestInterval(int personId, const QDate &date, int minDays) { return true; }
