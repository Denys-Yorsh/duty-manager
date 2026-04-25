#include "DutyTypesWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSqlRelation>
#include <QSqlRelationalDelegate>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlError>
#include <QInputDialog>

// Специальная модель для отображения порядковых номеров
class DutyTypesModel : public QSqlRelationalTableModel {
public:
    using QSqlRelationalTableModel::QSqlRelationalTableModel;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (role == Qt::DisplayRole && index.column() == 0) {
            return index.row() + 1; // Показываем 1, 2, 3... вместо ID
        }
        return QSqlRelationalTableModel::data(index, role);
    }
    
    Qt::ItemFlags flags(const QModelIndex &index) const override {
        if (index.column() == 0) return Qt::ItemIsEnabled | Qt::ItemIsSelectable; // Запрещаем редактировать № з/п
        return QSqlRelationalTableModel::flags(index);
    }
};

DutyTypesWidget::DutyTypesWidget(QWidget *parent) : QWidget(parent) {
    m_model = new DutyTypesModel(this);
    m_model->setTable("duty_types");
    m_model->setEditStrategy(QSqlRelationalTableModel::OnManualSubmit);
    
    // Связь: min_rank_id -> ranks(id), показывать name
    m_model->setRelation(3, QSqlRelation("ranks", "id", "name"));
    
    m_model->setHeaderData(0, Qt::Horizontal, "№ з/п");
    m_model->setHeaderData(1, Qt::Horizontal, "Назва наряду");
    m_model->setHeaderData(2, Qt::Horizontal, "Абревіатура");
    m_model->setHeaderData(3, Qt::Horizontal, "Мін. звання");
    m_model->setHeaderData(5, Qt::Horizontal, "К-сть осіб");
    
    m_model->select();
    setupUi();
}

DutyTypesWidget::~DutyTypesWidget() {}

void DutyTypesWidget::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->setItemDelegate(new QSqlRelationalDelegate(m_view));
    
    // Скрываем ненужные колонки: ID (0 в БД, но используется для номера) и color_code (4)
    m_view->hideColumn(4); // color_code
    
    // Настраиваем ширину колонки № з/п
    m_view->setColumnWidth(0, 60);
    m_view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    
    // Остальные колонки растягиваются
    m_view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_view->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_view->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_view->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);

    m_view->verticalHeader()->setVisible(false);
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
    bool ok;
    QString name = QInputDialog::getText(this, "Новий наряд", 
                                         "Введіть назву наряду:", QLineEdit::Normal, 
                                         "", &ok);
    
    if (ok && !name.trimmed().isEmpty()) {
        int row = m_model->rowCount();
        if (m_model->insertRow(row)) {
            m_model->setData(m_model->index(row, 1), name.trimmed());
            m_model->setData(m_model->index(row, 2), "АБР");
            m_model->setData(m_model->index(row, 3), 1); // Солдат
            m_model->setData(m_model->index(row, 5), 1); // К-сть осіб
            
            if (m_model->submitAll()) {
                m_model->select();
                m_view->scrollToBottom();
            } else {
                QMessageBox::critical(this, "Помилка БД", "Можливо, наряд з такою назвою вже існує.");
                m_model->select();
            }
        }
    }
}

void DutyTypesWidget::deleteDutyType() {
    QModelIndexList selected = m_view->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    if (QMessageBox::question(this, "Видалення", "Видалити цей тип наряду?") == QMessageBox::Yes) {
        for (const QModelIndex &index : selected) {
            m_model->removeRow(index.row());
        }
        m_model->submitAll();
        m_model->select();
    }
}
