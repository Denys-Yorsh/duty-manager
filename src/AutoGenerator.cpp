#include "AutoGenerator.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QSet>
#include <algorithm>

AutoGenerator::AutoGenerator(int month, int year, int startDay) 
    : m_month(month), m_year(year), m_startDay(startDay) {}

bool AutoGenerator::run() {
    QDate firstDay(m_year, m_month, 1);
    int days = firstDay.daysInMonth();
    QString monthStr = QString("%1").arg(m_month, 2, 10, QChar('0'));
    QString yearStr = QString::number(m_year);
    QDate startDate(m_year, m_month, m_startDay);

    // 1. Отримуємо типи нарядів з пріоритетами звань та днями відпочинку
    struct DutyType {
        int id;
        QString name;
        int personCount;
        int minRankPriority;
        int maxRankPriority;
        int restDays;
    };
    QList<DutyType> dutyTypes;
    QSqlQuery qDuty("SELECT dt.id, dt.name, dt.person_count, r1.priority, r2.priority, dt.rest_days "
                    "FROM duty_types dt "
                    "LEFT JOIN ranks r1 ON dt.min_rank_id = r1.id "
                    "LEFT JOIN ranks r2 ON dt.max_rank_id = r2.id");
    while (qDuty.next()) {
        dutyTypes.append({
            qDuty.value(0).toInt(),
            qDuty.value(1).toString(),
            qDuty.value(2).toInt(),
            qDuty.value(3).toInt(),
            qDuty.value(4).toInt(),
            qDuty.value(5).toInt()
        });
    }

    if (dutyTypes.isEmpty()) {
        m_error = "Типи нарядів не знайдені.";
        return false;
    }

    // Очищуємо старий графік (автоматичний) за вибраний період
    QSqlQuery qDel;
    if (m_startDay == 1) {
        qDel.prepare("DELETE FROM schedule WHERE strftime('%m', duty_date) = ? AND strftime('%Y', duty_date) = ? AND is_manual = 0");
        qDel.addBindValue(monthStr);
        qDel.addBindValue(yearStr);
    } else {
        qDel.prepare("DELETE FROM schedule WHERE duty_date >= ? AND strftime('%m', duty_date) = ? AND strftime('%Y', duty_date) = ? AND is_manual = 0");
        qDel.addBindValue(startDate.toString(Qt::ISODate));
        qDel.addBindValue(monthStr);
        qDel.addBindValue(yearStr);
    }
    qDel.exec();

    // 2. Отримуємо особовий склад з пріоритетом звання
    struct Person {
        int id;
        QString name;
        QString isActive;
        int rankPriority;
        int monthlyDutyCount = 0;
        QDate nextAvailableDate; // Дата, коли людина може заступити знову
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
            qPers.value(3).toInt(),
            0,
            QDate()
        });
    }

    if (personnel.isEmpty()) {
        m_error = "Особовий склад порожній.";
        return false;
    }

    // Попередній підрахунок нарядів та визначення наступної вільної дати
    for (int i = 0; i < personnel.size(); ++i) {
        // Рахуємо наряди за поточний місяць ДО дати початку (включно з ручними)
        QSqlQuery qCount;
        if (m_startDay == 1) {
             qCount.prepare("SELECT COUNT(*) FROM schedule WHERE person_id = ? "
                           "AND strftime('%m', duty_date) = ? AND strftime('%Y', duty_date) = ?");
             qCount.addBindValue(personnel[i].id);
             qCount.addBindValue(monthStr);
             qCount.addBindValue(yearStr);
        } else {
             qCount.prepare("SELECT COUNT(*) FROM schedule WHERE person_id = ? "
                           "AND duty_date < ? AND strftime('%m', duty_date) = ? AND strftime('%Y', duty_date) = ?");
             qCount.addBindValue(personnel[i].id);
             qCount.addBindValue(startDate.toString(Qt::ISODate));
             qCount.addBindValue(monthStr);
             qCount.addBindValue(yearStr);
        }
        
        if (qCount.exec() && qCount.next()) {
            personnel[i].monthlyDutyCount = qCount.value(0).toInt();
        }

        // Шукаємо ОСТАННІЙ наряд ПЕРЕД датою початку, щоб знати коли людина звільниться
        QSqlQuery qLast;
        qLast.prepare("SELECT s.duty_date, dt.rest_days FROM schedule s "
                      "JOIN duty_types dt ON s.duty_type_id = dt.id "
                      "WHERE s.person_id = ? AND s.duty_date < ? "
                      "ORDER BY s.duty_date DESC LIMIT 1");
        qLast.addBindValue(personnel[i].id);
        qLast.addBindValue(startDate.toString(Qt::ISODate));

        if (qLast.exec() && qLast.next()) {
            QDate lastDate = QDate::fromString(qLast.value(0).toString(), Qt::ISODate);
            int rest = qLast.value(1).toInt();
            personnel[i].nextAvailableDate = lastDate.addDays(rest + 1);
        }
    }

    // 3. Основний цикл генерації по днях
    for (int d = m_startDay; d <= days; ++d) {
        QDate currentDate(m_year, m_month, d);
        QSet<int> assignedToday;

        // --- ОБРОБКА РУЧНИХ ПРАВОК ---
        // Якщо на цей день уже є ручні наряди, ми маємо врахувати їх:
        // 1. Позначити людей як зайнятих (assignedToday).
        // 2. Оновити їх лічильник нарядів (monthlyDutyCount).
        // 3. Оновити їх дату наступного відпочинку (nextAvailableDate).
        QSqlQuery qCheckManual;
        qCheckManual.prepare("SELECT s.person_id, dt.rest_days FROM schedule s "
                              "JOIN duty_types dt ON s.duty_type_id = dt.id "
                              "WHERE s.duty_date = ? AND s.is_manual = 1");
        qCheckManual.addBindValue(currentDate.toString(Qt::ISODate));
        if (qCheckManual.exec()) {
            while (qCheckManual.next()) {
                int pid = qCheckManual.value(0).toInt();
                int rest = qCheckManual.value(1).toInt();
                assignedToday.insert(pid);
                
                // Знаходимо людину в нашому списку і оновлюємо дані
                for (int i = 0; i < personnel.size(); ++i) {
                    if (personnel[i].id == pid) {
                        personnel[i].monthlyDutyCount++;
                        personnel[i].nextAvailableDate = currentDate.addDays(rest + 1);
                        break;
                    }
                }
            }
        }

        for (const auto& duty : dutyTypes) {
            int needed = duty.personCount > 0 ? duty.personCount : 1;
            QList<int> availableIndices;

            for (int i = 0; i < personnel.size(); ++i) {
                // ПЕРЕВІРКА ЗВАННЯ: боєць має бути в межах допуску наряду
                if (personnel[i].rankPriority < duty.minRankPriority || 
                    personnel[i].rankPriority > duty.maxRankPriority) {
                    continue; 
                }

                // Перевірка відпочинку: чи наступила вже дата, коли можна заступати
                if (personnel[i].nextAvailableDate.isValid() && personnel[i].nextAvailableDate > currentDate) {
                    continue;
                }
                
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
                    // Оновлюємо дату наступного наряду: сьогодні + дні відпочинку + 1 (день самого наряду)
                    personnel[idx].nextAvailableDate = currentDate.addDays(duty.restDays + 1);
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
