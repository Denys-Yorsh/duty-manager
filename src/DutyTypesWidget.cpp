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

// Спеціальний делегат, щоб текст не дублювався під ComboBox
class DutyTypeDelegate : public QSqlRelationalDelegate {
public:
    explicit DutyTypeDelegate(QObject *parent = nullptr) : QSqlRelationalDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        // Колонки 2 (Мін) та 3 (Макс) - це ComboBox, там не малюємо текст
        if (index.column() == 2 || index.column() == 3) {
            QStyleOptionViewItem opt = option;
            opt.text = ""; // Очищаємо текст, щоб він не просвічував
            opt.widget->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
        } else {
            QSqlRelationalDelegate::paint(painter, option, index);
        }
    }
};

class DutyTypesModel : public QSqlRelationalTableModel {
public:
    using QSqlRelationalTableModel::QSqlRelationalTableModel;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (role == Qt::DisplayRole && index.column() == 0) {
            return index.row() + 1;
        }
        return QSqlRelationalTableModel::data(index, role);
    }
    
    Qt::ItemFlags flags(const QModelIndex &index) const override {
        if (index.column() == 0) return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        return QSqlRelationalTableModel::flags(index);
    }
};

DutyTypesWidget::DutyTypesWidget(QWidget *parent) : QWidget(parent) {
    // Перевірка та оновлення структури таблиці (якщо база стара)
    QSqlQuery checkCol;
    if (!checkCol.exec("SELECT max_rank_id FROM duty_types LIMIT 1")) {
        QSqlQuery alter;
        alter.exec("ALTER TABLE duty_types ADD COLUMN max_rank_id INTEGER REFERENCES ranks(id)");
    }

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
    
    m_model->select();
    setupUi();

    QTimer::singleShot(200, this, &DutyTypesWidget::updatePersistentEditors);
}

DutyTypesWidget::~DutyTypesWidget() {}

void DutyTypesWidget::updatePersistentEditors() {
    for (int i = 0; i < m_model->rowCount(); ++i) {
        m_view->openPersistentEditor(m_model->index(i, 2)); // Мін. звання
        m_view->openPersistentEditor(m_model->index(i, 3)); // Макс. звання
    }
}

void DutyTypesWidget::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->setItemDelegate(new DutyTypeDelegate(m_view)); 
    
    m_view->hideColumn(4); // Колір
    
    m_view->setColumnWidth(0, 60);
    m_view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
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
            QMessageBox::information(this, "Успіх", "Налаштування нарядів збережено.");
            m_model->select();
            updatePersistentEditors();
        } else {
            QMessageBox::critical(this, "Помилка", m_model->lastError().text());
        }
    });
}

void DutyTypesWidget::addDutyType() {
    bool ok;
    QString name = QInputDialog::getText(this, "Новий наряд", 
                                         "Введіть назву наряду:", QLineEdit::Normal, 
                                         "", &ok);
    
    if (ok && !name.trimmed().isEmpty()) {
        int minId = 1;
        int maxId = 1;
        QSqlQuery qRank("SELECT MIN(id), MAX(id) FROM ranks");
        if (qRank.exec() && qRank.next()) {
            minId = qRank.value(0).toInt();
            maxId = qRank.value(1).toInt();
        }

        QSqlQuery query;
        query.prepare("INSERT INTO duty_types (name, min_rank_id, max_rank_id, person_count) VALUES (?, ?, ?, 1)");
        query.addBindValue(name.trimmed());
        query.addBindValue(minId);
        query.addBindValue(maxId);
        
        if (query.exec()) {
            m_model->select();
            updatePersistentEditors();
            m_view->scrollToBottom();
        } else {
            QMessageBox::critical(this, "Помилка БД", "Не вдалося додати наряд:\n" + query.lastError().text());
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
        updatePersistentEditors();
    }
}
