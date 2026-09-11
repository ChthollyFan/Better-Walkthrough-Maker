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
    // 版本号由 src/CMakeLists.txt 从顶层 project(VERSION) 注入（BWM_VERSION 宏）；
    // 仅当脱离 CMake 单独编译该文件时才走下面的回退值，回退值需与 CMakeLists.txt 保持一致。
#ifdef BWM_VERSION
    QCoreApplication::setApplicationVersion(QStringLiteral(BWM_VERSION));
#else
    QCoreApplication::setApplicationVersion(QStringLiteral("0.3.0"));
#endif

    // 应用图标（窗口标题栏、任务栏、Alt+Tab）；exe 文件图标由 app.rc 嵌入
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/Chtholly.ico")));

    bwm::MainWindow window;
    window.show();
    return app.exec();
}
