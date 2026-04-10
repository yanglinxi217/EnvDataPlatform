#include <QApplication>
#include <QFile>
#include <QSettings>
#include "core/database/databasemanager.h"
#include "modules/login/loginwindow.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("EnvDataPlatform");
    app.setOrganizationName("EnvDataPlatform");
    app.setApplicationVersion("1.0.0");

    // High-DPI support (Qt 6 enables it by default)

    // Initialize database
    if (!DatabaseManager::instance().initialize()) {
        return -1;
    }

    // Apply saved theme before showing login
    QSettings settings("EnvDataPlatform", "Settings");
    QString theme = settings.value("theme", "light").toString();
    QString qssFile = (theme == "dark") ? ":/styles/dark.qss" : ":/styles/light.qss";
    QFile f(qssFile);
    if (f.open(QFile::ReadOnly)) {
        app.setStyleSheet(QString::fromUtf8(f.readAll()));
        f.close();
    }

    // Show login window
    LoginWindow login;
    if (login.exec() != QDialog::Accepted) {
        return 0;  // User cancelled
    }

    // Launch main window with logged-in user
    MainWindow mainWin(login.loggedInUser());
    mainWin.show();

    return app.exec();
}
