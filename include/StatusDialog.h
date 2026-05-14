#ifndef STATUSDIALOG_H
#define STATUSDIALOG_H

#include <QDialog>
#include <QSqlTableModel>
#include <QTableView>

class StatusDialog : public QDialog {
    Q_OBJECT
public:
    explicit StatusDialog(int personId, const QString &personName, QWidget *parent = nullptr);
    virtual ~StatusDialog();

private slots:
    void addStatus();
    void deleteStatus();
    void saveAndSync();

private:
    int m_personId;
    QSqlTableModel *m_model;
    QTableView *m_view;
    void setupUi(const QString &personName);
    void updatePersistentEditors();
};

#endif // STATUSDIALOG_H
