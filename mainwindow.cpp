#include "mainwindow.h"
#include "waveformwidget.h"
#include "QVBoxLayout"
#include "QPushButton"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *myWidget = new WaveformWidget(central);   // or: new QWidget(central);

    auto *vbox = new QVBoxLayout(central);
    vbox->setContentsMargins(8,8,8,8);
    vbox->setSpacing(8);
    vbox->addWidget(myWidget, /*stretch*/ 1); // take remaining space

    auto *okBtn     = new QPushButton("OK", central);
    auto *cancelBtn = new QPushButton("Cancel", central);
    auto *btnRow    = new QHBoxLayout();
    btnRow->setSpacing(8);
    btnRow->addStretch();
    btnRow->addWidget(okBtn);
    btnRow->addWidget(cancelBtn);

    vbox->addLayout(btnRow); // add the horizontal row to the vertical layout

    connect(okBtn, &QPushButton::clicked, this, []{ /* ... */ });
 }

MainWindow::~MainWindow()
{
}
