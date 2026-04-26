#include "MainWindow.h"
#include "PersonnelWidget.h"
#include "DutyTypesWidget.h"
#include "ScheduleWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // Використовуємо шлях до вбудованих ресурсів
    setWindowIcon(QIcon(":/assets/icon.png"));
    setupUi();
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUi() {
    setWindowTitle("Модуль «Графік нарядів ВЧ»");
    resize(1200, 800);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    QTabWidget *tabs = new QTabWidget(this);
    
    PersonnelWidget *personnelTab = new PersonnelWidget(centralWidget);
    DutyTypesWidget *dutyTypesTab = new DutyTypesWidget(centralWidget);
    ScheduleWidget *scheduleTab = new ScheduleWidget(centralWidget);
    
    QIcon commonIcon(":/assets/icon.png");

    tabs->addTab(personnelTab, commonIcon, "Особовий склад");
    tabs->addTab(dutyTypesTab, commonIcon, "Види нарядів");
    tabs->addTab(scheduleTab, commonIcon, "Графік нарядів");
    
    mainLayout->addWidget(tabs);
}
