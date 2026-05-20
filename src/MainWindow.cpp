#include "MainWindow.h"
#include <QIcon>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowIcon(QIcon(":/assets/icon.png"));
}

MainWindow::~MainWindow()
{
}
