#ifndef PERSONNELWIDGET_H
#define PERSONNELWIDGET_H

#include <QWidget>
#include <QSqlRelationalTableModel>
#include <QTableView>

class PersonnelWidget : public QWidget {
    Q_OBJECT
public:
    explicit PersonnelWidget(QWidget *parent = nullptr);
    virtual ~PersonnelWidget();

private slots:
    void addPerson();
    void deletePerson();
    void manageStatuses();

private:
    QSqlRelationalTableModel *m_model;
    QTableView *m_view;
    void setupUi();
    void updatePersistentEditors();
};

#endif // PERSONNELWIDGET_H
