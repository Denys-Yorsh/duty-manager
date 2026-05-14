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
#include <QPrinter>
#include <QFileDialog>
#include <QInputDialog>
#include <QMap>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QDialog>
#include <QStyledItemDelegate>
#include <QPainter>
#include "xlsxdocument.h"
#include "xlsxformat.h"

// Делегат для графіка
class ScheduleDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyledItemDelegate::paint(painter, option, index);
    }
};

ScheduleWidget::ScheduleWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
    updateCalendar();
}

ScheduleWidget::~ScheduleWidget() {}

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
    QPushButton *changeBtn = new QPushButton("Змінити графік", this);
    QPushButton *exportExcelBtn = new QPushButton("Експорт Excel", this);
    QPushButton *exportPdfBtn = new QPushButton("Експорт PDF", this);
    
    exportExcelBtn->setStyleSheet("background-color: #217346; color: white; font-weight: bold;");
    exportPdfBtn->setStyleSheet("background-color: #d32f2f; color: white; font-weight: bold;");

    ctrlLayout->addWidget(m_monthCombo);
    ctrlLayout->addWidget(m_yearCombo);
    ctrlLayout->addWidget(genBtn);
    ctrlLayout->addWidget(changeBtn);
    ctrlLayout->addStretch();
    ctrlLayout->addWidget(exportExcelBtn);
    ctrlLayout->addWidget(exportPdfBtn);
    layout->addLayout(ctrlLayout);

    m_table = new QTableWidget(this);
    m_table->setObjectName("scheduleTable");
    m_table->setItemDelegate(new ScheduleDelegate(m_table));
    m_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    layout->addWidget(m_table);

    connect(m_monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScheduleWidget::updateCalendar);
    connect(m_yearCombo, &QComboBox::currentTextChanged, this, &ScheduleWidget::updateCalendar);
    connect(genBtn, &QPushButton::clicked, this, &ScheduleWidget::generateAutomatically);
    connect(changeBtn, &QPushButton::clicked, this, &ScheduleWidget::onChangeScheduleClicked);
    connect(exportExcelBtn, &QPushButton::clicked, this, &ScheduleWidget::exportToExcel);
    connect(exportPdfBtn, &QPushButton::clicked, this, &ScheduleWidget::exportToPdf);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &ScheduleWidget::onCellDoubleClicked);
}

void ScheduleWidget::onChangeScheduleClicked() {
    int month = m_monthCombo->currentIndex() + 1;
    int year = m_yearCombo->currentText().toInt();
    QDate firstDay(year, month, 1);

    QDialog dlg(this);
    dlg.setWindowTitle("Внести зміни");
    QVBoxLayout *vbox = new QVBoxLayout(&dlg);
    QHBoxLayout *dateLayout = new QHBoxLayout();
    dateLayout->addWidget(new QLabel("З якої дати:", &dlg));
    QSpinBox *daySpin = new QSpinBox(&dlg);
    daySpin->setRange(1, firstDay.daysInMonth());
    daySpin->setValue(1);
    dateLayout->addWidget(daySpin);
    vbox->addLayout(dateLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *cancelBtn = new QPushButton("Відміна", &dlg);
    QPushButton *okBtn = new QPushButton("Змінити", &dlg);
    btnLayout->addStretch(); btnLayout->addWidget(cancelBtn); btnLayout->addWidget(okBtn);
    vbox->addLayout(btnLayout);

    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    if (dlg.exec() == QDialog::Accepted) {
        AutoGenerator gen(month, year, daySpin->value());
        if (gen.run()) updateCalendar();
        else QMessageBox::warning(this, "Помилка", gen.lastError());
    }
}

void ScheduleWidget::updateCalendar() {
    int month = m_monthCombo->currentIndex() + 1;
    int year = m_yearCombo->currentText().toInt();
    int days = QDate(year, month, 1).daysInMonth();

    m_table->clear();
    m_table->setRowCount(0);
    m_table->setColumnCount(days + 2);

    QStringList headers;
    headers << "№ з/п" << "Назва наряду";
    for (int d = 1; d <= days; ++d) headers << QString::number(d);
    m_table->setHorizontalHeaderLabels(headers);

    for (int i = 2; i < m_table->columnCount(); ++i) {
        m_table->setColumnWidth(i, 45);
        QDate date(year, month, i - 1);
        if (date.dayOfWeek() >= 6) {
            QColor color = (date.dayOfWeek() == 6 ? QColor("#E3F2FD") : QColor("#FFEBEE"));
            if (auto item = m_table->horizontalHeaderItem(i)) item->setBackground(color);
        }
    }
    m_table->verticalHeader()->setVisible(false);

    QSqlQuery q("SELECT id, name, person_count FROM duty_types ORDER BY id");
    struct DutyInfo { int id; QString name; int count; };
    QList<DutyInfo> duties;
    while (q.next()) duties.append({q.value(0).toInt(), q.value(1).toString(), q.value(2).toInt()});

    int currentRow = 0;
    for (const auto& duty : duties) {
        for (int i = 0; i < (duty.count > 0 ? duty.count : 1); ++i) {
            m_table->insertRow(currentRow);
            m_table->setItem(currentRow, 0, new QTableWidgetItem(QString::number(currentRow + 1)));
            
            QComboBox *combo = new QComboBox(m_table);
            for (const auto &dt : duties) combo->addItem(dt.name, dt.id);
            combo->setCurrentIndex(combo->findData(duty.id));
            m_table->setCellWidget(currentRow, 1, combo);
            
            for (int d = 1; d <= days; ++d) {
                QDate date(year, month, d);
                QTableWidgetItem *item = new QTableWidgetItem("");
                if (date.dayOfWeek() == 6) item->setBackground(QColor("#E3F2FD"));
                else if (date.dayOfWeek() == 7) item->setBackground(QColor("#FFEBEE"));
                m_table->setItem(currentRow, d + 1, item);
            }
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
        int dutyTypeId = q.value(2).toInt();
        QString name = q.value(3).toString();
        for (int r = 0; r < m_table->rowCount(); ++r) {
            QComboBox *cb = qobject_cast<QComboBox*>(m_table->cellWidget(r, 1));
            if (cb && cb->currentData().toInt() == dutyTypeId) {
                QTableWidgetItem *item = m_table->item(r, date.day() + 1);
                if (item && item->text().isEmpty()) {
                    item->setText(shortenName(name));
                    item->setData(Qt::UserRole, q.value(1).toInt());
                    item->setTextAlignment(Qt::AlignCenter);
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
    for (int i = 1; i < parts.size(); ++i) result += " " + parts[i].left(1).toUpper() + ".";
    return result;
}

void ScheduleWidget::onCellDoubleClicked(int row, int column) {
    if (column < 2) return;
    int day = column - 1;
    QDate date(m_yearCombo->currentText().toInt(), m_monthCombo->currentIndex() + 1, day);
    QComboBox *cb = qobject_cast<QComboBox*>(m_table->cellWidget(row, 1));
    if (!cb) return;
    int dutyTypeId = cb->currentData().toInt();

    QSqlQuery q("SELECT id, full_name FROM personnel WHERE is_active = 'в наявності' ORDER BY full_name");
    QStringList names; QMap<QString, int> nameToId;
    names << "--- Порожньо ---";
    while (q.next()) { names << q.value(1).toString(); nameToId[q.value(1).toString()] = q.value(0).toInt(); }

    bool ok;
    QString sel = QInputDialog::getItem(this, "Призначити", "Оберіть в/c:", names, 0, false, &ok);
    if (ok) {
        QTableWidgetItem *old = m_table->item(row, column);
        if (old) {
            QSqlQuery qDel; qDel.prepare("DELETE FROM schedule WHERE duty_date = ? AND duty_type_id = ? AND person_id = ?");
            qDel.addBindValue(date.toString(Qt::ISODate)); qDel.addBindValue(dutyTypeId);
            qDel.addBindValue(old->data(Qt::UserRole).toInt()); qDel.exec();
        }
        if (sel != "--- Порожньо ---") {
            QSqlQuery qIns; qIns.prepare("INSERT INTO schedule (duty_date, person_id, duty_type_id, is_manual) VALUES (?, ?, ?, 1)");
            qIns.addBindValue(date.toString(Qt::ISODate)); qIns.addBindValue(nameToId[sel]); qIns.addBindValue(dutyTypeId); qIns.exec();
        }
        loadData();
    }
}

void ScheduleWidget::generateAutomatically() {
    AutoGenerator gen(m_monthCombo->currentIndex() + 1, m_yearCombo->currentText().toInt());
    if (gen.run()) updateCalendar();
    else QMessageBox::warning(this, "Помилка", gen.lastError());
}

void ScheduleWidget::exportToPdf() {
    QString fileName = QFileDialog::getSaveFileName(this, "Експорт PDF", "", "*.pdf");
    if (fileName.isEmpty()) return;
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageMargins(QMarginsF(15, 15, 15, 15));

    int month = m_monthCombo->currentIndex() + 1;
    int year = m_yearCombo->currentText().toInt();
    int days = m_table->columnCount() - 2;

    QString html = "<html><head><style>table { border-collapse: collapse; width: 100%; } th, td { border: 1px solid black; padding: 4px; text-align: center; font-size: 9pt; } .sat { background-color: #E3F2FD; } .sun { background-color: #FFEBEE; }</style></head><body>"
                   "<h2>Графік нарядів на " + m_monthCombo->currentText() + " " + m_yearCombo->currentText() + "</h2>"
                   "<table><thead><tr><th>Дата</th>";
    for (int r = 0; r < m_table->rowCount(); ++r) {
        QComboBox *cb = qobject_cast<QComboBox*>(m_table->cellWidget(r, 1));
        html += "<th>" + (cb ? cb->currentText() : "") + "</th>";
    }
    html += "</tr></thead><tbody>";
    for (int d = 1; d <= days; ++d) {
        QDate date(year, month, d);
        QString cls = (date.dayOfWeek() == 6 ? " class='sat'" : (date.dayOfWeek() == 7 ? " class='sun'" : ""));
        html += "<tr><td" + cls + ">" + QString::number(d) + "</td>";
        for (int r = 0; r < m_table->rowCount(); ++r) {
            QTableWidgetItem *item = m_table->item(r, d + 1);
            html += "<td" + cls + ">" + (item ? item->text() : "") + "</td>";
        }
        html += "</tr>";
    }
    html += "</tbody></table></body></html>";
    QTextDocument doc; doc.setHtml(html); doc.print(&printer);
}

void ScheduleWidget::exportToExcel() {
    QString fileName = QFileDialog::getSaveFileName(this, "Експорт Excel", "", "*.xlsx");
    if (fileName.isEmpty()) return;
    QXlsx::Document xlsx;
    int month = m_monthCombo->currentIndex() + 1;
    int year = m_yearCombo->currentText().toInt();
    int days = QDate(year, month, 1).daysInMonth();

    QXlsx::Format headerF; headerF.setFontBold(true); headerF.setPatternBackgroundColor(QColor("#D9D9D9")); headerF.setBorderStyle(QXlsx::Format::BorderThin);
    QXlsx::Format satF; satF.setPatternBackgroundColor(QColor("#E3F2FD")); satF.setBorderStyle(QXlsx::Format::BorderThin);
    QXlsx::Format sunF; sunF.setPatternBackgroundColor(QColor("#FFEBEE")); sunF.setBorderStyle(QXlsx::Format::BorderThin);
    QXlsx::Format defF; defF.setBorderStyle(QXlsx::Format::BorderThin);

    xlsx.write(1, 1, "Дата", headerF);
    for (int r = 0; r < m_table->rowCount(); ++r) {
        QComboBox *cb = qobject_cast<QComboBox*>(m_table->cellWidget(r, 1));
        xlsx.write(1, r + 2, cb ? cb->currentText() : "", headerF);
    }
    for (int d = 1; d <= days; ++d) {
        QDate date(year, month, d);
        QXlsx::Format *f = (date.dayOfWeek() == 6 ? &satF : (date.dayOfWeek() == 7 ? &sunF : &defF));
        xlsx.write(d + 1, 1, d, *f);
        for (int r = 0; r < m_table->rowCount(); ++r) {
            QTableWidgetItem *item = m_table->item(r, d + 1);
            xlsx.write(d + 1, r + 2, item ? item->text() : "", *f);
        }
    }
    xlsx.saveAs(fileName);
}
