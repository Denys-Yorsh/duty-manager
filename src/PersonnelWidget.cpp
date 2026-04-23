#include "PersonnelWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSqlRelation>
#include <QSqlRelationalDelegate>
#include <QHeaderView>
#include <QMessageBox>

PersonnelWidget::PersonnelWidget(QWidget *parent) : QWidget(parent) {
    m_model = new QSqlRelationalTableModel(this);
    m_model->setTable("personnel");
    
    // Встановлюємо зв'язок: rank_id -> таблиця ranks, поле id -> показувати поле name
    m_model->setRelation(1, QSqlRelation("ranks", "id", "name"));
    
    m_model->setHeaderData(1, Qt::Horizontal, "Звання");
    m_model->setHeaderData(2, Qt::Horizontal, "ПІБ");
    m_model->setHeaderData(3, Qt::Horizontal, "Посада");
    
    m_model->select();

    setupUi();
}

void PersonnelWidget::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->setItemDelegate(new QSqlRelationalDelegate(m_view));
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);

    layout->addWidget(m_view);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Додати бійця", this);
    QPushButton *delBtn = new QPushButton("Видалити обраного", this);
    
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addStretch();
    
    layout->addLayout(btnLayout);

    connect(addBtn, &QPushButton::clicked, this, &PersonnelWidget::addPerson);
    connect(delBtn, &QPushButton::clicked, this, &PersonnelWidget::deletePerson);
}

void PersonnelWidget::addPerson() {
    int row = m_model->rowCount();
    m_model->insertRow(row);
    // Встановлюємо дефолтне звання (напр. id=1 "Солдат")
    m_model->setData(m_model->index(row, 1), 1); 
    m_model->setData(m_model->index(row, 2), "Новий боєць");
}

void PersonnelWidget::deletePerson() {
    QModelIndexList selected = m_view->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    if (QMessageBox::question(this, "Видалення", "Ви впевнені, що хочете видалити обраний запис?") == QMessageBox::Yes) {
        for (const QModelIndex &index : selected) {
            m_model->removeRow(index.row());
        }
        m_model->select();
    }
}
