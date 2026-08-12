#include "DutyTypesController.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>
#include <QVariantMap>

/**
 * @brief Конструктор DutyTypesController.
 * @param parent Батьківський об'єкт QObject.
 */
DutyTypesController::DutyTypesController(QObject *parent) : QObject(parent) {
}

/**
 * @brief Встановлює ID вибраного типу наряду.
 * @param id ID типу наряду.
 */
void DutyTypesController::setSelectedDutyTypeId(int id) {
    if (m_selectedDutyTypeId != id) {
        m_selectedDutyTypeId = id;
        emit selectedDutyTypeIdChanged();
    }
}

/**
 * @brief Отримує список всіх видів нарядів з бази даних з назвами звань.
 * @return Список QVariantList з даними нарядів.
 */
QVariantList DutyTypesController::getDutyTypes() {
    QVariantList list;
    QSqlQuery query("SELECT dt.id, dt.name, r1.name as min_rank, r2.name as max_rank, dt.rest_days, dt.person_count "
                    "FROM duty_types dt "
                    "LEFT JOIN ranks r1 ON dt.min_rank_id = r1.id "
                    "LEFT JOIN ranks r2 ON dt.max_rank_id = r2.id "
                    "ORDER BY dt.id ASC");
    
    while (query.next()) {
        list.append(QVariantMap{
            {"id", query.value("id")},
            {"name", query.value("name")},
            {"minRank", query.value("min_rank")},
            {"maxRank", query.value("max_rank")},
            {"restDays", query.value("rest_days")},
            {"personCount", query.value("person_count")}
        });
    }
    return list;
}

/**
 * @brief Додає новий тип наряду до бази даних.
 */
bool DutyTypesController::addDutyType(const QString &name, const QString &minRank, const QString &maxRank, int restDays, int personCount) {
    int minId = -1, maxId = -1;
    QSqlQuery rq;
    
    rq.prepare("SELECT id FROM ranks WHERE name = ?");
    rq.addBindValue(minRank);
    if (rq.exec() && rq.next()) minId = rq.value(0).toInt();
    
    rq.prepare("SELECT id FROM ranks WHERE name = ?");
    rq.addBindValue(maxRank);
    if (rq.exec() && rq.next()) maxId = rq.value(0).toInt();

    QSqlQuery query;
    query.prepare("INSERT INTO duty_types (name, min_rank_id, max_rank_id, rest_days, person_count) "
                  "VALUES (:name, :min, :max, :rest, :count)");
    query.bindValue(":name", name);
    query.bindValue(":min", minId != -1 ? QVariant(minId) : QVariant(QMetaType::fromType<int>()));
    query.bindValue(":max", maxId != -1 ? QVariant(maxId) : QVariant(QMetaType::fromType<int>()));
    query.bindValue(":rest", restDays);
    query.bindValue(":count", personCount);

    if (query.exec()) {
        emit dutyTypesChanged();
        return true;
    }
    qWarning() << "Помилка додавання типу наряду:" << query.lastError().text();
    return false;
}

/**
 * @brief Оновлює існуючий тип наряду.
 */
bool DutyTypesController::updateDutyType(int id, const QString &name, const QString &minRank, const QString &maxRank, int restDays, int personCount) {
    int minId = -1, maxId = -1;
    QSqlQuery rq;
    
    rq.prepare("SELECT id FROM ranks WHERE name = ?");
    rq.addBindValue(minRank);
    if (rq.exec() && rq.next()) minId = rq.value(0).toInt();
    
    rq.prepare("SELECT id FROM ranks WHERE name = ?");
    rq.addBindValue(maxRank);
    if (rq.exec() && rq.next()) maxId = rq.value(0).toInt();

    QSqlQuery query;
    query.prepare("UPDATE duty_types SET name = :name, min_rank_id = :min, max_rank_id = :max, "
                  "rest_days = :rest, person_count = :count WHERE id = :id");
    query.bindValue(":name", name);
    query.bindValue(":min", minId != -1 ? QVariant(minId) : QVariant(QMetaType::fromType<int>()));
    query.bindValue(":max", maxId != -1 ? QVariant(maxId) : QVariant(QMetaType::fromType<int>()));
    query.bindValue(":rest", restDays);
    query.bindValue(":count", personCount);
    query.bindValue(":id", id);

    if (query.exec()) {
        emit dutyTypesChanged();
        return true;
    }
    qWarning() << "Помилка оновлення типу наряду:" << query.lastError().text();
    return false;
}

/**
 * @brief Видаляє тип наряду. Також видаляє всі записи з графіка, що на нього посилаються.
 */
bool DutyTypesController::deleteDutyType(int id) {
    // Спочатку видаляємо записи з графіка, щоб не порушувати цілісність (Foreign Key)
    QSqlQuery qSchedule;
    qSchedule.prepare("DELETE FROM schedule WHERE duty_type_id = :id");
    qSchedule.bindValue(":id", id);
    if (!qSchedule.exec()) {
        qWarning() << "Помилка видалення пов'язаних записів графіка:" << qSchedule.lastError().text();
    }

    QSqlQuery query;
    query.prepare("DELETE FROM duty_types WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec()) {
        if (m_selectedDutyTypeId == id) setSelectedDutyTypeId(-1);
        emit dutyTypesChanged();
        return true;
    }
    qWarning() << "Помилка видалення типу наряду:" << query.lastError().text();
    return false;
}
