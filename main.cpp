#include <QApplication>
#include "ThreeDViewWidget.h"

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    ThreeDimensionalDisplayPage w;
    w.show();

    return a.exec();
}
