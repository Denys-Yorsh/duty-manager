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
#include <QInputDialog>
#include <QTimer>
#include <QPainter>

// Спеціальний делегат для контролю відмальовування та редагування
class RankDelegate : public QSqlRelationalDelegate
{
public:
    explicit RankDelegate(QObject *parent = nullptr) : QSqlRelationalDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (index.column() == 1)
        {
            // Малюємо тільки фон комірки для колонки Звання (щоб текст не дублювався)
            QStyleOptionViewItem opt = option;
            opt.text = "";
            opt.widget->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
        }
        else
        {
            QSqlRelationalDelegate::paint(painter, option, index);
        }
    }

    // Забороняємо відкривати редактор для № з/п та Приміток
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (index.column() == 0 || index.column() == 4)
        {
            return nullptr;
        }
        return QSqlRelationalDelegate::createEditor(parent, option, index);
    }
};

// Модель для відображення порядкових номерів
class PersonnelModel : public QSqlRelationalTableModel
{
public:
    using QSqlRelationalTableModel::QSqlRelationalTableModel;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::DisplayRole && index.column() == 0)
        {
            return index.row() + 1;
        }
        return QSqlRelationalTableModel::data(index, role);
    }
    // Дозволяємо ItemIsEditable для програмної вставки даних
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        return QSqlRelationalTableModel::flags(index);
    }
};

PersonnelWidget::PersonnelWidget(QWidget *parent) : QWidget(parent)
{
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

    QTimer::singleShot(200, this, &PersonnelWidget::updatePersistentEditors);
}

PersonnelWidget::~PersonnelWidget() {}

void PersonnelWidget::updatePersistentEditors()
{
    for (int i = 0; i < m_model->rowCount(); ++i)
    {
        m_view->openPersistentEditor(m_model->index(i, 1));
    }
}

void PersonnelWidget::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->setItemDelegate(new RankDelegate(m_view));

    m_view->setColumnWidth(0, 60);
    m_view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);

    for (int i = 1; i < 5; ++i)
    {
        m_view->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }

    m_view->verticalHeader()->setVisible(false);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);

    layout->addWidget(m_view);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Додати бійця", this);
    QPushButton *delBtn = new QPushButton("Видалити бійця", this);
    QPushButton *statusBtn = new QPushButton("Статус бійця", this);
    QPushButton *saveBtn = new QPushButton("Зберегти все", this);
    saveBtn->setStyleSheet("font-weight: bold; color: green;");

    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addWidget(statusBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    layout->addLayout(btnLayout);

    connect(addBtn, &QPushButton::clicked, this, &PersonnelWidget::addPerson);
    connect(delBtn, &QPushButton::clicked, this, &PersonnelWidget::deletePerson);
    connect(statusBtn, &QPushButton::clicked, this, &PersonnelWidget::manageStatuses);
    connect(saveBtn, &QPushButton::clicked, this, [this]()
            {
        if (m_model->submitAll()) {
            m_model->select();
            updatePersistentEditors();
            QMessageBox::information(this, "Успіх", "Інформація збережена");
        } else {
            QMessageBox::critical(this, "Помилка", m_model->lastError().text());
        } });
}

void PersonnelWidget::addPerson()
{
    bool ok;
    QString name = QInputDialog::getText(this, "Новий боєць", "Введіть ПІБ бійця:", QLineEdit::Normal, "", &ok);
    if (ok && !name.trimmed().isEmpty())
    {
        int row = m_model->rowCount();
        if (m_model->insertRow(row))
        {
            // Пряме заповнення даних в кеш моделі
            m_model->setData(m_model->index(row, 1), 1); // Солдат
            m_model->setData(m_model->index(row, 2), name.trimmed());
            m_model->setData(m_model->index(row, 3), "Посада");
            m_model->setData(m_model->index(row, 4), "в наявності");

            m_view->scrollToBottom();

            // Примусове перемальовування та відкриття списку звань
            QTimer::singleShot(100, this, [this, row]()
                               {
                m_view->openPersistentEditor(m_model->index(row, 1));
                m_view->viewport()->update(); });
        }
    }
}

void PersonnelWidget::deletePerson()
{
    QModelIndexList selected = m_view->selectionModel()->selectedRows();
    if (selected.isEmpty())
        return;
    if (QMessageBox::question(this, "Видалення", "Ви впевнені?") == QMessageBox::Yes)
    {
        for (const QModelIndex &index : selected)
            m_model->removeRow(index.row());
        m_model->submitAll();
        m_model->select();
        updatePersistentEditors();
    }
}

void PersonnelWidget::manageStatuses()
{
    QModelIndexList selected = m_view->selectionModel()->selectedRows();
    if (selected.isEmpty())
    {
        QMessageBox::warning(this, "Попередження", "Оберіть бійця.");
        return;
    }
    int row = selected.first().row();
    int personId = m_model->record(row).value("id").toInt();
    if (personId <= 0)
    {
        QMessageBox::information(this, "Збереження", "Спочатку натисніть 'Зберегти все'");
        return;
    }
    QString name = m_model->record(row).value("full_name").toString();
    StatusDialog dlg(personId, name, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        m_model->select();
        updatePersistentEditors();
    }
}
