#ifndef STATISTICSWIDGET_H
#define STATISTICSWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>

class StatisticsWidget : public QWidget {
    Q_OBJECT
public:
    explicit StatisticsWidget(QWidget *parent = nullptr);
    virtual ~StatisticsWidget();

private slots:
    void refreshData();
    void exportToPdf();
    void exportToExcel();

private:
    QTableWidget *m_table;
    QComboBox *m_monthCombo;
    QComboBox *m_yearCombo;
    QComboBox *m_periodTypeCombo;
    
    void setupUi();
    void loadStats();
};

#endif // STATISTICSWIDGET_H
