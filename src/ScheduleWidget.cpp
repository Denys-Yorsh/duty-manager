#include "ScheduleWidget.h"
#include "AutoGenerator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>
#include <QDate>
#include <QTableWidgetItem>
#include <QTextDocument>
#include <QFile>
#include <QTextStream>
#include <QPrinter>
#include <QFileDialog>
#include <QInputDialog>
#include <QMap>

ScheduleWidget::ScheduleWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
    updateCalendar();
}

ScheduleWidget::~ScheduleWidget() {
}

void ScheduleWidget::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *ctrlLayout = new QHBoxLayout();
    
    m_monthCombo = new QComboBox(this);
    m_monthCombo->addItems({"Січень", "Лютий", "Березень", "Квітень", "Травень", "Червень", 
                          "Липень", "Серпень", "Вересень", "Жовтень", "Листопад", "Грудень"});
    m_monthCombo->setCurrentIndex(QDate::currentDate().month() - 1);
    m_monthCombo->setMinimumWidth(150);

    m_yearCombo = new QComboBox(this);
    for (int y = 2024; y <= 2030; ++y) m_yearCombo->addItem(QString::number(y));
    m_yearCombo->setCurrentText(QString::number(QDate::currentDate().year()));
    m_yearCombo->setMinimumWidth(100);

    QPushButton *genBtn = new QPushButton("Авто-генерація", this);
    genBtn->setMinimumWidth(180);
    QPushButton *exportExcelBtn = new QPushButton("Експорт Excel", this);
    exportExcelBtn->setMinimumWidth(180);
    QPushButton *exportPdfBtn = new QPushButton("Експорт PDF", this);
    exportPdfBtn->setMinimumWidth(180);
    
    exportExcelBtn->setStyleSheet("background-color: #217346; color: white; font-weight: bold;");
    exportPdfBtn->setStyleSheet("background-color: #d32f2f; color: white; font-weight: bold;");

    ctrlLayout->addWidget(m_monthCombo);
    ctrlLayout->addWidget(m_yearCombo);
    ctrlLayout->addWidget(genBtn);
    ctrlLayout->addStretch();
    ctrlLayout->addWidget(exportExcelBtn);
    ctrlLayout->addWidget(exportPdfBtn);
    layout->addLayout(ctrlLayout);

    m_table = new QTableWidget(this);
    m_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    layout->addWidget(m_table);

    connect(m_monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScheduleWidget::updateCalendar);
    connect(m_yearCombo, &QComboBox::currentTextChanged, this, &ScheduleWidget::updateCalendar);
    connect(genBtn, &QPushButton::clicked, this, &ScheduleWidget::generateAutomatically);
    connect(exportExcelBtn, &QPushButton::clicked, this, &ScheduleWidget::exportToExcel);
    connect(exportPdfBtn, &QPushButton::clicked, this, &ScheduleWidget::exportToPdf);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &ScheduleWidget::onCellDoubleClicked);
}

void ScheduleWidget::updateCalendar() {
    int month = m_monthCombo->currentIndex() + 1;
    int year = m_yearCombo->currentText().toInt();
    QDate firstDay(year, month, 1);
    int daysInMonth = firstDay.daysInMonth();

    m_table->clear();
    m_table->setRowCount(0);
    m_table->setColumnCount(daysInMonth + 2);

    QStringList headers;
    headers << "№ з/п" << "Назва наряду";
    for (int d = 1; d <= daysInMonth; ++d) headers << QString::number(d);
    m_table->setHorizontalHeaderLabels(headers);

    m_table->setColumnWidth(0, 50);
    m_table->setColumnWidth(1, 180);
    for (int i = 2; i < m_table->columnCount(); ++i) m_table->setColumnWidth(i, 110);
    
    m_table->verticalHeader()->setVisible(false);

    // Cache duty types to avoid multiple SQL queries in loop
    struct DutyType { int id; QString name; };
    QList<DutyType> cachedTypes;
    QSqlQuery qTypes("SELECT id, name FROM duty_types ORDER BY id");
    while (qTypes.next()) {
        cachedTypes.append({qTypes.value(0).toInt(), qTypes.value(1).toString()});
    }

    QSqlQuery q("SELECT id, name, person_count FROM duty_types ORDER BY id");
    int currentRow = 0;
    while (q.next()) {
        int dutyTypeId = q.value(0).toInt();
        QString dutyName = q.value(1).toString();
        int count = q.value(2).toInt();
        if (count <= 0) count = 1;

        for (int i = 0; i < count; ++i) {
            m_table->insertRow(currentRow);
            QTableWidgetItem *numItem = new QTableWidgetItem(QString::number(currentRow + 1));
            numItem->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(currentRow, 0, numItem);

            QComboBox *combo = new QComboBox(m_table);
            for (const auto &dt : cachedTypes) {
                combo->addItem(dt.name, dt.id);
            }
            combo->setCurrentText(dutyName);
            m_table->setCellWidget(currentRow, 1, combo);
            currentRow++;
        }
    }
    loadData();
}

void ScheduleWidget::loadData() {
    int month = m_monthCombo->currentIndex() + 1;
    int year = m_yearCombo->currentText().toInt();
    QSqlQuery q;
    q.prepare("SELECT duty_date, person_id, duty_type_id, p.full_name "
              "FROM schedule s JOIN personnel p ON s.person_id = p.id "
              "WHERE strftime('%m', duty_date) = ? AND strftime('%Y', duty_date) = ?");
    q.addBindValue(QString("%1").arg(month, 2, 10, QChar('0')));
    q.addBindValue(QString::number(year));
    if (!q.exec()) return;

    while (q.next()) {
        QDate date = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        int personId = q.value(1).toInt();
        int dutyTypeId = q.value(2).toInt();
        QString personName = q.value(3).toString();
        int dayCol = date.day() + 1;
        
        for (int r = 0; r < m_table->rowCount(); ++r) {
            QComboBox *cb = qobject_cast<QComboBox*>(m_table->cellWidget(r, 1));
            if (cb && cb->currentData().toInt() == dutyTypeId) {
                if (!m_table->item(r, dayCol)) {
                    QTableWidgetItem *item = new QTableWidgetItem(shortenName(personName));
                    item->setData(Qt::UserRole, personId);
                    item->setTextAlignment(Qt::AlignCenter);
                    m_table->setItem(r, dayCol, item);
                    break;
                }
            }
        }
    }
    
    m_table->resizeColumnsToContents();
    if (m_table->columnWidth(0) < 50) m_table->setColumnWidth(0, 50);
    if (m_table->columnWidth(1) < 180) m_table->setColumnWidth(1, 180);
}

QString ScheduleWidget::shortenName(const QString &fullName) const {
    QStringList parts = fullName.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 2) return fullName;
    
    QString result = parts[0];
    for (int i = 1; i < parts.size(); ++i) {
        if (!parts[i].isEmpty()) {
            result += " " + parts[i].left(1).toUpper() + ".";
        }
    }
    return result;
}

void ScheduleWidget::onCellDoubleClicked(int row, int column) {
    if (column < 2) return;

    int month = m_monthCombo->currentIndex() + 1;
    int year = m_yearCombo->currentText().toInt();
    int day = column - 1;
    QDate date(year, month, day);

    QComboBox *cb = qobject_cast<QComboBox*>(m_table->cellWidget(row, 1));
    if (!cb) return;
    int dutyTypeId = cb->currentData().toInt();

    QSqlQuery q("SELECT id, full_name FROM personnel WHERE is_active = 'в наявності' ORDER BY full_name");
    QStringList names;
    QMap<QString, int> nameToId;
    names << "--- Порожньо ---";
    
    while (q.next()) {
        QString name = q.value(1).toString();
        names << name;
        nameToId[name] = q.value(0).toInt();
    }

    bool ok;
    QString selectedName = QInputDialog::getItem(this, "Призначити наряд", 
                                                QString("Дата: %1\nНаряд: %2").arg(date.toString("dd.MM.yyyy")).arg(cb->currentText()), 
                                                names, 0, false, &ok);
    
    if (ok) {
        QTableWidgetItem *oldItem = m_table->item(row, column);
        if (oldItem) {
            int oldPersonId = oldItem->data(Qt::UserRole).toInt();
            QSqlQuery qDel;
            qDel.prepare("DELETE FROM schedule WHERE duty_date = ? AND duty_type_id = ? AND person_id = ?");
            qDel.addBindValue(date.toString(Qt::ISODate));
            qDel.addBindValue(dutyTypeId);
            qDel.addBindValue(oldPersonId);
            qDel.exec();
        }

        if (selectedName != "--- Порожньо ---") {
            int personId = nameToId[selectedName];
            QSqlQuery qIns;
            qIns.prepare("INSERT INTO schedule (duty_date, person_id, duty_type_id, is_manual) VALUES (?, ?, ?, 1)");
            qIns.addBindValue(date.toString(Qt::ISODate));
            qIns.addBindValue(personId);
            qIns.addBindValue(dutyTypeId);
            qIns.exec();
        }
        
        loadData();
    }
}

void ScheduleWidget::generateAutomatically() {
    AutoGenerator gen(m_monthCombo->currentIndex() + 1, m_yearCombo->currentText().toInt());
    if (gen.run()) {
        updateCalendar();
        QMessageBox::information(this, "Успіх", "Графік успішно згенеровано та збережено в базу даних.");
    } else {
        QMessageBox::warning(this, "Помилка", gen.lastError());
    }
}

void ScheduleWidget::exportToPdf() {
    QString fileName = QFileDialog::getSaveFileName(this, "Експорт PDF", "", "*.pdf");
    if (fileName.isEmpty()) return;

    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(15, 15, 15, 15));

    int daysInMonth = m_table->columnCount() - 2;

    QString html = "<html><head><style>"
                   "table { border-collapse: collapse; width: 100%; border: 1px solid black; }"
                   "th { border: 1px solid black; background-color: #f2f2f2; font-weight: bold; padding: 4px; text-align: center; font-size: 9pt; }"
                   "td { border: 1px solid black; padding: 4px; text-align: center; font-size: 9pt; }"
                   "h2 { text-align: center; }"
                   "</style></head><body>"
                   "<h2>Графік нарядів на " + m_monthCombo->currentText() + " " + m_yearCombo->currentText() + "</h2>"
                   "<table cellspacing='0' cellpadding='4'><thead><tr>"
                   "<th style='width: 40px;'>Дата</th>";

    for (int r = 0; r < m_table->rowCount(); ++r) {
        QString dutyName = "";
        QComboBox *cb = qobject_cast<QComboBox*>(m_table->cellWidget(r, 1));
        if (cb) dutyName = cb->currentText();
        html += "<th>" + dutyName + "</th>";
    }
    html += "</tr></thead><tbody>";

    for (int d = 1; d <= daysInMonth; ++d) {
        html += "<tr>";
        html += "<td style='border: 1px solid black; background-color: #f9f9f9; font-weight: bold;'>" + QString::number(d) + "</td>";
        
        for (int r = 0; r < m_table->rowCount(); ++r) {
            QString personName = "";
            QTableWidgetItem *item = m_table->item(r, d + 1);
            if (item) personName = item->text();
            html += "<td style='border: 1px solid black;'>" + personName + "</td>";
        }
        html += "</tr>";
    }

    html += "</tbody></table></body></html>";

    QTextDocument doc;
    doc.setHtml(html);
    doc.setPageSize(printer.pageRect(QPrinter::DevicePixel).size());
    doc.print(&printer);
    
    QMessageBox::information(this, "Експорт", "PDF успішно збережено.");
}

void ScheduleWidget::exportToExcel() {
    QString fileName = QFileDialog::getSaveFileName(this, "Експорт Excel", "", "*.xls");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);
    out.setGenerateByteOrderMark(true);

    int daysInMonth = m_table->columnCount() - 2;

    out << "<html><head><meta charset='UTF-8'><style>"
        << "table { border: 1px solid #000; border-collapse: collapse; table-layout: auto; }"
        << "th, td { border: 1px solid #000; padding: 6px; text-align: center; font-size: 10pt; white-space: nowrap; }"
        << "th { background-color: #d9d9d9; font-weight: bold; }"
        << "h2 { text-align: center; }"
        << "</style></head><body>"
        << "<h2>Графік нарядів на " << m_monthCombo->currentText() << " " << m_yearCombo->currentText() << "</h2>"
        << "<table><thead><tr>"
        << "<th style='min-width: 50px;'>Дата</th>";

    for (int r = 0; r < m_table->rowCount(); ++r) {
        QString dutyName = "";
        QComboBox *cb = qobject_cast<QComboBox*>(m_table->cellWidget(r, 1));
        if (cb) dutyName = cb->currentText();
        out << "<th>" << dutyName << "</th>";
    }
    out << "</tr></thead><tbody>";

    for (int d = 1; d <= daysInMonth; ++d) {
        out << "<tr>";
        out << "<td style='background-color: #f9f9f9; font-weight: bold;'>" << QString::number(d) << "</td>";
        
        for (int r = 0; r < m_table->rowCount(); ++r) {
            QString personName = "";
            QTableWidgetItem *item = m_table->item(r, d + 1);
            if (item) personName = item->text();
            out << "<td>" << personName << "</td>";
        }
        out << "</tr>";
    }

    out << "</tbody></table></body></html>";
    file.close();

    QMessageBox::information(this, "Експорт", "Дані успішно експортовано у формат .xls");
}

bool ScheduleWidget::validateAssignment(int, const QDate&, int, QString&) { return true; }
