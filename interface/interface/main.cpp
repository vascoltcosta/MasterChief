#include "interface.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/app_icon.ico"));
    interface w;
    w.show();
    return a.exec();
}
