#include "StatisticsWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDate>
#include <QTableWidgetItem>
#include <QFileDialog>
#include <QTextDocument>
#include <QPrinter>

#include "xlsxdocument.h"
#include "xlsxformat.h"

StatisticsWidget::StatisticsWidget(QWidget *parent) : QWidget(parent) {
    setupUi();
    refreshData();
}

StatisticsWidget::~StatisticsWidget() {}

void StatisticsWidget::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *ctrlLayout = new QHBoxLayout();
    
    m_periodTypeCombo = new QComboBox(this);
    m_periodTypeCombo->addItems({"За місяць", "За рік"});
    m_periodTypeCombo->setMinimumWidth(120);

    m_monthCombo = new QComboBox(this);
    m_monthCombo->addItems({"Січень", "Лютий", "Березень", "Квітень", "Травень", "Червень", 
                          "Липень", "Серпень", "Вересень", "Жовтень", "Листопад", "Грудень"});
    m_monthCombo->setCurrentIndex(QDate::currentDate().month() - 1);
    m_monthCombo->setMinimumWidth(150);

    m_yearCombo = new QComboBox(this);
    for (int y = 2024; y <= 2030; ++y) m_yearCombo->addItem(QString::number(y));
    m_yearCombo->setCurrentText(QString::number(QDate::currentDate().year()));
    m_yearCombo->setMinimumWidth(100);

    QPushButton *refreshBtn = new QPushButton("Оновити", this);
    QPushButton *exportExcelBtn = new QPushButton("Експорт Excel", this);
    QPushButton *exportPdfBtn = new QPushButton("Експорт PDF", this);
    
    exportExcelBtn->setStyleSheet("background-color: #217346; color: white; font-weight: bold;");
    exportPdfBtn->setStyleSheet("background-color: #d32f2f; color: white; font-weight: bold;");

    ctrlLayout->addWidget(m_periodTypeCombo);
    ctrlLayout->addWidget(m_monthCombo);
    ctrlLayout->addWidget(m_yearCombo);
    ctrlLayout->addWidget(refreshBtn);
    ctrlLayout->addStretch();
    ctrlLayout->addWidget(exportExcelBtn);
    ctrlLayout->addWidget(exportPdfBtn);
    layout->addLayout(ctrlLayout);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"№ з/п", "Військовослужбовець", "Звання", "Кількість нарядів"});
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->verticalHeader()->setVisible(false);
    layout->addWidget(m_table);

    connect(m_periodTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        m_monthCombo->setEnabled(index == 0);
        refreshData();
    });
    connect(m_monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatisticsWidget::refreshData);
    connect(m_yearCombo, &QComboBox::currentTextChanged, this, &StatisticsWidget::refreshData);
    connect(refreshBtn, &QPushButton::clicked, this, &StatisticsWidget::refreshData);
    connect(exportExcelBtn, &QPushButton::clicked, this, &StatisticsWidget::exportToExcel);
    connect(exportPdfBtn, &QPushButton::clicked, this, &StatisticsWidget::exportToPdf);
}

void StatisticsWidget::refreshData() {
    m_table->setRowCount(0);
    int month = m_monthCombo->currentIndex() + 1;
    int year = m_yearCombo->currentText().toInt();
    bool isYearly = m_periodTypeCombo->currentIndex() == 1;

    QSqlQuery q;
    QString sql = "SELECT p.full_name, r.name, COUNT(s.id) as duty_count FROM personnel p "
                  "LEFT JOIN ranks r ON p.rank_id = r.id "
                  "LEFT JOIN schedule s ON p.id = s.person_id ";
    if (isYearly) sql += "AND strftime('%Y', s.duty_date) = ? ";
    else sql += "AND strftime('%m', s.duty_date) = ? AND strftime('%Y', s.duty_date) = ? ";
    sql += "GROUP BY p.id ORDER BY duty_count DESC, p.full_name ASC";
    
    q.prepare(sql);
    if (isYearly) q.addBindValue(QString::number(year));
    else { q.addBindValue(QString("%1").arg(month, 2, 10, QChar('0'))); q.addBindValue(QString::number(year)); }

    if (q.exec()) {
        int row = 0;
        while (q.next()) {
            m_table->insertRow(row);
            m_table->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
            m_table->setItem(row, 1, new QTableWidgetItem(q.value(0).toString()));
            m_table->setItem(row, 2, new QTableWidgetItem(q.value(1).toString()));
            QTableWidgetItem *item = new QTableWidgetItem(q.value(2).toString());
            item->setTextAlignment(Qt::AlignCenter);
            int count = q.value(2).toInt();
            if (count > 8) item->setBackground(QColor("#FFEBEE"));
            else if (count > 0 && count < 3) item->setBackground(QColor("#E8F5E9"));
            m_table->setItem(row, 3, item);
            row++;
        }
    }
}

void StatisticsWidget::exportToPdf() {
    QString fileName = QFileDialog::getSaveFileName(this, "Експорт PDF", "", "*.pdf");
    if (fileName.isEmpty()) return;
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    QString html = "<html><head><style>table { border-collapse: collapse; width: 100%; } th, td { border: 1px solid black; padding: 8px; text-align: center; }</style></head><body>"
                   "<h2>Статистика</h2><table><thead><tr><th>№ з/п</th><th>ПІБ</th><th>Звання</th><th>К-сть</th></tr></thead><tbody>";
    for (int i = 0; i < m_table->rowCount(); ++i) {
        int count = m_table->item(i, 3)->text().toInt();
        QString style = (count > 8 ? "background-color:#FFEBEE;" : (count > 0 && count < 3 ? "background-color:#E8F5E9;" : ""));
        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td style='%4'>%5</td></tr>")
                .arg(m_table->item(i, 0)->text()).arg(m_table->item(i, 1)->text()).arg(m_table->item(i, 2)->text()).arg(style).arg(count);
    }
    html += "</tbody></table></body></html>";
    QTextDocument doc; doc.setHtml(html); doc.print(&printer);
}

void StatisticsWidget::exportToExcel() {
    QString fileName = QFileDialog::getSaveFileName(this, "Експорт Excel", "", "*.xlsx");
    if (fileName.isEmpty()) return;
    QXlsx::Document xlsx;
    QXlsx::Format headerF; headerF.setFontBold(true); headerF.setPatternBackgroundColor(QColor("#D9D9D9")); headerF.setBorderStyle(QXlsx::Format::BorderThin);
    xlsx.write(1, 1, "№ з/п", headerF); xlsx.write(1, 2, "ПІБ", headerF); xlsx.write(1, 3, "Звання", headerF); xlsx.write(1, 4, "Кількість", headerF);
    for (int i = 0; i < m_table->rowCount(); ++i) {
        int row = i + 2;
        xlsx.write(row, 1, m_table->item(i, 0)->text().toInt());
        xlsx.write(row, 2, m_table->item(i, 1)->text());
        xlsx.write(row, 3, m_table->item(i, 2)->text());
        int count = m_table->item(i, 3)->text().toInt();
        QXlsx::Format f; f.setBorderStyle(QXlsx::Format::BorderThin);
        if (count > 8) f.setPatternBackgroundColor(QColor("#FFEBEE"));
        else if (count > 0 && count < 3) f.setPatternBackgroundColor(QColor("#E8F5E9"));
        xlsx.write(row, 4, count, f);
    }
    xlsx.saveAs(fileName);
}
