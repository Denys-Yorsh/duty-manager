#include "PersonnelController.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDate>
#include <QSqlDatabase>

/**
 * @brief Конструктор PersonnelController.
 * @param parent Батьківський об'єкт QObject.
 */
PersonnelController::PersonnelController(QObject *parent) : QObject(parent) {}

/**
 * @brief Встановлює ID вибраного військовослужбовця та оновлює його ім'я.
 * @param id ID військовослужбовця.
 */
void PersonnelController::setSelectedPersonId(int id) {
    if (m_selectedPersonId != id) {
        m_selectedPersonId = id;
        m_currentSelectedPersonName = "";

        if (id != -1) {
            QSqlQuery query;
            query.prepare("SELECT name FROM personnel WHERE id = :id");
            query.bindValue(":id", id);
            if (query.exec() && query.next()) {
                m_currentSelectedPersonName = query.value("name").toString();
            }
        }

        emit selectedPersonIdChanged();
        emit currentSelectedPersonNameChanged();
    }
}

/**
 * @brief Перевіряє статуси в/с на поточну дату та оновлює поле notes в таблиці personnel.
 * @param personId ID військовослужбовця.
 */
void PersonnelController::updateCurrentStatus(int personId) {
    if (personId == -1) return;

    QDate today = QDate::currentDate();
    QString todayStr = today.toString("yyyy-MM-dd");
    
    QSqlQuery query;
    // Шукаємо статус, який охоплює сьогоднішню дату
    query.prepare("SELECT status_name FROM personnel_statuses "
                  "WHERE person_id = :pid AND :today >= start_date AND :today <= end_date "
                  "ORDER BY start_date DESC, id DESC LIMIT 1");
    query.bindValue(":pid", personId);
    query.bindValue(":today", todayStr);
    
    QString newStatus = "в наявності";
    if (query.exec() && query.next()) {
        newStatus = query.value("status_name").toString();
    }
    
    // Оновлюємо тільки якщо статус змінився
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT notes FROM personnel WHERE id = :id");
    checkQuery.bindValue(":id", personId);
    if (checkQuery.exec() && checkQuery.next()) {
        if (checkQuery.value("notes").toString() != newStatus) {
            updatePersonNotes(personId, newStatus);
        }
    }
}

/**
 * @brief Глобально оновлює статуси для всього особового складу.
 */
void PersonnelController::updateAllPersonnelStatuses() {
    QSqlQuery query("SELECT id FROM personnel");
    while (query.next()) {
        updateCurrentStatus(query.value("id").toInt());
    }
    emit personnelChanged();
}

/**
 * @brief Отримує список військовослужбовців з назвами їх звань.
 * @return Список QVariantMap з даними.
 */
QVariantList PersonnelController::getPersonnelList() {
    QVariantList list;
    QSqlQuery query("SELECT p.id, r.name as rank_name, p.name, p.position, p.notes "
                    "FROM personnel p "
                    "LEFT JOIN ranks r ON p.rank_id = r.id "
                    "ORDER BY r.priority DESC, p.name ASC");
    while (query.next()) {
        QVariantMap map;
        map["id"] = query.value("id");
        map["rankName"] = query.value("rank_name");
        map["fullName"] = query.value("name");
        map["position"] = query.value("position");
        map["notes"] = query.value("notes");
        list.append(map);
    }
    return list;
}

/**
 * @brief Отримує список звань, відсортований за пріоритетом.
 * @return Список звань.
 */
QVariantList PersonnelController::getRanks() {
    QVariantList list;
    QSqlQuery query("SELECT name FROM ranks ORDER BY priority ASC");
    while (query.next()) {
        QVariantMap map;
        map["name"] = query.value("name");
        list.append(map);
    }
    return list;
}

/**
 * @brief Додає нового військовослужбовця.
 */
bool PersonnelController::addPerson(const QString &rank, const QString &name, const QString &position, const QString &notes) {
    QSqlQuery rankQuery;
    rankQuery.prepare("SELECT id FROM ranks WHERE name = :name");
    rankQuery.bindValue(":name", rank);
    int rankId = -1;
    if (rankQuery.exec() && rankQuery.next()) {
        rankId = rankQuery.value("id").toInt();
    }

    QSqlQuery query;
    query.prepare("INSERT INTO personnel (rank_id, name, position, notes) VALUES (:rank_id, :name, :pos, :notes)");
    // Використання QMetaType для уникнення deprecated попереджень в Qt 6
    query.bindValue(":rank_id", rankId != -1 ? QVariant(rankId) : QVariant(QMetaType::fromType<int>()));
    query.bindValue(":name", name);
    query.bindValue(":pos", position);
    query.bindValue(":notes", notes.isEmpty() ? "в наявності" : notes);

    if (query.exec()) {
        emit personnelChanged();
        return true;
    }
    return false;
}

/**
 * @brief Видаляє запис військовослужбовця.
 */
bool PersonnelController::deletePerson(int id) {
    QSqlQuery query;
    query.prepare("DELETE FROM personnel WHERE id = :id");
    query.bindValue(":id", id);
    if (query.exec()) {
        if (m_selectedPersonId == id) {
            setSelectedPersonId(-1);
        }
        emit personnelChanged();
        return true;
    }
    return false;
}

/**
 * @brief Оновлює дані військовослужбовця.
 */
bool PersonnelController::updatePerson(int id, const QString &rank, const QString &name, const QString &position, const QString &notes) {
    QSqlQuery rankQuery;
    rankQuery.prepare("SELECT id FROM ranks WHERE name = :name");
    rankQuery.bindValue(":name", rank);
    int rankId = -1;
    if (rankQuery.exec() && rankQuery.next()) {
        rankId = rankQuery.value("id").toInt();
    }

    QSqlQuery query;
    query.prepare("UPDATE personnel SET rank_id = :rank_id, name = :name, position = :pos, notes = :notes WHERE id = :id");
    query.bindValue(":rank_id", rankId != -1 ? QVariant(rankId) : QVariant(QMetaType::fromType<int>()));
    query.bindValue(":name", name);
    query.bindValue(":pos", position);
    query.bindValue(":notes", notes);
    query.bindValue(":id", id);
    if (query.exec()) {
        emit personnelChanged();
        return true;
    }
    return false;
}

/**
 * @brief Оновлює поле notes (примітки/статус) для в/с.
 */
bool PersonnelController::updatePersonNotes(int id, const QString &notes) {
    QSqlQuery query;
    query.prepare("UPDATE personnel SET notes = :notes WHERE id = :id");
    query.bindValue(":notes", notes);
    query.bindValue(":id", id);
    return query.exec();
}

/**
 * @brief Отримує історію статусів для в/с.
 */
QVariantList PersonnelController::getStatuses(int personId) {
    QVariantList list;
    QSqlQuery query;
    query.prepare("SELECT id, status_name, start_date, end_date, notes FROM personnel_statuses "
                  "WHERE person_id = :pid ORDER BY start_date DESC, id DESC");
    query.bindValue(":pid", personId);
    if (query.exec()) {
        while (query.next()) {
            QVariantMap map;
            map["id"] = query.value("id");
            map["statusName"] = query.value("status_name");
            map["startDate"] = query.value("start_date");
            map["endDate"] = query.value("end_date");
            map["notes"] = query.value("notes");
            list.append(map);
        }
    }
    return list;
}

/**
 * @brief Пакетне збереження статусів військовослужбовця.
 */
bool PersonnelController::saveStatuses(int personId, const QVariantList &statusesList) {
    if (personId == -1) return false;

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    QSqlQuery deleteQuery;
    deleteQuery.prepare("DELETE FROM personnel_statuses WHERE person_id = :pid");
    deleteQuery.bindValue(":pid", personId);
    if (!deleteQuery.exec()) {
        db.rollback();
        return false;
    }

    QSqlQuery insertQuery;
    insertQuery.prepare("INSERT INTO personnel_statuses (person_id, status_name, start_date, end_date, notes) "
                        "VALUES (:pid, :status, :start, :end, :notes)");
    
    for (const QVariant &v : statusesList) {
        QVariantMap map = v.toMap();
        insertQuery.bindValue(":pid", personId);
        insertQuery.bindValue(":status", map["statusName"]);
        insertQuery.bindValue(":start", map["startDate"]);
        insertQuery.bindValue(":end", map["endDate"]);
        insertQuery.bindValue(":notes", map["notes"]);
        
        if (!insertQuery.exec()) {
            db.rollback();
            return false;
        }
    }

    if (db.commit()) {
        updateCurrentStatus(personId);
        emit personnelChanged();
        return true;
    }

    return false;
}
