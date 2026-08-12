#include "AutoGenerator.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QSet>
#include <algorithm>
#include <QDebug>

/**
 * @brief Конструктор AutoGenerator.
 */
AutoGenerator::AutoGenerator(int month, int year, int startDay) 
    : m_month(month), m_year(year), m_startDay(startDay) {}

/**
 * @brief Основний алгоритм генерації графіка.
 */
bool AutoGenerator::run() {
    QDate firstDay(m_year, m_month, 1);
    int daysInMonth = firstDay.daysInMonth();
    QString monthStr = QString("%1").arg(m_month, 2, 10, QChar('0'));
    QString yearStr = QString::number(m_year);
    QDate startDate(m_year, m_month, m_startDay);

    // 1. Отримуємо типи нарядів
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
        m_error = "Типи нарядів не знайдено.";
        return false;
    }

    // 2. Очищуємо автоматичні записи ПІСЛЯ вибраного дня
    QSqlQuery qDel;
    qDel.prepare("DELETE FROM schedule WHERE duty_date >= ? AND is_manual = 0");
    qDel.addBindValue(startDate.toString(Qt::ISODate));
    qDel.exec();

    // 3. Отримуємо особовий склад
    struct Person {
        int id;
        QString name;
        QString notes;
        int rankPriority;
        int monthlyDutyCount = 0;
        QDate nextAvailableDate; 
    };
    QList<Person> personnel;
    QSqlQuery qPers("SELECT p.id, p.name, p.notes, r.priority FROM personnel p LEFT JOIN ranks r ON p.rank_id = r.id");
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

    // Розрахунок початкових даних (враховуючи кінець попереднього місяця)
    for (int i = 0; i < personnel.size(); ++i) {
        // Рахуємо кількість нарядів у поточному місяці (до startDate)
        QSqlQuery qCount;
        qCount.prepare("SELECT COUNT(*) FROM schedule WHERE person_id = ? "
                       "AND duty_date < ? AND strftime('%m', duty_date) = ? AND strftime('%Y', duty_date) = ?");
        qCount.addBindValue(personnel[i].id);
        qCount.addBindValue(startDate.toString(Qt::ISODate));
        qCount.addBindValue(monthStr);
        qCount.addBindValue(yearStr);
        if (qCount.exec() && qCount.next()) personnel[i].monthlyDutyCount = qCount.value(0).toInt();

        // Шукаємо дату ОСТАННЬОГО наряду (може бути в минулому місяці)
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

    // 4. Генерація по днях
    for (int d = m_startDay; d <= daysInMonth; ++d) {
        QDate currentDate(m_year, m_month, d);
        QSet<int> assignedToday;

        // Враховуємо ручні призначення (is_manual = 1) та існуючі автоматичні (до m_startDay)
        QSqlQuery qExist;
        qExist.prepare("SELECT person_id FROM schedule WHERE duty_date = ?");
        qExist.addBindValue(currentDate.toString(Qt::ISODate));
        if (qExist.exec()) {
            while (qExist.next()) assignedToday.insert(qExist.value(0).toInt());
        }

        for (const auto& duty : dutyTypes) {
            // Рахуємо скільки вже людей призначено на цей наряд сьогодні
            QSqlQuery qFilled;
            qFilled.prepare("SELECT COUNT(*) FROM schedule WHERE duty_date = ? AND duty_type_id = ?");
            qFilled.addBindValue(currentDate.toString(Qt::ISODate));
            qFilled.addBindValue(duty.id);
            int alreadyAssigned = 0;
            if (qFilled.exec() && qFilled.next()) alreadyAssigned = qFilled.value(0).toInt();

            int needed = duty.personCount - alreadyAssigned;
            if (needed <= 0) continue;

            QList<int> available;
            for (int i = 0; i < personnel.size(); ++i) {
                if (personnel[i].rankPriority < duty.minRankPriority || 
                    (duty.maxRankPriority > 0 && personnel[i].rankPriority > duty.maxRankPriority)) continue;

                if (personnel[i].nextAvailableDate.isValid() && personnel[i].nextAvailableDate > currentDate) continue;
                if (assignedToday.contains(personnel[i].id)) continue;
                if (!isAvailable(personnel[i].id, personnel[i].notes, currentDate)) continue;

                available.append(i);
            }

            // Пріоритет тим, хто менше заступав
            std::sort(available.begin(), available.end(), [&](int a, int b) {
                return personnel[a].monthlyDutyCount < personnel[b].monthlyDutyCount;
            });

            for (int idx : available) {
                if (needed <= 0) break;

                QSqlQuery qIns;
                qIns.prepare("INSERT INTO schedule (duty_date, person_id, duty_type_id, is_manual) VALUES (?, ?, ?, 0)");
                qIns.addBindValue(currentDate.toString(Qt::ISODate));
                qIns.addBindValue(personnel[idx].id);
                qIns.addBindValue(duty.id);
                
                if (qIns.exec()) {
                    personnel[idx].monthlyDutyCount++;
                    personnel[idx].nextAvailableDate = currentDate.addDays(duty.restDays + 1);
                    assignedToday.insert(personnel[idx].id);
                    needed--;
                }
            }
        }
    }
    return true;
}

/**
 * @brief Перевірка доступності в/с.
 */
bool AutoGenerator::isAvailable(int personId, const QString &currentNotes, const QDate &date) {
    if (currentNotes != "в наявності") return false;

    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM personnel_statuses WHERE person_id = ? AND ? BETWEEN start_date AND end_date");
    q.addBindValue(personId);
    q.addBindValue(date.toString(Qt::ISODate));
    
    if (q.exec() && q.next()) {
        if (q.value(0).toInt() > 0) return false;
    }
    return true;
}
