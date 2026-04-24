#include "ScheduleWidget.h"
#include "AutoGenerator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QLabel>
#include <QMessageBox>
#include <QMap>
#include <QPrinter>
#include <QPainter>
#include <QFileDialog>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>
#include <QDate>
#include <QTableWidgetItem>

ScheduleWidget::ScheduleWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
    updateCalendar();
}

ScheduleWidget::~ScheduleWidget() {
    // Реалізація деструктора
}

void ScheduleWidget::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *ctrlLayout = new QHBoxLayout();
    
    m_monthCombo = new QComboBox(this);
    m_monthCombo->addItems({"Січень", "Лютий", "Березень", "Квітень", "Травень", "Червень", 
                          "Липень", "Серпень", "Вересень", "Жовтень", "Листопад", "Грудень"});
    m_monthCombo->setCurrentIndex(QDate::currentDate().month() - 1);

    m_yearCombo = new QComboBox(this);
    for (int y = 2024; y <= 2030; ++y) m_yearCombo->addItem(QString::number(y));
    m_yearCombo->setCurrentText(QString::number(QDate::currentDate().year()));

    QPushButton *genBtn = new QPushButton("Авто-генерація", this);
    QPushButton *exportBtn = new QPushButton("Експорт PDF", this);

    ctrlLayout->addWidget(m_monthCombo);
    ctrlLayout->addWidget(m_yearCombo);
    ctrlLayout->addWidget(genBtn);
    ctrlLayout->addWidget(exportBtn);
    layout->addLayout(ctrlLayout);

    m_table = new QTableWidget(this);
    layout->addWidget(m_table);

    connect(m_monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScheduleWidget::updateCalendar);
    connect(m_yearCombo, &QComboBox::currentTextChanged, this, &ScheduleWidget::updateCalendar);
    connect(genBtn, &QPushButton::clicked, this, &ScheduleWidget::generateAutomatically);
    connect(exportBtn, &QPushButton::clicked, this, &ScheduleWidget::exportToPdf);
}

void ScheduleWidget::updateCalendar() {
    int month = m_monthCombo->currentIndex() + 1;
    int year = m_yearCombo->currentText().toInt();
    QDate firstDay(year, month, 1);
    int daysInMonth = firstDay.daysInMonth();

    QList<QStringList> dutyTypes;
    QSqlQuery q("SELECT id, name FROM duty_types ORDER BY id");
    while (q.next()) dutyTypes.append({q.value(0).toString(), q.value(1).toString()});

    m_table->clear();
    m_table->setRowCount(dutyTypes.size());
    m_table->setColumnCount(daysInMonth);

    QStringList headers;
    for (int d = 1; d <= daysInMonth; ++d) headers << QString::number(d);
    m_table->setHorizontalHeaderLabels(headers);

    QStringList rowHeaders;
    for (const auto& dt : dutyTypes) rowHeaders << dt[1];
    m_table->setVerticalHeaderLabels(rowHeaders);
    
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

    QMap<int, int> dutyTypeToRow;
    QSqlQuery qRows("SELECT id FROM duty_types ORDER BY id");
    int rowIdx = 0;
    while (qRows.next()) dutyTypeToRow[qRows.value(0).toInt()] = rowIdx++;

    while (q.next()) {
        QDate date = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        int row = dutyTypeToRow.value(q.value(2).toInt(), -1);
        if (row != -1) m_table->setItem(row, date.day() - 1, new QTableWidgetItem(q.value(3).toString()));
    }
}

void ScheduleWidget::generateAutomatically() {
    AutoGenerator gen(m_monthCombo->currentIndex() + 1, m_yearCombo->currentText().toInt());
    if (gen.run()) updateCalendar();
}

void ScheduleWidget::exportToPdf() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save PDF", "", "*.pdf");
    if (fileName.isEmpty()) return;
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageOrientation(QPageLayout::Landscape);
    QPainter painter(&printer);
    m_table->render(&painter);
}

void ScheduleWidget::onCellDoubleClicked(int, int) {}
bool ScheduleWidget::validateAssignment(int, const QDate&, int, QString&) { return true; }
