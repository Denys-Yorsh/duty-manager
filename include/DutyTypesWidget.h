#ifndef DUTYTYPESWIDGET_H
#define DUTYTYPESWIDGET_H

#include <QWidget>
#include <QSqlRelationalTableModel>
#include <QTableView>
#include <QPushButton>

class DutyTypesWidget : public QWidget {
    Q_OBJECT
public:
    explicit DutyTypesWidget(QWidget *parent = nullptr);
    virtual ~DutyTypesWidget();

private slots:
    void addDutyType();
    void deleteDutyType();

private:
    QSqlRelationalTableModel *m_model;
    QTableView *m_view;
    void setupUi();
    void updatePersistentEditors();
};

#endif // DUTYTYPESWIDGET_H
