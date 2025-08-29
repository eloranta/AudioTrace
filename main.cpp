#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow window;
    window.setWindowTitle("Audio Trace");
    window.resize(900, 300);
    window.show();
    return app.exec();
}
