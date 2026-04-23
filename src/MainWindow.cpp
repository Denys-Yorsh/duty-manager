#include "MainWindow.h"
#include "PersonnelWidget.h"
#include "DutyTypesWidget.h"
#include "ScheduleWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
    setWindowTitle("Модуль «Графік нарядів ВЧ»");
    resize(1200, 800);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    QTabWidget *tabs = new QTabWidget(this);
    
    // Вкладки
    PersonnelWidget *personnelTab = new PersonnelWidget(this);
    DutyTypesWidget *dutyTypesTab = new DutyTypesWidget(this);
    ScheduleWidget *scheduleTab = new ScheduleWidget(this);
    
    tabs->addTab(personnelTab, "Особовий склад");
    tabs->addTab(dutyTypesTab, "Види нарядів");
    tabs->addTab(scheduleTab, "Графік нарядів");
    
    mainLayout->addWidget(tabs);
}
