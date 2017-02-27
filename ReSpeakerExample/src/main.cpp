#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // app.setApplicationName("Qt Multimedia spectrum analyzer");

    MainWindow w;
    w.show();

    return a.exec();
}
