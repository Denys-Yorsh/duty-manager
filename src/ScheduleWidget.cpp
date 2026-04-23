#include "ScheduleWidget.h"
#include "AutoGenerator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QMap>
#include <QPrinter>
#include <QPainter>
#include <QFileDialog>

ScheduleWidget::ScheduleWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
    updateCalendar();
}

void ScheduleWidget::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *ctrlLayout = new QHBoxLayout();
    
    m_monthCombo = new QComboBox(this);
    QStringList months = {"Січень", "Лютий", "Березень", "Квітень", "Травень", "Червень", 
                          "Липень", "Серпень", "Вересень", "Жовтень", "Листопад", "Грудень"};
    m_monthCombo->addItems(months);
    m_monthCombo->setCurrentIndex(QDate::currentDate().month() - 1);

    m_yearCombo = new QComboBox(this);
    for (int y = 2024; y <= 2030; ++y) m_yearCombo->addItem(QString::number(y));
    m_yearCombo->setCurrentText(QString::number(QDate::currentDate().year()));

    QPushButton *genBtn = new QPushButton("Авто-генерація", this);
    genBtn->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold; padding: 5px;");

    QPushButton *exportBtn = new QPushButton("Експорт PDF", this);
    exportBtn->setStyleSheet("background-color: #2196F3; color: white; font-weight: bold; padding: 5px;");

    ctrlLayout->addWidget(new QLabel("Місяць:"));
    ctrlLayout->addWidget(m_monthCombo);
    ctrlLayout->addWidget(new QLabel("Рік:"));
    ctrlLayout->addWidget(m_yearCombo);
    ctrlLayout->addStretch();
    ctrlLayout->addWidget(genBtn);
    ctrlLayout->addWidget(exportBtn);

    layout->addLayout(ctrlLayout);

    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers); // Заборона прямого редагування тексту в клітинках
    layout->addWidget(m_table);

    connect(m_monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScheduleWidget::updateCalendar);
    connect(m_yearCombo, &QComboBox::currentTextChanged, this, &ScheduleWidget::updateCalendar);
    connect(genBtn, &QPushButton::clicked, this, &ScheduleWidget::generateAutomatically);
    connect(exportBtn, &QPushButton::clicked, this, &ScheduleWidget::exportToPdf);
}

void ScheduleWidget::exportToPdf() {
    QString fileName = QFileDialog::getSaveFileName(this, "Зберегти графік", "", "PDF Files (*.pdf)");
    if (fileName.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setPageMargins(QMarginsF(10, 10, 10, 10));

    QPainter painter(&printer);
    // Розрахунок масштабу для втиснення таблиці на сторінку
    double xscale = printer.pageRect(QPrinter::DevicePixel).width() / double(m_table->width());
    double yscale = printer.pageRect(QPrinter::DevicePixel).height() / double(m_table->height());
    double scale = qMin(xscale, yscale);
    
    painter.scale(scale, scale);
    m_table->render(&painter);
}

void ScheduleWidget::updateCalendar() {
    int month = m_monthCombo->currentIndex() + 1;
    int year = m_yearCombo->currentText().toInt();
    QDate firstDay(year, month, 1);
    int daysInMonth = firstDay.daysInMonth();

    QList<QStringList> dutyTypes;
    QSqlQuery q("SELECT id, name, abbr FROM duty_types ORDER BY id");
    while (q.next()) {
        dutyTypes.append({q.value(0).toString(), q.value(1).toString(), q.value(2).toString()});
    }

    m_table->clear();
    m_table->setRowCount(dutyTypes.size());
    m_table->setColumnCount(daysInMonth);

    QStringList headers;
    for (int d = 1; d <= daysInMonth; ++d) {
        QDate date(year, month, d);
        QString dayName = (date.dayOfWeek() >= 6) ? " (в)" : "";
        headers << QString::number(d) + dayName;
    }
    m_table->setHorizontalHeaderLabels(headers);

    QStringList rowHeaders;
    for (const auto& dt : dutyTypes) {
        rowHeaders << dt[1];
    }
    m_table->setVerticalHeaderLabels(rowHeaders);
    
    m_table->horizontalHeader()->setDefaultSectionSize(65);

    loadData();
}

void ScheduleWidget::loadData() {
    int month = m_monthCombo->currentIndex() + 1;
    int year = m_yearCombo->currentText().toInt();
    
    QSqlQuery q;
    q.prepare("SELECT duty_date, person_id, duty_type_id, p.full_name "
              "FROM schedule s "
              "JOIN personnel p ON s.person_id = p.id "
              "WHERE strftime('%m', duty_date) = ? AND strftime('%Y', duty_date) = ?");
    q.addBindValue(QString("%1").arg(month, 2, 10, QChar('0')));
    q.addBindValue(QString::number(year));

    if (!q.exec()) return;

    QMap<int, int> dutyTypeToRow;
    QSqlQuery qRows("SELECT id FROM duty_types ORDER BY id");
    int rowIdx = 0;
    while (qRows.next()) {
        dutyTypeToRow[qRows.value(0).toInt()] = rowIdx++;
    }

    while (q.next()) {
        QDate date = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        int dutyTypeId = q.value(2).toInt();
        QString name = q.value(3).toString();

        int col = date.day() - 1;
        int row = dutyTypeToRow.value(dutyTypeId, -1);

        if (row != -1) {
            QTableWidgetItem *item = new QTableWidgetItem(name);
            item->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(row, col, item);
        }
    }
}

void ScheduleWidget::generateAutomatically() {
    int month = m_monthCombo->currentIndex() + 1;
    int year = m_yearCombo->currentText().toInt();

    if (QMessageBox::question(this, "Генерація", "Автоматично заповнити графік на цей місяць?") == QMessageBox::Yes) {
        AutoGenerator gen(month, year);
        if (gen.run()) {
            updateCalendar();
            QMessageBox::information(this, "Готово", "Графік успішно сформовано.");
        } else {
            QMessageBox::warning(this, "Помилка", "Не вдалося згенерувати графік: " + gen.lastError());
        }
    }
}

void ScheduleWidget::onCellDoubleClicked(int row, int column) {
    // В майбутньому: відкриття діалогу вибору конкретного бійця
}

bool ScheduleWidget::validateAssignment(int personId, const QDate &date, int dutyTypeId, QString &errorMsg) {
    return true; 
}
