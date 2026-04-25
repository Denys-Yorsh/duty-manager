#include "MainWindow.h"
#include "PersonnelWidget.h"
#include "DutyTypesWidget.h"
#include "ScheduleWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QIcon>
#include <QDir>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // Получаем абсолютный путь к иконке относительно папки с программой
    QString iconPath = QDir(QCoreApplication::applicationDirPath()).filePath("assets/icon.png");
    
    // Если файла нет в папке сборки, проверяем в папке исходников (для отладки)
    if (!QFile::exists(iconPath)) {
        iconPath = "assets/icon.png"; 
    }

    setWindowIcon(QIcon(iconPath));
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
    
    QString iconPath = QDir(QCoreApplication::applicationDirPath()).filePath("assets/icon.png");
    if (!QFile::exists(iconPath)) {
        iconPath = "assets/icon.png";
    }
    QIcon commonIcon(iconPath);

    tabs->addTab(personnelTab, commonIcon, "Особовий склад");
    tabs->addTab(dutyTypesTab, commonIcon, "Види нарядів");
    tabs->addTab(scheduleTab, commonIcon, "Графік нарядів");
    
    mainLayout->addWidget(tabs);
}
