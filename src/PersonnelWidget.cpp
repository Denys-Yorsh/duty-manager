#include "PersonnelWidget.h"
#include "StatusDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSqlRelation>
#include <QSqlRelationalDelegate>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlRecord>
#include <QSqlError>
#include <QDebug>
#include <QInputDialog>
#include <QIcon>
#include <QCoreApplication>
#include <QDir>

// Спеціальна модель для відображення порядкових номерів
class PersonnelModel : public QSqlRelationalTableModel {
public:
    using QSqlRelationalTableModel::QSqlRelationalTableModel;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (role == Qt::DisplayRole && index.column() == 0) {
            return index.row() + 1; // Показуємо 1, 2, 3... замість ID
        }
        return QSqlRelationalTableModel::data(index, role);
    }
    
    Qt::ItemFlags flags(const QModelIndex &index) const override {
        if (index.column() == 0) return Qt::ItemIsEnabled | Qt::ItemIsSelectable; // Забороняємо редагувати № з/п
        return QSqlRelationalTableModel::flags(index);
    }
};

PersonnelWidget::PersonnelWidget(QWidget *parent) : QWidget(parent) {
    m_model = new PersonnelModel(this);
    m_model->setTable("personnel");
    m_model->setEditStrategy(QSqlRelationalTableModel::OnManualSubmit);
    m_model->setRelation(1, QSqlRelation("ranks", "id", "name"));
    m_model->setJoinMode(QSqlRelationalTableModel::LeftJoin);
    
    m_model->setHeaderData(0, Qt::Horizontal, "№ з/п");
    m_model->setHeaderData(1, Qt::Horizontal, "Звання");
    m_model->setHeaderData(2, Qt::Horizontal, "ПІБ");
    m_model->setHeaderData(3, Qt::Horizontal, "Посада");
    m_model->setHeaderData(4, Qt::Horizontal, "Примітки");
    
    m_model->select();
    setupUi();
}

PersonnelWidget::~PersonnelWidget() {}

void PersonnelWidget::setupUi() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->setItemDelegate(new QSqlRelationalDelegate(m_view));
    
    // Налаштовуємо ширину колонки № з/п
    m_view->setColumnWidth(0, 60);
    m_view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    
    // Решта колонок розтягуються
    for (int i = 1; i < 5; ++i) {
        m_view->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }
    
    m_view->verticalHeader()->setVisible(false); // Приховуємо стандартну нумерацію зліва
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_view);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Додати бійця", this);
    QPushButton *delBtn = new QPushButton("Видалити обраного", this);
    QPushButton *statusBtn = new QPushButton("Статус бійця", this);
    
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addWidget(statusBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(addBtn, &QPushButton::clicked, this, &PersonnelWidget::addPerson);
    connect(delBtn, &QPushButton::clicked, this, &PersonnelWidget::deletePerson);
    connect(statusBtn, &QPushButton::clicked, this, &PersonnelWidget::manageStatuses);
}

void PersonnelWidget::addPerson() {
    bool ok;
    QString name = QInputDialog::getText(this, "Новий боєць", 
                                         "Введіть ПІБ бійця:", QLineEdit::Normal, 
                                         "", &ok);
    
    if (ok && !name.trimmed().isEmpty()) {
        int row = m_model->rowCount();
        if (m_model->insertRow(row)) {
            m_model->setData(m_model->index(row, 1), 1); // Солдат
            m_model->setData(m_model->index(row, 2), name.trimmed());
            m_model->setData(m_model->index(row, 3), "Посада");
            m_model->setData(m_model->index(row, 4), "в наявності");
            
            if (m_model->submitAll()) {
                m_model->select();
                m_view->scrollToBottom();
            } else {
                QMessageBox::critical(this, "Помилка БД", m_model->lastError().text());
            }
        }
    }
}

void PersonnelWidget::deletePerson() {
    QModelIndexList selected = m_view->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;
    if (QMessageBox::question(this, "Видалення", "Ви впевнені?") == QMessageBox::Yes) {
        for (const QModelIndex &index : selected) m_model->removeRow(index.row());
        m_model->submitAll();
        m_model->select();
    }
}

void PersonnelWidget::manageStatuses() {
    QModelIndexList selected = m_view->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "Попередження", "Оберіть бійця.");
        return;
    }
    int row = selected.first().row();
    // Використовуємо m_model->sourceModel(), якщо б ми використовували Proxy, 
    // але тут ми просто беремо дані з моделі. Оскільки в базі це все ще id, 
    // нам треба бути обережними. 
    // Але QSqlTableModel::data(index, role) для id поверне реальний id з бази, 
    // навіть якщо ми перевизначили відображення.
    
    // Щоб отримати РЕАЛЬНИЙ ID для БД, викличемо оригінальний метод data:
    int personId = m_model->QSqlRelationalTableModel::data(m_model->index(row, 0)).toInt();
    QString name = m_model->record(row).value("full_name").toString();
    
    StatusDialog dlg(personId, name, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_model->select();
    }
}
