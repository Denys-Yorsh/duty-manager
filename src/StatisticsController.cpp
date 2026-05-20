#include "StatisticsController.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDate>
#include <QTextDocument>
#include <QPrinter>
#include <QColor>
#include <QUrl>
#include <filesystem>

#include "xlsxdocument.h"
#include "xlsxformat.h"

namespace fs = std::filesystem;

/**
 * @brief Конструктор StatisticsController.
 */
StatisticsController::StatisticsController(QObject *parent) : QObject(parent) {
}

/**
 * @brief Збирає статистику виходів у наряд.
 */
QVariantList StatisticsController::getStatistics(int month, int year, bool isYearly) {
    QVariantList list;
    QSqlQuery q;
    
    QString sql = "SELECT p.name, r.name as rank_name, COUNT(s.id) as duty_count "
                  "FROM personnel p "
                  "LEFT JOIN ranks r ON p.rank_id = r.id "
                  "LEFT JOIN schedule s ON p.id = s.person_id ";
    
    if (isYearly) {
        sql += "AND strftime('%Y', s.duty_date) = ? ";
    } else {
        sql += "AND strftime('%m', s.duty_date) = ? AND strftime('%Y', s.duty_date) = ? ";
    }
    sql += "GROUP BY p.id ORDER BY duty_count DESC, p.name ASC";
    
    q.prepare(sql);
    if (isYearly) {
        q.addBindValue(QString::number(year));
    } else {
        q.addBindValue(QString("%1").arg(month, 2, 10, QChar('0')));
        q.addBindValue(QString::number(year));
    }

    if (q.exec()) {
        int index = 1;
        while (q.next()) {
            QVariantMap map;
            map["index"] = index++;
            map["fullName"] = q.value("name");
            map["rankName"] = q.value("rank_name");
            map["dutyCount"] = q.value("duty_count");
            list.append(map);
        }
    } else {
        qWarning() << "Помилка при розрахунку статистики:" << q.lastError().text();
    }
    return list;
}

/**
 * @brief Отримує розподіл звань у нарядах для кругової діаграми.
 */
QVariantList StatisticsController::getRankDistribution(int month, int year, bool isYearly) {
    QVariantList list;
    QSqlQuery q;
    
    QString sql = "SELECT r.name as rank_name, COUNT(s.id) as duty_count "
                  "FROM schedule s "
                  "JOIN personnel p ON s.person_id = p.id "
                  "JOIN ranks r ON p.rank_id = r.id ";
    
    if (isYearly) {
        sql += "WHERE strftime('%Y', s.duty_date) = ? ";
    } else {
        sql += "WHERE strftime('%m', s.duty_date) = ? AND strftime('%Y', s.duty_date) = ? ";
    }
    sql += "GROUP BY r.id ORDER BY duty_count DESC";
    
    q.prepare(sql);
    if (isYearly) {
        q.addBindValue(QString::number(year));
    } else {
        q.addBindValue(QString("%1").arg(month, 2, 10, QChar('0')));
        q.addBindValue(QString::number(year));
    }

    if (q.exec()) {
        double total = 0;
        QList<QPair<QString, int>> results;
        while (q.next()) {
            QString rank = q.value("rank_name").toString();
            int count = q.value("duty_count").toInt();
            results.append({rank, count});
            total += count;
        }

        for (const auto &res : results) {
            if (total > 0) {
                QVariantMap map;
                map["label"] = res.first;
                map["value"] = (double)res.second / total * 100.0;
                list.append(map);
            }
        }
    }
    return list;
}

/**
 * @brief Генерує PDF-звіт зі статистикою (вертикальний формат, чітка сітка).
 */
void StatisticsController::exportToPdf(const QVariantList &data, const QString &filePath) {
    if (filePath.isEmpty()) {
        emit exportFinished(false, "Шлях до файлу порожній");
        return;
    }

    QString localPath = filePath;
    if (filePath.startsWith("file:///")) {
        localPath = QUrl(filePath).toLocalFile();
    }

    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(localPath);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(10, 10, 10, 10), QPageLayout::Millimeter);

    QString html = "<html><head><style>"
                   "body { font-family: sans-serif; }"
                   "table { border: 1px solid #000000; width: 100%; border-spacing: 0; border-collapse: collapse; }"
                   "th, td { border: 1px solid #000000; padding: 6px; text-align: center; font-size: 11pt; }"
                   "th { background-color: #f2f2f2; font-weight: bold; font-size: 12pt; }"
                   ".high { background-color: #FFEBEE; }"
                   ".low { background-color: #E8F5E9; }"
                   "</style></head><body>"
                   "<h2 style='text-align: center;'>Статистика нарядів</h2>"
                   "<table><thead><tr>"
                   "<th>№ з/п</th><th>ПІБ</th><th>Звання</th><th>Кількість</th>"
                   "</tr></thead><tbody>";

    for (const QVariant &item : data) {
        QVariantMap map = item.toMap();
        int count = map["dutyCount"].toInt();
        QString rowClass = "";
        if (count > 8) rowClass = "high";
        else if (count > 0 && count < 3) rowClass = "low";

        html += QString("<tr class='%1'><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
                .arg(rowClass)
                .arg(map["index"].toString())
                .arg(map["fullName"].toString())
                .arg(map["rankName"].toString())
                .arg(count);
    }

    html += "</tbody></table></body></html>";

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);
    
    emit exportFinished(true, "Експорт в PDF успішно здійснений");
}

/**
 * @brief Генерує Excel-звіт зі статистикою.
 */
void StatisticsController::exportToExcel(const QVariantList &data, const QString &filePath) {
    if (filePath.isEmpty()) {
        emit exportFinished(false, "Шлях до файлу порожній");
        return;
    }

    QString localPath = filePath;
    if (filePath.startsWith("file:///")) {
        localPath = QUrl(filePath).toLocalFile();
    }

    QXlsx::Document xlsx;
    
    // Заголовок
    QXlsx::Format titleF;
    titleF.setFontBold(true);
    titleF.setFontSize(14);
    titleF.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    xlsx.mergeCells("A1:D1", titleF);
    xlsx.write(1, 1, "Статистика нарядів");

    // Шапка таблиці
    QXlsx::Format headerF;
    headerF.setFontBold(true);
    headerF.setPatternBackgroundColor(QColor("#f2f2f2"));
    headerF.setBorderStyle(QXlsx::Format::BorderThin);
    headerF.setHorizontalAlignment(QXlsx::Format::AlignHCenter);

    xlsx.write(3, 1, "№ з/п", headerF);
    xlsx.write(3, 2, "ПІБ", headerF);
    xlsx.write(3, 3, "Звання", headerF);
    xlsx.write(3, 4, "Кількість", headerF);

    int row = 4;
    int maxNameLen = 10;
    int maxRankLen = 10;

    for (const QVariant &item : data) {
        QVariantMap map = item.toMap();
        int count = map["dutyCount"].toInt();
        QString name = map["fullName"].toString();
        QString rank = map["rankName"].toString();
        
        QXlsx::Format f;
        f.setBorderStyle(QXlsx::Format::BorderThin);
        f.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
        if (count > 8) f.setPatternBackgroundColor(QColor("#FFEBEE"));
        else if (count > 0 && count < 3) f.setPatternBackgroundColor(QColor("#E8F5E9"));

        xlsx.write(row, 1, map["index"].toInt(), f);
        xlsx.write(row, 2, name, f);
        xlsx.write(row, 3, rank, f);
        xlsx.write(row, 4, count, f);

        if (name.length() > maxNameLen) maxNameLen = name.length();
        if (rank.length() > maxRankLen) maxRankLen = rank.length();
        row++;
    }

    // Автопідбір ширини
    xlsx.setColumnWidth(1, 6);
    xlsx.setColumnWidth(2, maxNameLen * 1.2 + 2);
    xlsx.setColumnWidth(3, maxRankLen * 1.2 + 2);
    xlsx.setColumnWidth(4, 10);

    if (xlsx.saveAs(localPath)) {
        emit exportFinished(true, "Експорт в Excel успішно здійснений");
    } else {
        emit exportFinished(false, "Помилка при збереженні файлу");
    }
}

/**
 * @brief Отримує роки з бази даних.
 */
QVariantList StatisticsController::getAvailableYears() {
    QVariantList years;
    QSqlQuery q("SELECT DISTINCT strftime('%Y', duty_date) as year FROM schedule ORDER BY year DESC");
    while (q.next()) years.append(q.value(0).toInt());
    return years;
}

/**
 * @brief Отримує місяці з бази даних для конкретного року.
 */
QVariantList StatisticsController::getAvailableMonths(int year) {
    QVariantList months;
    QSqlQuery q;
    q.prepare("SELECT DISTINCT strftime('%m', duty_date) as month FROM schedule "
              "WHERE strftime('%Y', duty_date) = ? ORDER BY month ASC");
    q.addBindValue(QString::number(year));
    if (q.exec()) {
        while (q.next()) months.append(q.value(0).toInt());
    }
    return months;
}
