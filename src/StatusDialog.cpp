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
    m_model->setEditStrategy(QSqlTableModel::OnFieldChange);
    
    m_model->setHeaderData(2, Qt::Horizontal, "Тип статусу");
    m_model->setHeaderData(3, Qt::Horizontal, "Початок");
    m_model->setHeaderData(4, Qt::Horizontal, "Кінець");
    m_model->setHeaderData(5, Qt::Horizontal, "Коментар");
    
    m_model->select();

    setupUi(personName);
}

void StatusDialog::setupUi(const QString &personName) {
    setWindowTitle("Статуси: " + personName);
    resize(600, 400);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QString("Управління статусами (відпустки, лікарняні) для: <b>%1</b>").arg(personName)));

    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->hideColumn(0); // id
    m_view->hideColumn(1); // person_id
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    layout->addWidget(m_view);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Додати період", this);
    QPushButton *delBtn = new QPushButton("Видалити", this);
    QPushButton *closeBtn = new QPushButton("Закрити", this);
    
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);

    connect(addBtn, &QPushButton::clicked, this, &StatusDialog::addStatus);
    connect(delBtn, &QPushButton::clicked, this, &StatusDialog::deleteStatus);
    connect(closeBtn, &QPushButton::clicked, this, &StatusDialog::accept);
}

void StatusDialog::addStatus() {
    int row = m_model->rowCount();
    m_model->insertRow(row);
    m_model->setData(m_model->index(row, 1), m_personId);
    m_model->setData(m_model->index(row, 2), "Відпустка");
    m_model->setData(m_model->index(row, 3), QDate::currentDate().toString(Qt::ISODate));
    m_model->setData(m_model->index(row, 4), QDate::currentDate().addDays(7).toString(Qt::ISODate));
    m_model->submitAll();
}

void StatusDialog::deleteStatus() {
    QModelIndexList selected = m_view->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    if (QMessageBox::question(this, "Видалення", "Видалити цей період?") == QMessageBox::Yes) {
        for (const QModelIndex &index : selected) {
            m_model->removeRow(index.row());
        }
        m_model->select();
    }
}
