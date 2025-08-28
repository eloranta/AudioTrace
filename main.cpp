#include <QApplication>
#include "waveformwidget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    WaveformWidget w;
    w.setWindowTitle("Audio Trace (Qt)");
    w.resize(900, 300);
    w.show();
    return app.exec();
}
