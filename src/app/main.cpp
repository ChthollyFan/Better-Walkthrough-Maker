#include <QApplication>
#include <QCoreApplication>

#include "app/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("bwm"));
    QCoreApplication::setApplicationName(QStringLiteral("BetterWalkthroughMaker"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    bwm::MainWindow window;
    window.show();
    return app.exec();
}
