#include "ScheduleController.h"
#include "AutoGenerator.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>
#include <QVariantMap>
#include <QTextDocument>
#include <QPrinter>
#include "xlsxdocument.h"
#include "xlsxformat.h"

/**
 * @brief Конструктор ScheduleController.
 */
ScheduleController::ScheduleController(QObject *parent) : QObject(parent) {}

/**
 * @brief Отримує структуру графіка з інформацією про групування для заголовків.
 */
QVariantList ScheduleController::getScheduleStructure()
{
    QVariantList structure;
    QSqlQuery q("SELECT id, name, person_count FROM duty_types ORDER BY id");

    int globalCol = 0;
    while (q.next())
    {
        int dutyId = q.value("id").toInt();
        QString name = q.value("name").toString();
        int count = q.value("person_count").toInt();
        if (count <= 0)
            count = 1;

        for (int i = 0; i < count; ++i)
        {
            structure.append(QVariantMap{
                {"colId", globalCol++},
                {"dutyTypeId", dutyId},
                {"dutyName", name},
                {"subIndex", i},
                {"isFirstInGroup", (i == 0)},
                {"groupSize", count}});
        }
    }
    return structure;
}

/**
 * @brief Отримує всі призначення на наряди для вказаного місяця та року.
 */
QVariantList ScheduleController::getScheduleData(int month, int year)
{
    QVariantList data;
    QSqlQuery q;
    q.prepare("SELECT duty_date, person_id, duty_type_id, p.name "
              "FROM schedule s JOIN personnel p ON s.person_id = p.id "
              "WHERE strftime('%m', duty_date) = :month AND strftime('%Y', duty_date) = :year "
              "ORDER BY duty_date ASC, duty_type_id ASC, s.id ASC");
    q.bindValue(":month", QString("%1").arg(month, 2, 10, QChar('0')));
    q.bindValue(":year", QString::number(year));

    if (!q.exec())
    {
        qWarning() << "Помилка отримання даних графіка:" << q.lastError().text();
        return data;
    }

    QMap<QString, int> dayDutyCounts;

    while (q.next())
    {
        QDate date = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        int dutyId = q.value(2).toInt();
        QString key = QString("%1_%2").arg(date.toString(Qt::ISODate)).arg(dutyId);

        int subIdx = dayDutyCounts.value(key, 0);
        dayDutyCounts[key] = subIdx + 1;

        data.append(QVariantMap{
            {"day", date.day()},
            {"personId", q.value(1).toInt()},
            {"dutyTypeId", dutyId},
            {"fullName", q.value(3).toString()},
            {"shortName", shortenName(q.value(3).toString())},
            {"subIndex", subIdx}});
    }
    return data;
}

/**
 * @brief Призначає військовослужбовця на наряд або замінює існуючого.
 */
bool ScheduleController::assignPerson(int day, int month, int year, int dutyTypeId, int personId, int oldPersonId)
{
    QDate date(year, month, day);
    QString dateStr = date.toString(Qt::ISODate);

    QSqlQuery query;
    if (oldPersonId > 0)
    {
        if (personId > 0)
        {
            query.prepare("UPDATE schedule SET person_id = ?, is_manual = 1 "
                          "WHERE duty_date = ? AND duty_type_id = ? AND person_id = ?");
            query.addBindValue(personId);
            query.addBindValue(dateStr);
            query.addBindValue(dutyTypeId);
            query.addBindValue(oldPersonId);
        }
        else
        {
            query.prepare("DELETE FROM schedule WHERE duty_date = ? AND duty_type_id = ? AND person_id = ?");
            query.addBindValue(dateStr);
            query.addBindValue(dutyTypeId);
            query.addBindValue(oldPersonId);
        }
    }
    else
    {
        if (personId > 0)
        {
            query.prepare("INSERT INTO schedule (duty_date, person_id, duty_type_id, is_manual) VALUES (?, ?, ?, 1)");
            query.addBindValue(dateStr);
            query.addBindValue(personId);
            query.addBindValue(dutyTypeId);
        }
    }

    if (query.exec())
    {
        emit scheduleChanged();
        return true;
    }
    return false;
}

/**
 * @brief Отримує список в/с, які на даний момент "в наявності".
 */
QVariantList ScheduleController::getAvailablePersonnel()
{
    QVariantList list;
    QSqlQuery q("SELECT id, name FROM personnel WHERE notes = 'в наявності' ORDER BY name");
    while (q.next())
    {
        list.append(QVariantMap{
            {"id", q.value(0).toInt()},
            {"name", q.value(1).toString()}});
    }
    return list;
}

/**
 * @brief Запускає автоматичну генерацію графіка.
 */
bool ScheduleController::generateSchedule(int month, int year, int startDay)
{
    AutoGenerator gen(month, year, startDay);
    if (gen.run())
    {
        emit scheduleChanged();
        return true;
    }
    return false;
}

/**
 * @brief Скорочує ПІБ.
 */
QString ScheduleController::shortenName(const QString &fullName) const
{
    QStringList parts = fullName.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 2)
        return fullName;
    QString result = parts[0];
    for (int i = 1; i < parts.size(); ++i)
        result += " " + parts[i].left(1).toUpper() + ".";
    return result;
}

/**
 * @brief Отримує список років.
 */
QVariantList ScheduleController::getAvailableYears()
{
    QVariantList years;
    QSqlQuery q("SELECT DISTINCT strftime('%Y', duty_date) as year FROM schedule ORDER BY year DESC");
    while (q.next())
        years.append(q.value(0).toInt());
    return years;
}

/**
 * @brief Отримує список місяців.
 */
QVariantList ScheduleController::getAvailableMonths(int year)
{
    QVariantList months;
    QSqlQuery q;
    q.prepare("SELECT DISTINCT strftime('%m', duty_date) as month FROM schedule "
              "WHERE strftime('%Y', duty_date) = ? ORDER BY month ASC");
    q.addBindValue(QString::number(year));
    if (q.exec())
    {
        while (q.next())
            months.append(q.value(0).toInt());
    }
    return months;
}

/**
 * @brief Експорт графіка в PDF зі стабільною сіткою та динамічною шириною.
 */
void ScheduleController::exportToPdf(int month, int year, const QString &filePath)
{
    if (filePath.isEmpty())
    {
        emit exportFinished(false, "Шлях до файлу порожній");
        return;
    }

    QStringList monthNames = {"Січень", "Лютий", "Березень", "Квітень", "Травень", "Червень",
                              "Липень", "Серпень", "Вересень", "Жовтень", "Листопад", "Грудень"};
    QString title = QString("%1 %2").arg(monthNames[month - 1].toUpper()).arg(year);

    QVariantList structList = getScheduleStructure();
    QVariantList dataList = getScheduleData(month, year);
    int daysInMonth = QDate(year, month, 1).daysInMonth();

    // Стилі для стабільної сітки (використовуємо окремі межі для кожної клітинки)
    QString html = "<html><head><style>"
                   "body { font-family: sans-serif; }"
                   "table { border: 1px solid #000000; width: 100%; border-spacing: 0; border-collapse: collapse; }"
                   "th, td { border: 1px solid #000000; padding: 4px; text-align: center; font-size: 9pt; }"
                   ".header { background-color: #f2f2f2; font-weight: bold; font-size: 10pt; }"
                   ".weekend { background-color: #e0e0e0; }"
                   ".title { text-align: center; font-size: 16pt; font-weight: bold; margin-bottom: 15pt; }"
                   "</style></head><body>";

    html += QString("<div class='title'>%1</div>").arg(title);
    html += "<table><thead>";
    
    // Рядок заголовка 1: Дні та загальний заголовок "Назва наряду"
    html += "<tr class='header'><th>Дні</th>";
    html += QString("<th colspan='%1'>Назва наряду</th>").arg(structList.size());
    html += "</tr>";

    // Рядок заголовка 2: Конкретні назви нарядів
    html += "<tr class='header'><th></th>";
    for (int i = 0; i < structList.size(); ++i)
    {
        QVariantMap row = structList[i].toMap();
        if (row["isFirstInGroup"].toBool())
        {
            html += QString("<th colspan='%1'>%2</th>").arg(row["groupSize"].toInt()).arg(row["dutyName"].toString());
        }
    }
    html += "</tr></thead><tbody>";

    for (int d = 1; d <= daysInMonth; ++d)
    {
        QDate currentDate(year, month, d);
        bool isWeekend = (currentDate.dayOfWeek() >= 6);
        QString rowClass = isWeekend ? "weekend" : "";
        html += QString("<tr class='%1'><td><b>%2</b></td>").arg(rowClass).arg(d);

        for (const auto &sRow : structList)
        {
            QVariantMap row = sRow.toMap();
            QString person = "";
            for (const auto &dItem : dataList)
            {
                QVariantMap item = dItem.toMap();
                if (item["day"].toInt() == d && item["dutyTypeId"].toInt() == row["dutyTypeId"].toInt() && item["subIndex"].toInt() == row["subIndex"].toInt())
                {
                    person = item["shortName"].toString();
                    break;
                }
            }
            html += QString("<td>%1</td>").arg(person);
        }
        html += "</tr>";
    }
    html += "</tbody></table></body></html>";

    QTextDocument doc;
    doc.setHtml(html);

    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(8, 8, 8, 8), QPageLayout::Millimeter);

    doc.print(&printer);
    emit exportFinished(true, "Експорт в PDF успішно здійснений");
}

/**
 * @brief Експорт графіка в Excel з точним автопідбором ширини за вмістом.
 */
void ScheduleController::exportToExcel(int month, int year, const QString &filePath)
{
    if (filePath.isEmpty())
    {
        emit exportFinished(false, "Шлях до файлу порожній");
        return;
    }

    QXlsx::Document xlsx;
    QStringList monthNames = {"Січень", "Лютий", "Березень", "Квітень", "Травень", "Червень",
                              "Липень", "Серпень", "Вересень", "Жовтень", "Листопад", "Грудень"};

    QVariantList structList = getScheduleStructure();
    int totalCols = structList.size() + 1;

    // Заголовок
    QXlsx::Format titleFormat;
    titleFormat.setFontBold(true);
    titleFormat.setFontSize(16);
    titleFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    titleFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    xlsx.mergeCells(QXlsx::CellRange(1, 1, 1, totalCols), titleFormat);
    xlsx.write(1, 1, QString("%1 %2").arg(monthNames[month - 1].toUpper()).arg(year));

    // Стиль заголовків
    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setBorderStyle(QXlsx::Format::BorderThin);
    headerFormat.setPatternBackgroundColor(QColor("#f2f2f2"));
    headerFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    headerFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    // Рядок заголовка 1: Дні та загальний заголовок "Назва наряду"
    xlsx.write(3, 1, "Дні", headerFormat);
    if (totalCols > 1) {
        xlsx.mergeCells(QXlsx::CellRange(3, 2, 3, totalCols), headerFormat);
        xlsx.write(3, 2, "Назва наряду", headerFormat);
    }
    xlsx.setColumnWidth(1, 5); // Дуже вузька колонка для днів

    // Вектор для розрахунку ширини (ініціалізуємо довжиною назви наряду в заголовку)
    QVector<double> colWidths(totalCols, 0);

    // Рядок заголовка 2: Конкретні назви нарядів
    xlsx.write(4, 1, "", headerFormat); // Порожня клітинка під "Дні"
    for (int i = 0; i < structList.size(); ++i)
    {
        QVariantMap row = structList[i].toMap();
        int colIdx = i + 2;
        if (row["isFirstInGroup"].toBool())
        {
            int groupSize = row["groupSize"].toInt();
            if (groupSize > 1)
            {
                xlsx.mergeCells(QXlsx::CellRange(4, colIdx, 4, colIdx + groupSize - 1), headerFormat);
            }
            xlsx.write(4, colIdx, row["dutyName"].toString(), headerFormat);

            // Розрахунок базової ширини від заголовка (розділено на кількість людей у наряді)
            double baseWidth = (double)row["dutyName"].toString().length() / groupSize;
            for (int g = 0; g < groupSize; ++g)
                colWidths[i + g + 1] = baseWidth;
        }
    }

    QVariantList dataList = getScheduleData(month, year);
    int daysInMonth = QDate(year, month, 1).daysInMonth();

    QXlsx::Format cellFormat;
    cellFormat.setBorderStyle(QXlsx::Format::BorderThin);
    cellFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);

    QXlsx::Format weekendFormat;
    weekendFormat.setBorderStyle(QXlsx::Format::BorderThin);
    weekendFormat.setPatternBackgroundColor(QColor("#e0e0e0"));
    weekendFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);

    for (int d = 1; d <= daysInMonth; ++d)
    {
        QDate currentDate(year, month, d);
        bool isWeekend = (currentDate.dayOfWeek() >= 6);
        QXlsx::Format &currentFmt = isWeekend ? weekendFormat : cellFormat;

        xlsx.write(4 + d, 1, d, currentFmt);
        for (int i = 0; i < structList.size(); ++i)
        {
            QVariantMap row = structList[i].toMap();
            QString person = "";
            for (const auto &dItem : dataList)
            {
                QVariantMap item = dItem.toMap();
                if (item["day"].toInt() == d && item["dutyTypeId"].toInt() == row["dutyTypeId"].toInt() && item["subIndex"].toInt() == row["subIndex"].toInt())
                {
                    person = item["shortName"].toString();
                    break;
                }
            }
            xlsx.write(4 + d, i + 2, person, currentFmt);

            // Оновлюємо максимальну ширину для стовпця за вмістом ПІБ
            if (person.length() > colWidths[i + 1])
                colWidths[i + 1] = person.length();
        }
    }

    // Застосовуємо розраховану ширину
    for (int i = 1; i < totalCols; ++i)
    {
        // Коефіцієнт 1.0 (пряма кількість символів) + мінімальний запас 0.5 для стандартного шрифту
        double finalWidth = colWidths[i] + 1;
        if (finalWidth < 5)
            finalWidth = 5;
        xlsx.setColumnWidth(i + 1, finalWidth);
    }

    if (xlsx.saveAs(filePath))
    {
        emit exportFinished(true, "Експорт в Excel успішно здійснений");
    }
    else
    {
        emit exportFinished(false, "Помилка при збереженні файлу");
    }
}
