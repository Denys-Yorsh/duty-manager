#include "StatusDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QDate>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QIcon>
#include <QDir>
#include <QCoreApplication>
#include <QComboBox>
#include <QStyledItemDelegate>

class StatusDelegate : public QStyledItemDelegate {
public:
    explicit StatusDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override {
        QStyledItemDelegate::initStyleOption(option, index);
        if (index.column() == 2) option->text = ""; // Прибираємо текст під комбобоксом
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        if (index.column() == 2) {
            QComboBox *editor = new QComboBox(parent);
            editor->addItems({"Відпустка", "Лікарняний", "Відрядження", "Інше"});
            return editor;
        }
        return QStyledItemDelegate::createEditor(parent, option, index);
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override {
        if (index.column() == 2) {
            QComboBox *cb = qobject_cast<QComboBox*>(editor);
            if (cb) cb->setCurrentText(index.data().toString());
        } else QStyledItemDelegate::setEditorData(editor, index);
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override {
        if (index.column() == 2) {
            QComboBox *cb = qobject_cast<QComboBox*>(editor);
            if (cb) model->setData(index, cb->currentText());
        } else QStyledItemDelegate::setModelData(editor, model, index);
    }
};

class StatusModel : public QSqlTableModel {
public:
    using QSqlTableModel::QSqlTableModel;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (index.column() == 0) {
            if (role == Qt::DisplayRole) return index.row() + 1;
            if (role == Qt::TextAlignmentRole) return Qt::AlignCenter;
        }
        return QSqlTableModel::data(index, role);
    }
};

StatusDialog::StatusDialog(int personId, const QString &personName, QWidget *parent)
    : QDialog(parent), m_personId(personId) {
    m_model = new StatusModel(this);
    m_model->setTable("personnel_statuses");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_model->setFilter(QString("person_id = %1").arg(m_personId));
    m_model->select();

    m_model->setHeaderData(0, Qt::Horizontal, "№ з/п");
    m_model->setHeaderData(2, Qt::Horizontal, "Тип");
    m_model->setHeaderData(3, Qt::Horizontal, "Початок");
    m_model->setHeaderData(4, Qt::Horizontal, "Кінець");
    m_model->setHeaderData(5, Qt::Horizontal, "Коментар");

    setupUi(personName);
}

StatusDialog::~StatusDialog() {}

void StatusDialog::setupUi(const QString &personName) {
    setWindowTitle("Статус в/c: " + personName);
    resize(600, 400);
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->setItemDelegate(new StatusDelegate(m_view));
    m_view->hideColumn(1);
    m_view->setColumnWidth(0, 60);
    m_view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    for (int i = 2; i < 6; ++i) m_view->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    m_view->verticalHeader()->setVisible(false);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_view);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Додати запис", this);
    QPushButton *delBtn = new QPushButton("Видалити", this);
    QPushButton *saveBtn = new QPushButton("Зберегти все", this);
    saveBtn->setStyleSheet("font-weight: bold;");
    btnLayout->addWidget(addBtn); btnLayout->addWidget(delBtn); btnLayout->addStretch(); btnLayout->addWidget(saveBtn);
    layout->addLayout(btnLayout);

    connect(addBtn, &QPushButton::clicked, this, &StatusDialog::addStatus);
    connect(delBtn, &QPushButton::clicked, this, &StatusDialog::deleteStatus);
    connect(saveBtn, &QPushButton::clicked, this, &StatusDialog::saveAndSync);
    updatePersistentEditors();
}

void StatusDialog::updatePersistentEditors() {
    for(int i=0; i<m_model->rowCount(); ++i) m_view->openPersistentEditor(m_model->index(i, 2));
}

void StatusDialog::addStatus() {
    int row = m_model->rowCount();
    m_model->insertRow(row);
    m_model->setData(m_model->index(row, 1), m_personId);
    m_model->setData(m_model->index(row, 2), "Відпустка");
    m_model->setData(m_model->index(row, 3), QDate::currentDate().toString(Qt::ISODate));
    m_model->setData(m_model->index(row, 4), QDate::currentDate().addDays(15).toString(Qt::ISODate));
    updatePersistentEditors();
}

void StatusDialog::deleteStatus() {
    QModelIndexList selected = m_view->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;
    for (const QModelIndex &index : selected) m_model->removeRow(index.row());
    m_model->submitAll(); m_model->select(); updatePersistentEditors();
}

void StatusDialog::saveAndSync() {
    if (!m_model->submitAll()) return;
    QString currentNote = "в наявності";
    QDate today = QDate::currentDate();
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QSqlRecord rec = m_model->record(i);
        QDate start = QDate::fromString(rec.value("start_date").toString(), Qt::ISODate);
        QDate end = QDate::fromString(rec.value("end_date").toString(), Qt::ISODate);
        if (today >= start && today <= end) { currentNote = rec.value("status_type").toString(); break; }
    }
    QSqlQuery query;
    query.prepare("UPDATE personnel SET is_active = ? WHERE id = ?");
    query.addBindValue(currentNote); query.addBindValue(m_personId);
    if (query.exec()) accept();
}
