#include "DutyTypesWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSqlRelation>
#include <QSqlRelationalDelegate>
#include <QHeaderView>
#include <QMessageBox>

DutyTypesWidget::DutyTypesWidget(QWidget *parent) : QWidget(parent) {
    m_model = new QSqlRelationalTableModel(this);
    m_model->setTable("duty_types");
    
    // Зв'язок: min_rank_id -> ranks(id), показувати name
    m_model->setRelation(3, QSqlRelation("ranks", "id", "name"));
    
    m_model->setHeaderData(1, Qt::Horizontal, "Назва наряду");
    m_model->setHeaderData(2, Qt::Horizontal, "Абревіатура");
    m_model->setHeaderData(3, Qt::Horizontal, "Мін. звання");
    
    m_model->select();

    setupUi();
}

void DutyTypesWidget::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->setItemDelegate(new QSqlRelationalDelegate(m_view));
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);

    layout->addWidget(m_view);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Додати тип наряду", this);
    QPushButton *delBtn = new QPushButton("Видалити обраний", this);
    
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addStretch();
    
    layout->addLayout(btnLayout);

    connect(addBtn, &QPushButton::clicked, this, &DutyTypesWidget::addDutyType);
    connect(delBtn, &QPushButton::clicked, this, &DutyTypesWidget::deleteDutyType);
}

void DutyTypesWidget::addDutyType() {
    int row = m_model->rowCount();
    m_model->insertRow(row);
    m_model->setData(m_model->index(row, 1), "Новий наряд");
    m_model->setData(m_model->index(row, 3), 1); // Дефолтне звання (Солдат)
}

void DutyTypesWidget::deleteDutyType() {
    QModelIndexList selected = m_view->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    if (QMessageBox::question(this, "Видалення", "Видалити цей тип наряду?") == QMessageBox::Yes) {
        for (const QModelIndex &index : selected) {
            m_model->removeRow(index.row());
        }
        m_model->select();
    }
}
