#ifndef SCHEDULEWIDGET_H
#define SCHEDULEWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QDate>
#include <QPushButton>

class ScheduleWidget : public QWidget {
    Q_OBJECT
public:
    explicit ScheduleWidget(QWidget *parent = nullptr);

private slots:
    void updateCalendar();
    void onCellDoubleClicked(int row, int column);
    void generateAutomatically();
    void exportToPdf();

private:
    QTableWidget *m_table;
    QComboBox *m_monthCombo;
    QComboBox *m_yearCombo;
    
    void setupUi();
    void loadData();
    bool validateAssignment(int personId, const QDate &date, int dutyTypeId, QString &errorMsg);
};

#endif // SCHEDULEWIDGET_H
