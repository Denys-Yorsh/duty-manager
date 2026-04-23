#ifndef PERSONNELWIDGET_H
#define PERSONNELWIDGET_H

#include <QWidget>
#include <QSqlRelationalTableModel>
#include <QTableView>
#include <QPushButton>

class PersonnelWidget : public QWidget {
    Q_OBJECT
public:
    explicit PersonnelWidget(QWidget *parent = nullptr);

private slots:
    void addPerson();
    void deletePerson();
    void manageStatuses();

private:
    QSqlRelationalTableModel *m_model;
    QTableView *m_view;
    void setupUi();
};

#endif // PERSONNELWIDGET_H
