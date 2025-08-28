#include <QApplication>
#include "waveformwidget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    WaveformWidget widget;
    widget.setWindowTitle("Audio Trace");
    widget.resize(900, 300);
    widget.show();
    return app.exec();
}
