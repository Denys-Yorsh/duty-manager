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

    // Нижня панель кнопок
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Добавити графік", this);
    QPushButton *delBtn = new QPushButton("Видалити графік", this);
    bottomLayout->addWidget(addBtn);
    bottomLayout->addWidget(delBtn);
    bottomLayout->addStretch();
    layout->addLayout(bottomLayout);

    connect(m_monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScheduleWidget::updateCalendar);
    connect(m_yearCombo, &QComboBox::currentTextChanged, this, &ScheduleWidget::updateCalendar);
    connect(genBtn, &QPushButton::clicked, this, &ScheduleWidget::generateAutomatically);
    connect(exportBtn, &QPushButton::clicked, this, &ScheduleWidget::exportToPdf);
    connect(addBtn, &QPushButton::clicked, this, &ScheduleWidget::addScheduleRow);
    connect(delBtn, &QPushButton::clicked, this, &ScheduleWidget::deleteScheduleRow);
}

void ScheduleWidget::updateCalendar() {
    int month = m_monthCombo->currentIndex() + 1;
    int year = m_yearCombo->currentText().toInt();
    QDate firstDay(year, month, 1);
    int daysInMonth = firstDay.daysInMonth();

    m_table->clear();
    // 2 службові колонки + дні місяця
    m_table->setColumnCount(daysInMonth + 2);

    QStringList headers;
    headers << "№ з/п" << "Назва наряду";
    for (int d = 1; d <= daysInMonth; ++d) headers << QString::number(d);
    m_table->setHorizontalHeaderLabels(headers);

    m_table->setColumnWidth(0, 60);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->setColumnWidth(1, 150);
    
    m_table->verticalHeader()->setVisible(false);

    // Автоматично додаємо існуючі типи нарядів як рядки
    QSqlQuery q("SELECT id, name FROM duty_types ORDER BY id");
    int row = 0;
    while (q.next()) {
        m_table->insertRow(row);
        
        // Номер
        QTableWidgetItem *numItem = new QTableWidgetItem(QString::number(row + 1));
        numItem->setFlags(numItem->flags() & ~Qt::ItemIsEditable);
        numItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 0, numItem);

        // Назва наряду (випадаючий список)
        QComboBox *combo = new QComboBox(m_table);
        QSqlQuery qTypes("SELECT id, name FROM duty_types ORDER BY id");
        while (qTypes.next()) {
            combo->addItem(qTypes.value(1).toString(), qTypes.value(0).toInt());
        }
        combo->setCurrentText(q.value(1).toString());
        m_table->setCellWidget(row, 1, combo);
        
        row++;
    }
    
    loadData();
}

void ScheduleWidget::addScheduleRow() {
    int row = m_table->rowCount();
    m_table->insertRow(row);

    // Номер
    QTableWidgetItem *numItem = new QTableWidgetItem(QString::number(row + 1));
    numItem->setFlags(numItem->flags() & ~Qt::ItemIsEditable);
    numItem->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(row, 0, numItem);

    // Назва наряду
    QComboBox *combo = new QComboBox(m_table);
    QSqlQuery qTypes("SELECT id, name FROM duty_types ORDER BY id");
    while (qTypes.next()) {
        combo->addItem(qTypes.value(1).toString(), qTypes.value(0).toInt());
    }
    m_table->setCellWidget(row, 1, combo);
}

void ScheduleWidget::deleteScheduleRow() {
    int row = m_table->currentRow();
    if (row >= 0) {
        m_table->removeRow(row);
        // Перенумеровуємо
        for (int i = 0; i < m_table->rowCount(); ++i) {
            if (m_table->item(i, 0)) {
                m_table->item(i, 0)->setText(QString::number(i + 1));
            }
        }
    }
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

    // Складніша логіка для відповідності типу наряду в рядку
    while (q.next()) {
        QDate date = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        int dutyTypeId = q.value(2).toInt();
        QString personName = q.value(3).toString();
        
        // Шукаємо перший підходящий рядок з таким типом наряду
        for (int r = 0; r < m_table->rowCount(); ++r) {
            QComboBox *cb = qobject_cast<QComboBox*>(m_table->cellWidget(r, 1));
            if (cb && cb->currentData().toInt() == dutyTypeId) {
                // Перевіряємо, чи клітинка вільна (проста логіка для демонстрації)
                if (!m_table->item(r, date.day() + 1)) {
                    m_table->setItem(r, date.day() + 1, new QTableWidgetItem(personName));
                    break;
                }
            }
        }
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
