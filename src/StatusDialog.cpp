#include "StatusDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QDate>

StatusDialog::StatusDialog(int personId, const QString &personName, QWidget *parent) 
    : QDialog(parent), m_personId(personId) {
    
    m_model = new QSqlTableModel(this);
    m_model->setTable("personnel_statuses");
    m_model->setFilter(QString("person_id = %1").arg(m_personId));
    m_model->select();

    setupUi(personName);
}

StatusDialog::~StatusDialog() {
}

void StatusDialog::setupUi(const QString &personName) {
    setWindowTitle("Статуси: " + personName);
    resize(600, 400);

    QVBoxLayout *layout = new QVBoxLayout(this);
    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->hideColumn(0);
    m_view->hideColumn(1);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    layout->addWidget(m_view);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Додати", this);
    QPushButton *delBtn = new QPushButton("Видалити", this);
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    layout->addLayout(btnLayout);

    connect(addBtn, &QPushButton::clicked, this, &StatusDialog::addStatus);
    connect(delBtn, &QPushButton::clicked, this, &StatusDialog::deleteStatus);
}

void StatusDialog::addStatus() {
    int row = m_model->rowCount();
    m_model->insertRow(row);
    m_model->setData(m_model->index(row, 1), m_personId);
    m_model->setData(m_model->index(row, 2), "Відпустка");
    m_model->setData(m_model->index(row, 3), QDate::currentDate().toString(Qt::ISODate));
    m_model->setData(m_model->index(row, 4), QDate::currentDate().addDays(7).toString(Qt::ISODate));
}

void StatusDialog::deleteStatus() {
    QModelIndexList selected = m_view->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;
    for (const QModelIndex &index : selected) {
        m_model->removeRow(index.row());
    }
    m_model->select();
}
