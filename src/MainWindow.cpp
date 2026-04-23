#include "MainWindow.h"
#include "PersonnelWidget.h"
#include "DutyTypesWidget.h"
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
    QWidget *scheduleTab = new QWidget();
    
    tabs->addTab(personnelTab, "Особовий склад");
    tabs->addTab(dutyTypesTab, "Види нарядів");
    tabs->addTab(scheduleTab, "Графік нарядів");
    
    mainLayout->addWidget(tabs);

    // Заглушка для графіка
    QVBoxLayout *sLayout = new QVBoxLayout(scheduleTab);
    sLayout->addWidget(new QLabel("Тут буде календарна сітка", scheduleTab));
}
