#include "AutoGenerator.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

AutoGenerator::AutoGenerator(int month, int year) : m_month(month), m_year(year) {}

bool AutoGenerator::run() {
    QDate firstDay(m_year, m_month, 1);
    int days = firstDay.daysInMonth();

    // 1. Отримуємо всі типи нарядів
    QList<int> dutyTypeIds;
    QSqlQuery qDuty("SELECT id FROM duty_types");
    while (qDuty.next()) dutyTypeIds.append(qDuty.value(0).toInt());

    // 2. Очищуємо старий (не заблокований вручну) графік за цей місяць
    QSqlQuery qDel;
    qDel.prepare("DELETE FROM schedule WHERE strftime('%m', duty_date) = ? AND strftime('%Y', duty_date) = ? AND is_manual = 0");
    qDel.addBindValue(QString("%1").arg(m_month, 2, 10, QChar('0')));
    qDel.addBindValue(QString::number(m_year));
    qDel.exec();

    // 3. Цикл по днях
    for (int d = 1; d <= days; ++d) {
        QDate currentDate(m_year, m_month, d);

        for (int dutyTypeId : dutyTypeIds) {
            // Шукаємо найкращого кандидата
            // Критерії: не в наряді, не у відпустці, пройшло 2 дні, найменше нарядів в цьому місяці
            QSqlQuery qBest;
            qBest.prepare(
                "SELECT p.id FROM personnel p "
                "JOIN ranks r ON p.rank_id = r.id "
                "WHERE p.is_active = 1 "
                "AND NOT EXISTS (SELECT 1 FROM personnel_statuses ps WHERE ps.person_id = p.id AND ? BETWEEN ps.start_date AND ps.end_date) "
                "AND NOT EXISTS (SELECT 1 FROM schedule s WHERE s.person_id = p.id AND s.duty_date BETWEEN ? AND ?) "
                "ORDER BY (SELECT COUNT(*) FROM schedule s2 WHERE s2.person_id = p.id AND strftime('%m', s2.duty_date) = ?) ASC, r.priority DESC "
                "LIMIT 1"
            );
            
            QString dateStr = currentDate.toString(Qt::ISODate);
            qBest.addBindValue(dateStr);
            qBest.addBindValue(currentDate.addDays(-2).toString(Qt::ISODate)); // Відпочинок 2 дні до
            qBest.addBindValue(currentDate.addDays(2).toString(Qt::ISODate));  // Відпочинок 2 дні після
            qBest.addBindValue(QString("%1").arg(m_month, 2, 10, QChar('0')));

            if (qBest.exec() && qBest.next()) {
                int personId = qBest.value(0).toInt();
                
                QSqlQuery qIns;
                qIns.prepare("INSERT INTO schedule (duty_date, person_id, duty_type_id) VALUES (?, ?, ?)");
                qIns.addBindValue(dateStr);
                qIns.addBindValue(personId);
                qIns.addBindValue(dutyTypeId);
                if (!qIns.exec()) {
                    qDebug() << "Insert error:" << qIns.lastError().text();
                }
            }
        }
    }

    return true;
}
