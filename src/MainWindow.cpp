#include "MainWindow.h"
#include "PersonnelWidget.h"
#include "DutyTypesWidget.h"
#include "ScheduleWidget.h"
#include "StatisticsWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    // Використовуємо шлях до вбудованих ресурсів
    setWindowIcon(QIcon(":/assets/icon.png"));
    setupUi();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    setWindowTitle("«Графік нарядів ВЧ»");
    resize(1200, 800);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QTabWidget *tabs = new QTabWidget(this);
    tabs->setStyleSheet(
        "QTabBar::tab:selected { "
        "   color: #F39200; "
        "   font-weight: bold; "
        "   background: white; "
        "} "
        "QTabBar::tab { "
        "   background: #f0f0f0; "
        "   border: 1px solid #dcdcdc; "
        "   border-bottom: none; "
        "   padding: 8px 20px; "
        "   border-top-left-radius: 4px; "
        "   border-top-right-radius: 4px; "
        "   margin-right: 2px; "
        "}"
    );

    PersonnelWidget *personnelTab = new PersonnelWidget(centralWidget);
    DutyTypesWidget *dutyTypesTab = new DutyTypesWidget(centralWidget);
    ScheduleWidget *scheduleTab = new ScheduleWidget(centralWidget);
    StatisticsWidget *statsTab = new StatisticsWidget(centralWidget);

    QIcon commonIcon(":/assets/icon.png");

    tabs->addTab(personnelTab, commonIcon, "Особовий склад");
    tabs->addTab(dutyTypesTab, commonIcon, "Види нарядів");
    tabs->addTab(scheduleTab, commonIcon, "Графік нарядів");
    tabs->addTab(statsTab, commonIcon, "Статистика");

    mainLayout->addWidget(tabs);
}
