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
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTimer>
#include <QPainter>

// Делегат для контролю відмальовування колонок з випадаючими списками
class DutyTypeDelegate : public QSqlRelationalDelegate {
public:
    explicit DutyTypeDelegate(QObject *parent = nullptr) : QSqlRelationalDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        if (index.column() == 2 || index.column() == 3) {
            // Малюємо тільки фон (текст не дублюємо під QComboBox)
            QStyleOptionViewItem opt = option;
            opt.text = "";
            QSqlRelationalDelegate::paint(painter, opt, index);
        } else {
            QSqlRelationalDelegate::paint(painter, option, index);
        }
    }
};

class DutyTypesModel : public QSqlRelationalTableModel {
public:
    using QSqlRelationalTableModel::QSqlRelationalTableModel;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (role == Qt::DisplayRole && index.column() == 0) return index.row() + 1;
        return QSqlRelationalTableModel::data(index, role);
    }
    
    Qt::ItemFlags flags(const QModelIndex &index) const override {
        if (index.column() == 0) return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        return QSqlRelationalTableModel::flags(index);
    }
};

DutyTypesWidget::DutyTypesWidget(QWidget *parent) : QWidget(parent) {
    m_model = new DutyTypesModel(this);
    m_model->setTable("duty_types");
    m_model->setEditStrategy(QSqlRelationalTableModel::OnManualSubmit);
    m_model->setRelation(2, QSqlRelation("ranks", "id", "name"));
    m_model->setRelation(3, QSqlRelation("ranks", "id", "name"));
    
    m_model->setHeaderData(0, Qt::Horizontal, "№ з/п");
    m_model->setHeaderData(1, Qt::Horizontal, "Назва наряду");
    m_model->setHeaderData(2, Qt::Horizontal, "Мін. звання");
    m_model->setHeaderData(3, Qt::Horizontal, "Макс. звання");
    m_model->setHeaderData(5, Qt::Horizontal, "К-сть осіб");
    m_model->setHeaderData(6, Qt::Horizontal, "Дні відпочинку");
    
    m_model->select();
    setupUi();
    QTimer::singleShot(200, this, &DutyTypesWidget::updatePersistentEditors);
}

DutyTypesWidget::~DutyTypesWidget() {}

void DutyTypesWidget::updatePersistentEditors() {
    for (int i = 0; i < m_model->rowCount(); ++i) {
        m_view->openPersistentEditor(m_model->index(i, 2));
        m_view->openPersistentEditor(m_model->index(i, 3));
    }
}

void DutyTypesWidget::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->setItemDelegate(new DutyTypeDelegate(m_view)); 
    m_view->hideColumn(4); // Приховуємо колонку кольору
    
    m_view->horizontalHeader()->moveSection(6, 5); // Дні відпочинку перед К-сть осіб
    m_view->setColumnWidth(0, 60);
    m_view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    for (int i = 1; i < m_model->columnCount(); ++i) m_view->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);

    m_view->verticalHeader()->setVisible(false);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_view);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Додати тип наряду", this);
    QPushButton *delBtn = new QPushButton("Видалити обраний", this);
    QPushButton *saveBtn = new QPushButton("Зберегти зміни", this);
    saveBtn->setStyleSheet("font-weight: bold; color: green;");
    
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    layout->addLayout(btnLayout);

    connect(addBtn, &QPushButton::clicked, this, &DutyTypesWidget::addDutyType);
    connect(delBtn, &QPushButton::clicked, this, &DutyTypesWidget::deleteDutyType);
    connect(saveBtn, &QPushButton::clicked, this, [this](){
        if (m_model->submitAll()) {
            QMessageBox::information(this, "Успіх", "Налаштування збережено.");
            m_model->select();
            updatePersistentEditors();
        } else {
            QMessageBox::critical(this, "Помилка", m_model->lastError().text());
        }
    });
}

void DutyTypesWidget::addDutyType() {
    bool ok;
    QString name = QInputDialog::getText(this, "Новий наряд", "Назва наряду:", QLineEdit::Normal, "", &ok);
    if (ok && !name.trimmed().isEmpty()) {
        QSqlQuery query;
        query.prepare("INSERT INTO duty_types (name, min_rank_id, max_rank_id, person_count, rest_days) VALUES (?, 1, 1, 1, 1)");
        query.addBindValue(name.trimmed());
        if (query.exec()) {
            m_model->select();
            updatePersistentEditors();
            m_view->scrollToBottom();
        }
    }
}

void DutyTypesWidget::deleteDutyType() {
    QModelIndexList selected = m_view->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;
    if (QMessageBox::question(this, "Видалення", "Видалити?") == QMessageBox::Yes) {
        for (const QModelIndex &index : selected) m_model->removeRow(index.row());
        m_model->submitAll();
        m_model->select();
        updatePersistentEditors();
    }
}
