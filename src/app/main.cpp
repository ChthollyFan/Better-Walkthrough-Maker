/**
 * @file main.cpp
 * @author zhangweimu
 * @brief 应用入口。
 */
#include <QApplication>
#include <QCoreApplication>
#include <QIcon>

#include "app/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
#ifdef Q_OS_WIN
    // 亚克力半透明背景下，Fusion style 绘制最稳定（原生 WindowsStyle 在透明窗口上易异常）
    QApplication::setStyle(QStringLiteral("Fusion"));
#endif
    QCoreApplication::setOrganizationName(QStringLiteral("bwm"));
    QCoreApplication::setApplicationName(QStringLiteral("BetterWalkthroughMaker"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    // 应用图标（窗口标题栏、任务栏、Alt+Tab）；exe 文件图标由 app.rc 嵌入
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/Chtholly.ico")));

    bwm::MainWindow window;
    window.show();
    return app.exec();
}
